/*
** CrabeLoader
** File description:
** main file for the CrabeLoader project
*/

#include "minhook/lib/proxy/proxy.hpp" // BINKW32 PROXY DIRECT LINKER FORWARDING
#include <windows.h>
#include <thread>
#include <string>
#include "logger/logger.hpp"
#include "loader/loader.hpp"

namespace {
    // The host process' current working directory is not reliable (it may
    // not match the game's install folder), so the log is placed next to
    // this DLL itself instead of using a path relative to the CWD.
    std::string moduleLogPath(HMODULE hModule) {
        char path[MAX_PATH];
        DWORD len = GetModuleFileNameA(hModule, path, MAX_PATH);
        std::string dir(path, len);

        auto pos = dir.find_last_of("\\/");
        dir.resize(pos != std::string::npos ? pos + 1 : 0);

        return dir + "CrabeLoader.log";
    }
}

void init_logger(HMODULE hModule) {
    Logger& logger = Logger::getInstance();
    logger.setLogFile(moduleLogPath(hModule));
    logger.setLogLevel(LogLevel::DEBUG);
    logger.info("Logger initialized.");
}

void InitMain() {
    Loader::get().initialize();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        init_logger(hModule);
        Logger::getInstance().info("CrabeLoader DLL loaded.");
        std::thread(InitMain).detach();
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        Logger::getInstance().info("CrabeLoader DLL unloaded.");
    }
    return TRUE;
}

int main()
{
    DllMain(GetModuleHandle(NULL), DLL_PROCESS_ATTACH, NULL);
    Logger::getInstance().info("CrabeLoader started.");

    return 0;
}