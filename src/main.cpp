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
    std::string moduleLogPath(HMODULE hModule) {
        char path[MAX_PATH];
        DWORD len = GetModuleFileNameA(hModule, path, MAX_PATH);
        std::string dir(path, len);

        auto pos = dir.find_last_of("\\/");
        dir.resize(pos != std::string::npos ? pos + 1 : 0);

        return dir + "loader.log";
    }
    long WINAPI crashFilter(EXCEPTION_POINTERS* info) {
        Logger::getInstance().error(
            "Unhandled exception 0x{:X} at address 0x{:X}",
            static_cast<unsigned long>(info->ExceptionRecord->ExceptionCode),
            reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress)
        );
        return EXCEPTION_EXECUTE_HANDLER;
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

bool APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        init_logger(hModule);
        SetUnhandledExceptionFilter(crashFilter);
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