/*
** CrabeLoader
** File description:
** Declares the installer's engine: look at a game folder, then put the proxy DLL in place.
** Every step is a rename or a new file, never a delete of a foreign file, and it rolls back.
** Shows no window and picks no folder; installer/main.cpp does.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INSTALLER_GAME_INSTALLER_HPP_
#define CRABELOADER_INSTALLER_GAME_INSTALLER_HPP_

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "installer/install_plan.hpp"

namespace crabe::installer {

    enum class InstallErrorCode : std::uint8_t {
        NotAGameFolder,
        ProxyMissing,
        ProxyUnrecognized,
        PayloadInvalid,
        FileInUse,
        AccessDenied,
        IoFailure,
    };

    struct InstallError {
        InstallErrorCode code{InstallErrorCode::IoFailure};
        std::string detail;
    };

    struct InstallReport {
        Plan performed{Plan::FreshInstall};
        bool modsFolderCreated{false};
        bool readmeCreated{false};
    };

    class GameInstaller final {
    public:
        explicit GameInstaller(std::filesystem::path gameRoot);
        GameInstaller(std::filesystem::path gameRoot, std::vector<std::string> knownOriginalSha256);

        [[nodiscard]] const std::filesystem::path& gameRoot() const noexcept { return _gameRoot; }

        [[nodiscard]] FolderFacts inspect() const;

        // Applies planInstall() to the folder. A refusal changes nothing on disk, and a failure
        // part-way restores the folder to how it was found.
        [[nodiscard]] std::expected<InstallReport, InstallError> install(std::span<const std::byte> payload) const;

    private:
        [[nodiscard]] bool isKnownOriginal(std::string_view sha256) const noexcept;

        std::filesystem::path _gameRoot;
        std::vector<std::string> _knownOriginals;
    };

    // The text written to mods/LISEZMOI.txt, in French then English.
    [[nodiscard]] std::string_view modsReadmeText() noexcept;

} // namespace crabe::installer

#endif /* !CRABELOADER_INSTALLER_GAME_INSTALLER_HPP_ */
