/*
** CrabeLoader
** File description:
** Declares how the dinput8.dll bootstrap starts CrabeLoader.asi: load it, then call its export.
** A missing, broken or crashing CrabeLoader.asi is reported and the game starts without it.
** Knows nothing of when it runs; bootstrap_main.cpp calls it at the game's entry point.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_BOOTSTRAP_LOAD_LOADER_HPP_
#define CRABELOADER_BOOTSTRAP_LOAD_LOADER_HPP_

#include <cstdint>
#include <filesystem>
#include <string>

namespace crabe::bootstrap {

    constexpr const wchar_t* kLoaderFileName = L"CrabeLoader.asi";
    constexpr const char* kLoaderBootstrapExport = "CrabeLoaderBootstrap";

    enum class LoaderStart : std::uint8_t {
        Started,
        StartedWithoutExport,
        Missing,
        Failed,
        Crashed,
    };

    struct LoaderStartResult {
        LoaderStart outcome{LoaderStart::Missing};
        unsigned long code{0};
    };

    /// Loads the loader and calls its CrabeLoaderBootstrap export, both under SEH.
    /// Its own folder is searched first for the DLLs it needs. Never throws.
    [[nodiscard]] LoaderStartResult startLoader(const std::filesystem::path& loaderFile) noexcept;

    /// Returns one log line for a start that went wrong, or an empty string for Started.
    [[nodiscard]] std::string describe(const LoaderStartResult& result);

} // namespace crabe::bootstrap

#endif /* !CRABELOADER_BOOTSTRAP_LOAD_LOADER_HPP_ */
