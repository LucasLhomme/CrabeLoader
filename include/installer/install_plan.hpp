/*
** CrabeLoader
** File description:
** Declares what the installer may do to a game folder, decided from four facts about it.
** CrabeLoader installs as dinput8.dll and never overwrites one it did not write, nor a game file.
** Touches no file; game_installer.hpp gathers the facts and carries the decision out.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INSTALLER_INSTALL_PLAN_HPP_
#define CRABELOADER_INSTALLER_INSTALL_PLAN_HPP_

#include <cstdint>
#include <string_view>

namespace crabe::installer {

    constexpr std::string_view kGameExecutable = "DisneyInfinity3.exe";
    constexpr std::string_view kLoaderName = "dinput8.dll";
    constexpr std::string_view kStagingName = "dinput8.dll.crabe-new";
    constexpr std::string_view kLegacyProxyName = "bink2w32.dll";
    constexpr std::string_view kLegacyBackupName = "bink2w32_orig.dll";
    constexpr std::string_view kLegacyRetiredName = "bink2w32.dll.crabe-old";
    constexpr std::string_view kModsFolderName = "mods";
    constexpr std::string_view kReadmeName = "README.txt";

    enum class FileKind : std::uint8_t {
        Missing,
        Loader,
        Other,
    };

    struct FolderFacts {
        bool hasGameExecutable{false};
        FileKind loader{FileKind::Missing};
        FileKind legacyProxy{FileKind::Missing};
        bool hasLegacyBackup{false};
    };

    enum class Plan : std::uint8_t {
        FreshInstall,
        Update,
        NotAGameFolder,
        LoaderSlotTaken,
        LegacyOriginalMissing,
    };

    [[nodiscard]] std::string_view describe(Plan plan) noexcept;

    /// Decides the install from the facts: a dinput8.dll that is not CrabeLoader is never replaced.
    /// A 1.x install (CrabeLoader as bink2w32.dll) is refused only when its kept original is gone,
    /// since the game cannot play its videos without it.
    [[nodiscard]] Plan planInstall(const FolderFacts& facts) noexcept;

    /// Reports whether the game's own bink2w32.dll, kept as bink2w32_orig.dll by a 1.x install, goes back in place.
    /// That happens when bink2w32.dll is CrabeLoader or missing; a game file there is left alone.
    [[nodiscard]] bool restoresLegacyOriginal(const FolderFacts& facts) noexcept;

} // namespace crabe::installer

#endif /* !CRABELOADER_INSTALLER_INSTALL_PLAN_HPP_ */
