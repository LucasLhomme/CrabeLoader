/*
** CrabeLoader
** File description:
** Declares how the installer guesses where Steam put the game, so the folder dialog starts there.
** The guess only pre-fills a question; the player always confirms or picks another folder.
** Parses Steam's libraryfolders.vdf as text and installs nothing.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INSTALLER_STEAM_LOCATOR_HPP_
#define CRABELOADER_INSTALLER_STEAM_LOCATOR_HPP_

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

namespace crabe::installer {

    constexpr std::string_view kSteamGameFolderName = "Disney Infinity 3.0 Gold Edition";

    // Every "path" value of a libraryfolders.vdf, with the file's \\ escapes undone.
    [[nodiscard]] std::vector<std::filesystem::path> parseLibraryFolders(std::string_view vdfText);

    // The first library that holds steamapps/common/<game folder>/DisneyInfinity3.exe.
    [[nodiscard]] std::optional<std::filesystem::path> findGameInLibraries(
        const std::vector<std::filesystem::path>& libraries);

    // Reads the Steam install path from the registry, then its libraries; nullopt when not found.
    [[nodiscard]] std::optional<std::filesystem::path> locateSteamGame();

} // namespace crabe::installer

#endif /* !CRABELOADER_INSTALLER_STEAM_LOCATOR_HPP_ */
