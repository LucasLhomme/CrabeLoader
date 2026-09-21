/*
** CrabeLoader
** File description:
** The DLL entry point: starts the loader thread and finishes installing the crash reporter.
** SymInitialize enumerates modules, which deadlocks under the loader lock, so it happens here.
** Forwards every export to bink2w32_orig.dll through the vendored include/minhook/lib/proxy.
**
** Authors: @LucasLhomme
*/

#include "minhook/lib/proxy/proxy.hpp" // BINKW32 PROXY DIRECT LINKER FORWARDING
#include <windows.h>
#include <thread>
#include <string>
#include "shared/logger.hpp"
#include "shared/version.hpp"
#include "application/loader.hpp"
#include "application/multiplayer/multiplayer_manager.hpp"
#include "domain/game_profile.hpp"
#include "infrastructure/crash_reporter.hpp"

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
}

void initLogger(HMODULE hModule) {
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
    logger.setLogFile(moduleLogPath(hModule));
    logger.setLogLevel(crabe::shared::LogLevel::DEBUG);
    logger.info("CrabeLoader v{} initialized (built {} {}).",
                crabe::version::String, crabe::version::BuildDate, crabe::version::BuildTime);
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

void initMain() {
    crabe::infrastructure::CrashReporter::declareThreadRole(
        crabe::infrastructure::ThreadRole::Script);
    crabe::infrastructure::CrashReporter::pushBreadcrumb("main: script thread started");

    initCrashReporting();

    crabe::application::Loader::get().initialize();
}

bool APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    (void)lpReserved;
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        initLogger(hModule);

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
        crabe::multiplayer::application::MultiplayerManager::getInstance().applyMemoryPatchesNow();
        std::thread(initMain).detach();
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
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
