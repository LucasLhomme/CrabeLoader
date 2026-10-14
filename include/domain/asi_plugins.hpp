/*
** CrabeLoader
** File description:
** Declares which .asi plugins a game folder offers, in load order, and why one is skipped.
** Tells a 32-bit DLL from a 64-bit one or a non-DLL by its PE header, before Windows does.
** Loads nothing; infrastructure/asi_loader.hpp does.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_DOMAIN_ASI_PLUGINS_HPP_
#define CRABELOADER_DOMAIN_ASI_PLUGINS_HPP_

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include "domain/config.hpp"

namespace crabe::domain {

    struct AsiCandidate {
        std::filesystem::path path;
        std::string source;
        std::string skipReason;

        [[nodiscard]] bool shouldLoad() const noexcept { return skipReason.empty(); }
    };

    enum class PluginBinary : std::uint8_t {
        Win32Dll,
        NotWin32,
        NotADll,
        NotPortableExecutable,
    };

    /// Lists every .asi plugin of a game folder: the folder itself, scripts/, plugins/, then each mod folder.
    /// Files load in that order, sorted by name inside a folder; a mod's are skipped when its profile disables it.
    /// [plugins].asi = false or a name in [plugins].skip keeps the file listed with the reason it is skipped.
    [[nodiscard]] std::vector<AsiCandidate> discoverAsiPlugins(const std::filesystem::path& gameRoot,
                                                               const Config& config);

    /// Classifies the first bytes of a file from its PE header, so a 64-bit plugin gets a clear refusal.
    /// The bytes must reach the end of the COFF file header; the first 4 KiB of a file always do.
    [[nodiscard]] PluginBinary classifyPluginBinary(std::span<const std::byte> header) noexcept;

    /// Returns why a plugin of this kind cannot load into a 32-bit game, or an empty string for a 32-bit DLL.
    [[nodiscard]] std::string pluginBinaryProblem(PluginBinary kind);

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_ASI_PLUGINS_HPP_ */
