/*
** CrabeLoader
** File description:
** The DLL entry point: starts the loader thread and finishes installing the crash reporter.
** SymInitialize enumerates modules, which deadlocks under the loader lock, so it happens here.
** Answers as dinput8.dll and keeps the Bink forwarders of include/minhook/lib/proxy for 1.x setups.
**
** Authors: @LucasLhomme
*/

#include "minhook/lib/proxy/proxy.hpp" // BINKW32 PROXY DIRECT LINKER FORWARDING
#include <windows.h>
#include <filesystem>
#include <thread>
#include <string>
#include "shared/logger.hpp"
#include "shared/version.hpp"
#include "application/loader.hpp"
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
    std::string moduleDirectory(HMODULE hModule) {
        char path[MAX_PATH];
        DWORD len = GetModuleFileNameA(hModule, path, MAX_PATH);
        std::string dir(path, len);

        auto pos = dir.find_last_of("\\/");
        dir.resize(pos != std::string::npos ? pos + 1 : 0);

        return dir;
    }

    std::string moduleLogPath(HMODULE hModule) {
        return moduleDirectory(hModule) + "loader.log";
    }

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

void initLogger(HMODULE hModule) {
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
    logger.setLogFile(moduleLogPath(hModule));
    logger.setLogLevel(crabe::shared::LogLevel::DEBUG);
    logger.info("CrabeLoader v{} initialized (built {} {}, loaded as {}).",
                crabe::version::String, crabe::version::BuildDate, crabe::version::BuildTime,
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
}

// Loads the .asi plugins of the folder holding the game's executable, once.
void loadAsiPlugins() {
    crabe::infrastructure::CrashReporter::pushBreadcrumb("main: loading .asi plugins");
    crabe::infrastructure::AsiLoader::get().loadAll(g_gameRoot);
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

// Loads .asi plugins just before the game's entry point when the game imported this DLL at startup,
// which is when plugins expect to run. A DLL loaded later finds the entry point already passed, and so
// do plugins when the detour cannot be installed: both load them from the script thread instead.
void scheduleAsiPlugins(bool loadedAtStartup) {
    g_gameRoot = executableDirectory();
    if (loadedAtStartup && crabe::infrastructure::runBeforeEntryPoint(&loadAsiPlugins))
        return;
    if (loadedAtStartup)
        crabe::shared::Logger::getInstance().warning(
            "AsiLoader: could not run before the game's entry point; .asi plugins load from the loader thread.");
    g_asiOnScriptThread = true;
}

// Starts the loader when the game loads this DLL, unless another copy of CrabeLoader outranks it.
// A copy that steps aside only forwards its exports, so a folder holding a 1.x bink2w32.dll and a
// dinput8.dll runs exactly one loader.
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
        const std::string crashFolder = moduleDirectory(hModule) + "Crabe";
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
