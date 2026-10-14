/*
** CrabeLoader
** File description:
** Declares the installer's engine: inspect a game folder, then put CrabeLoader's files in place.
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

#include "installer/install_plan.hpp"

namespace crabe::installer {

    enum class InstallErrorCode : std::uint8_t {
        NotAGameFolder,
        BootstrapSlotTaken,
        LoaderSlotTaken,
        LegacyOriginalMissing,
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
        bool legacyOriginalRestored{false};
        bool modsFolderCreated{false};
        bool readmeCreated{false};
    };

    struct InstallPayload {
        std::span<const std::byte> loader;
        std::span<const std::byte> bootstrap;
    };

    class GameInstaller final {
    public:
        explicit GameInstaller(std::filesystem::path gameRoot);

        [[nodiscard]] const std::filesystem::path& gameRoot() const noexcept { return _gameRoot; }

        /// Reads the facts planInstall() decides from: a file is CrabeLoader or its bootstrap by its description.
        [[nodiscard]] FolderFacts inspect() const;

        /// Applies planInstall() to the folder, then puts a 1.x install's kept Bink DLL back when there is one.
        /// A refusal changes nothing on disk, and a failure part-way restores the folder to how it was found.
        [[nodiscard]] std::expected<InstallReport, InstallError> install(const InstallPayload& payload) const;

    private:
        std::filesystem::path _gameRoot;
    };

    /// Returns the text written to mods/README.txt, in English then French.
    [[nodiscard]] std::string_view modsReadmeText() noexcept;

} // namespace crabe::installer

#endif /* !CRABELOADER_INSTALLER_GAME_INSTALLER_HPP_ */
