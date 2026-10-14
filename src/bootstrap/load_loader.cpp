/*
** CrabeLoader
** File description:
** Loads CrabeLoader.asi and calls its CrabeLoaderBootstrap export, each under its own SEH guard.
** The export lets CrabeLoader load .asi plugins at once: before game code, outside the loader lock.
** Writes nothing; bootstrap_main.cpp logs what describe() returns.
**
** Authors: @LucasLhomme
*/

#include "bootstrap/load_loader.hpp"

#include <windows.h>

#include <format>

namespace crabe::bootstrap {

    namespace {

        using BootstrapFn = void(__cdecl*)();

        struct GuardedLoad {
            HMODULE module;
            DWORD lastError;
            DWORD exceptionCode;
        };

        /// Loads a DLL under SEH, so a loader whose DllMain faults cannot take the game down.
        GuardedLoad loadGuarded(const wchar_t* path) noexcept
        {
            GuardedLoad result{nullptr, ERROR_SUCCESS, 0};
            __try {
                result.module = LoadLibraryExW(path, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
                if (!result.module)
                    result.lastError = GetLastError();
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                result.exceptionCode = GetExceptionCode();
            }
            return result;
        }

        /// Calls the bootstrap export under SEH; returns the exception code it raised, or 0.
        DWORD callGuarded(BootstrapFn bootstrap) noexcept
        {
            __try {
                bootstrap();
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }
            return 0;
        }

    } // namespace

    LoaderStartResult startLoader(const std::filesystem::path& loaderFile) noexcept
    {
        std::error_code error;
        if (!std::filesystem::is_regular_file(loaderFile, error))
            return {LoaderStart::Missing, 0};

        const GuardedLoad load = loadGuarded(loaderFile.c_str());
        if (load.exceptionCode != 0)
            return {LoaderStart::Crashed, load.exceptionCode};
        if (!load.module)
            return {LoaderStart::Failed, load.lastError};

        const auto bootstrap = reinterpret_cast<BootstrapFn>(GetProcAddress(load.module, kLoaderBootstrapExport));
        if (!bootstrap)
            return {LoaderStart::StartedWithoutExport, 0};
        if (const DWORD exception = callGuarded(bootstrap); exception != 0)
            return {LoaderStart::Crashed, exception};
        return {LoaderStart::Started, 0};
    }

    std::string describe(const LoaderStartResult& result)
    {
        switch (result.outcome) {
            case LoaderStart::Started:
                return {};
            case LoaderStart::StartedWithoutExport:
                return "CrabeLoader.asi has no CrabeLoaderBootstrap export (an older build?); "
                       "its .asi plugins load from its own thread";
            case LoaderStart::Missing:
                return "CrabeLoader.asi is not next to the game; the game starts without CrabeLoader";
            case LoaderStart::Failed:
                return std::format("CrabeLoader.asi could not be loaded (Windows error {}); "
                                   "the game starts without CrabeLoader", result.code);
            case LoaderStart::Crashed:
                return std::format("CrabeLoader.asi raised exception 0x{:08X} while starting; "
                                   "the game starts anyway", result.code);
        }
        return "unknown outcome";
    }

} // namespace crabe::bootstrap
