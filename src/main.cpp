/*
** CrabeLoader
** File description:
** The DLL entry point: starts the loader thread and finishes installing the crash reporter.
** SymInitialize enumerates modules, which deadlocks under the loader lock, so it happens here.
** Built as CrabeLoader.asi, started by the dinput8.dll bootstrap or any ASI loader.
**
** Authors: @LucasLhomme
*/

#include "minhook/lib/proxy/proxy.hpp" // BINKW32 PROXY DIRECT LINKER FORWARDING
#include <windows.h>
#include <filesystem>
#include <mutex>
#include <thread>
#include <string>
#include "shared/logger.hpp"
#include "shared/build_info.hpp"
#include "shared/version.hpp"
#include "application/loader.hpp"
#include "application/plugin_api.hpp"
#if CRABELOADER_WITH_MULTIPLAYER
#include "application/multiplayer/multiplayer_manager.hpp"
#endif
#include "domain/game_profile.hpp"
#include "infrastructure/asi_loader.hpp"
#include "infrastructure/crash_reporter.hpp"
#include "infrastructure/engine_resolution.hpp"
#include "infrastructure/entry_point_hook.hpp"
#include "infrastructure/loader_instance.hpp"
#include "presentation/render_hook.hpp"

namespace {
    // Returns the folder of the game's executable, or the current folder if Windows cannot name it.
    std::filesystem::path executableDirectory() {
        std::wstring path(MAX_PATH, L'\0');
        for (;;) {
            const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
            if (length == 0)
                return std::filesystem::current_path();
            if (length < path.size()) {
                path.resize(length);
                return std::filesystem::path(path).parent_path();
            }
            path.resize(path.size() * 2);
        }
    }

    // Narrows a module name for the log, which is UTF-8; a character outside ASCII becomes '?'.
    std::string asciiName(std::wstring_view name) {
        std::string out;
        out.reserve(name.size());
        for (const wchar_t c : name)
            out.push_back(c < 0x80 ? static_cast<char>(c) : '?');
        return out;
    }
}

// Opens loader.log next to the game's executable, wherever this DLL itself was loaded from: an ASI
// loader may load CrabeLoader.asi from scripts/ or plugins/, and the log and Crabe/ stay in one place.
void initLogger(HMODULE hModule) {
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
    logger.setLogFile((executableDirectory() / "loader.log").string());
    logger.setLogLevel(crabe::shared::LogLevel::DEBUG);
    logger.info("CrabeLoader v{} initialized (built {}, commit {}, loaded as {}).",
                crabe::build::label(), crabe::build::time(), crabe::build::commit(),
                asciiName(crabe::infrastructure::moduleFileName(hModule)));
}

// The half of crash reporting that must not run under the loader lock:
// SymInitialize loads and enumerates modules, and doing that from DllMain
// deadlocks. Running here also means the game profile is already detected, so
// the reporter can name the build without calling back into domain from inside
// a handler.
void initCrashReporting() {
    crabe::infrastructure::CrashReporter::completeInstall();

    const crabe::domain::GameProfile* profile = crabe::domain::activeProfile();
    const crabe::domain::PeIdentity identity = crabe::domain::runningGameIdentity();

    crabe::infrastructure::CrashReporter::setGameProfile(
        profile ? profile->id : std::string_view{},
        profile ? profile->displayName : std::string_view{},
        identity.timeDateStamp, identity.sizeOfImage, identity.checkSum);
}

namespace {
    bool g_forwardOnly = false;
    bool g_asiOnScriptThread = false;
    std::filesystem::path g_gameRoot;
    std::once_flag g_asiOnce;
}

// Loads the .asi plugins of the folder holding the game's executable, then hands the plugin API to each
// one that exports CrabePluginInit. Runs once: a second caller waits until the first one has finished.
void loadAsiPlugins() {
    std::call_once(g_asiOnce, [] {
        crabe::infrastructure::CrashReporter::pushBreadcrumb("main: loading .asi plugins");
        crabe::infrastructure::AsiLoader& loader = crabe::infrastructure::AsiLoader::get();
        loader.loadAll(g_gameRoot);
        crabe::application::PluginApi::get().startPlugins(loader.records());
    });
}

// Called by the bootstrap dinput8.dll right after it loaded this file at the game's entry point, outside
// the loader lock and before any game code: the .asi plugins load there and then, as they expect to.
extern "C" __declspec(dllexport) void __cdecl CrabeLoaderBootstrap() {
    if (!g_forwardOnly)
        loadAsiPlugins();
}

void initMain() {
    crabe::infrastructure::CrashReporter::declareThreadRole(
        crabe::infrastructure::ThreadRole::Script);
    crabe::infrastructure::CrashReporter::pushBreadcrumb("main: script thread started");

    initCrashReporting();

    if (g_asiOnScriptThread)
        loadAsiPlugins();

    crabe::application::Loader::get().initialize();
}

// Loads .asi plugins just before the game's entry point when the game imported this DLL at startup.
// Loaded later (by the bootstrap or another ASI loader), they load from the script thread, unless the
// bootstrap calls CrabeLoaderBootstrap first, which it does before the game's code runs.
void scheduleAsiPlugins(bool loadedAtStartup) {
    g_gameRoot = executableDirectory();
    if (loadedAtStartup && crabe::infrastructure::runBeforeEntryPoint(&loadAsiPlugins))
        return;
    if (loadedAtStartup)
        crabe::shared::Logger::getInstance().warning(
            "AsiLoader: could not run before the game's entry point; .asi plugins load from the loader thread.");
    g_asiOnScriptThread = true;
}

// Starts the loader when this DLL is loaded, unless another copy of CrabeLoader outranks it.
// A copy that steps aside starts nothing, so a folder still holding a 1.x bink2w32.dll loader and
// CrabeLoader.asi runs exactly one loader.
bool APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        g_forwardOnly = !crabe::infrastructure::outrankingLoaderCopy(hModule).empty();
        if (g_forwardOnly)
            return TRUE;
        DisableProcessWindowsGhosting();
        initLogger(hModule);
        crabe::presentation::RenderHook::installEarlyDelayLoadHook();
        // Before the engine's first boot read of the saved resolution, which is why it is not
        // left to Loader::initialize on the script thread.
        crabe::infrastructure::EngineResolution::get().initialize();

        // Installed before anything else runs, and before the game gets a
        // chance to install a filter of its own. install() adds a vectored
        // handler at priority 1 and keeps SetUnhandledExceptionFilter behind
        // it as a secondary net; the DbgHelp half waits for the script thread
        // (initCrashReporting above), because SymInitialize under the loader
        // lock deadlocks.
        const std::string crashFolder = (executableDirectory() / "Crabe").string();
        crabe::infrastructure::CrashReporter::install(crashFolder.c_str());
        crabe::infrastructure::CrashReporter::pushBreadcrumb("main: DLL_PROCESS_ATTACH");

        // Said once, here, and never from the handler itself: naming the folder
        // costs a std::format and a std::string, which is exactly what a
        // handler running inside a corrupt heap must not do. So the log carries
        // the destination up front, and the handler writes there in silence.
        crabe::shared::Logger::getInstance().info(
            "CrabeLoader: crash reports and minidumps will be written to '{}'.", crashFolder);

        crabe::shared::Logger::getInstance().debug("CrabeLoader DLL loaded.");
        scheduleAsiPlugins(lpReserved != nullptr);
#if CRABELOADER_WITH_MULTIPLAYER
        crabe::multiplayer::application::MultiplayerManager::getInstance().applyMemoryPatchesNow();
#endif
        std::thread(initMain).detach();
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH && !g_forwardOnly) {
        crabe::shared::Logger::getInstance().debug("CrabeLoader DLL unloaded.");
    }
    return TRUE;
}

int main()
{
    DllMain(GetModuleHandle(NULL), DLL_PROCESS_ATTACH, NULL);
    crabe::shared::Logger::getInstance().info("CrabeLoader started.");

    return 0;
}
