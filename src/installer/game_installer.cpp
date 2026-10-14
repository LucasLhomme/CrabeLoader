/*
** CrabeLoader
** File description:
** Carries out an install plan on a game folder with exclusive writes and no-replace renames.
** CrabeLoader's own files are retired beside themselves first, so every step can be undone.
** Chooses nothing: install_plan.cpp decides, and this file does exactly that.
**
** Authors: @LucasLhomme
*/

#include "installer/game_installer.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <iterator>
#include <memory>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include <windows.h>

#include "installer/sha256.hpp"
#include "shared/loader_marker.hpp"

namespace crabe::installer {

    namespace {

        constexpr std::size_t kMinimumLoaderBytes = 64 * 1024;
        constexpr std::size_t kMinimumBootstrapBytes = 4 * 1024;
        constexpr std::uintmax_t kLargestInspectedFile = 64ull * 1024 * 1024;
        constexpr std::string_view kUtf8Bom = "\xEF\xBB\xBF";

        struct HandleCloser {
            void operator()(HANDLE handle) const noexcept
            {
                if (handle && handle != INVALID_HANDLE_VALUE)
                    CloseHandle(handle);
            }
        };

        using FileHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleCloser>;

        /// Maps a Windows error to the installer's own codes, so the window can say what to do.
        [[nodiscard]] InstallErrorCode classify(DWORD winError) noexcept
        {
            switch (winError) {
            case ERROR_SHARING_VIOLATION:
            case ERROR_LOCK_VIOLATION:
                return InstallErrorCode::FileInUse;
            case ERROR_ACCESS_DENIED:
                return InstallErrorCode::AccessDenied;
            default:
                return InstallErrorCode::IoFailure;
            }
        }

        /// Builds the error of a failed step from the Windows error code.
        [[nodiscard]] InstallError fromWin32(std::string_view step, DWORD winError)
        {
            return InstallError{classify(winError), std::format("{} (Windows error {})", step, winError)};
        }

        /// Reports whether a path exists, treating an error as absence.
        [[nodiscard]] bool pathExists(const std::filesystem::path& path) noexcept
        {
            std::error_code ignored;
            return std::filesystem::exists(path, ignored);
        }

        /// Removes a file the installer itself wrote or retired; a failure is ignored.
        void discardFile(const std::filesystem::path& path) noexcept
        {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }

        /// Tells whether a file is missing, is CrabeLoader, is its bootstrap, or is something else.
        /// A file too large or unreadable is something else, so it is never replaced. The loader is checked
        /// first: CrabeLoader.asi quotes the bootstrap description too, in the code that recognises it.
        [[nodiscard]] FileKind kindOf(const std::filesystem::path& path)
        {
            std::error_code error;
            if (!std::filesystem::exists(path, error))
                return FileKind::Missing;
            const std::uintmax_t size = std::filesystem::file_size(path, error);
            if (error || size > kLargestInspectedFile)
                return FileKind::Other;
            std::ifstream stream(path, std::ios::binary);
            const std::vector<char> bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
            if (!stream.eof() && stream.fail())
                return FileKind::Other;
            if (crabe::shared::containsLoaderMarker(bytes.data(), bytes.size()))
                return FileKind::Loader;
            if (crabe::shared::containsBootstrapMarker(bytes.data(), bytes.size()))
                return FileKind::Bootstrap;
            return FileKind::Other;
        }

        /// Writes a file that must not exist yet, flushed to disk.
        [[nodiscard]] std::expected<void, InstallError> writeNewFile(const std::filesystem::path& path,
                                                                     std::span<const std::byte> bytes)
        {
            FileHandle file(CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                        FILE_ATTRIBUTE_NORMAL, nullptr));
            if (file.get() == INVALID_HANDLE_VALUE)
                return std::unexpected(fromWin32("create " + path.filename().string(), GetLastError()));

            std::size_t written = 0;
            while (written < bytes.size()) {
                const DWORD chunk = static_cast<DWORD>(std::min<std::size_t>(bytes.size() - written, 1u << 20));
                DWORD done = 0;
                if (!WriteFile(file.get(), bytes.data() + written, chunk, &done, nullptr) || done == 0)
                    return std::unexpected(fromWin32("write " + path.filename().string(), GetLastError()));
                written += done;
            }
            if (!FlushFileBuffers(file.get()))
                return std::unexpected(fromWin32("flush " + path.filename().string(), GetLastError()));
            return {};
        }

        /// Renames a file, failing if the target already exists.
        [[nodiscard]] std::expected<void, InstallError> moveWithoutReplacing(const std::filesystem::path& from,
                                                                             const std::filesystem::path& to)
        {
            if (!MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_WRITE_THROUGH))
                return std::unexpected(fromWin32(std::format("rename {} to {}", from.filename().string(),
                                                             to.filename().string()), GetLastError()));
            return {};
        }

        /// Refuses a bundled file that is too small, not a Windows executable, or not the expected kind.
        [[nodiscard]] std::expected<void, InstallError> checkPayload(std::span<const std::byte> payload,
                                                                     std::string_view name, std::size_t minimum,
                                                                     FileKind expected)
        {
            const auto refuse = [&](std::string_view why) {
                return std::unexpected(InstallError{InstallErrorCode::PayloadInvalid,
                                                    std::format("the bundled {} {}", name, why)});
            };
            if (payload.size() < minimum)
                return refuse("is too small");
            if (payload[0] != std::byte{'M'} || payload[1] != std::byte{'Z'})
                return refuse("is not a DLL");
            const bool isLoader = crabe::shared::containsLoaderMarker(payload.data(), payload.size());
            const bool isBootstrap = crabe::shared::containsBootstrapMarker(payload.data(), payload.size());
            if (expected == FileKind::Loader && !isLoader)
                return refuse("is not CrabeLoader");
            if (expected == FileKind::Bootstrap && (!isBootstrap || isLoader))
                return refuse("is not the CrabeLoader bootstrap");
            return {};
        }

        struct Placement {
            std::filesystem::path target;
            std::filesystem::path retired;
            bool hadPrevious{false};
        };

        /// Moves a staged file to its final name. CrabeLoader's previous copy is retired beside it first,
        /// so undoPlacement() can bring it back; if the move fails, the folder is left as it was.
        [[nodiscard]] std::expected<Placement, InstallError> place(const std::filesystem::path& staging,
                                                                   const std::filesystem::path& target,
                                                                   bool replacesOwnFile)
        {
            Placement placement{target, std::filesystem::path(target) += kRetiredSuffix, replacesOwnFile};
            if (replacesOwnFile) {
                discardFile(placement.retired);
                if (auto retired = moveWithoutReplacing(target, placement.retired); !retired)
                    return std::unexpected(retired.error());
            }
            if (auto moved = moveWithoutReplacing(staging, target); !moved) {
                InstallError error = moved.error();
                if (replacesOwnFile) {
                    if (auto back = moveWithoutReplacing(placement.retired, target); !back)
                        error.detail += std::format("; ALSO could not put back the old {}: {}",
                                                    target.filename().string(), back.error().detail);
                }
                return std::unexpected(std::move(error));
            }
            return placement;
        }

        /// Undoes a placement: the new file goes, and the previous copy comes back if there was one.
        void undoPlacement(const Placement& placement, InstallError& error)
        {
            discardFile(placement.target);
            if (!placement.hadPrevious)
                return;
            if (auto back = moveWithoutReplacing(placement.retired, placement.target); !back)
                error.detail += std::format("; ALSO could not put back the old {}: {}",
                                            placement.target.filename().string(), back.error().detail);
        }

        /// Writes a bundled file beside its final name and checks it reads back identical.
        [[nodiscard]] std::expected<void, InstallError> stagePayload(const std::filesystem::path& staging,
                                                                     std::span<const std::byte> payload)
        {
            const auto expected = sha256Hex(payload);
            if (!expected)
                return std::unexpected(InstallError{InstallErrorCode::PayloadInvalid,
                                                    "a bundled file could not be hashed"});
            discardFile(staging);
            if (auto written = writeNewFile(staging, payload); !written)
                return written;

            const auto actual = sha256FileHex(staging);
            if (!actual || *actual != *expected) {
                discardFile(staging);
                return std::unexpected(InstallError{InstallErrorCode::IoFailure,
                                                    std::format("the written {} does not match the bundled file",
                                                                staging.filename().string())});
            }
            return {};
        }

        /// Puts the game's own bink2w32.dll back from bink2w32_orig.dll, retiring a 1.x CrabeLoader first.
        /// If the original cannot be moved, the retired CrabeLoader goes back, so the folder is as it was.
        [[nodiscard]] std::expected<void, InstallError> restoreLegacyOriginal(const std::filesystem::path& root,
                                                                              FileKind legacyProxy)
        {
            const std::filesystem::path proxy = root / kLegacyProxyName;
            const std::filesystem::path backup = root / kLegacyBackupName;
            const std::filesystem::path retired = root / kLegacyRetiredName;

            if (legacyProxy == FileKind::Loader) {
                discardFile(retired);
                if (auto moved = moveWithoutReplacing(proxy, retired); !moved)
                    return moved;
            }
            if (auto restored = moveWithoutReplacing(backup, proxy); !restored) {
                if (legacyProxy == FileKind::Loader) {
                    if (auto back = moveWithoutReplacing(retired, proxy); !back)
                        restored.error().detail += std::format("; ALSO could not put back the old {}: {}",
                                                               kLegacyProxyName, back.error().detail);
                }
                return restored;
            }
            discardFile(retired);
            return {};
        }

        /// Creates the mods folder and its readme when they are missing; never overwrites either.
        void writeModsEnvironment(const std::filesystem::path& root, InstallReport& report)
        {
            const std::filesystem::path mods = root / kModsFolderName;
            std::error_code error;
            if (!std::filesystem::exists(mods, error)) {
                report.modsFolderCreated = std::filesystem::create_directory(mods, error) && !error;
            }

            const std::filesystem::path readme = mods / kReadmeName;
            if (!pathExists(readme) && pathExists(mods)) {
                const std::string_view text = modsReadmeText();
                std::vector<std::byte> bytes;
                bytes.reserve(kUtf8Bom.size() + text.size());
                for (const char c : kUtf8Bom)
                    bytes.push_back(static_cast<std::byte>(c));
                for (const char c : text)
                    bytes.push_back(static_cast<std::byte>(c));
                report.readmeCreated = writeNewFile(readme, bytes).has_value();
            }
        }

    } // namespace

    GameInstaller::GameInstaller(std::filesystem::path gameRoot) : _gameRoot(std::move(gameRoot))
    {
    }

    FolderFacts GameInstaller::inspect() const
    {
        FolderFacts facts;
        facts.hasGameExecutable = pathExists(_gameRoot / kGameExecutable);
        facts.loader = kindOf(_gameRoot / kLoaderName);
        facts.bootstrap = kindOf(_gameRoot / kBootstrapName);
        facts.legacyProxy = kindOf(_gameRoot / kLegacyProxyName);
        facts.hasLegacyBackup = pathExists(_gameRoot / kLegacyBackupName);
        return facts;
    }

    std::expected<InstallReport, InstallError> GameInstaller::install(const InstallPayload& payload) const
    {
        if (auto valid = checkPayload(payload.loader, kLoaderName, kMinimumLoaderBytes, FileKind::Loader); !valid)
            return std::unexpected(valid.error());
        if (auto valid = checkPayload(payload.bootstrap, kBootstrapName, kMinimumBootstrapBytes, FileKind::Bootstrap);
            !valid)
            return std::unexpected(valid.error());

        const FolderFacts facts = inspect();
        const Plan plan = planInstall(facts);
        switch (plan) {
        case Plan::NotAGameFolder:
            return std::unexpected(InstallError{InstallErrorCode::NotAGameFolder, std::string(describe(plan))});
        case Plan::BootstrapSlotTaken:
            return std::unexpected(InstallError{InstallErrorCode::BootstrapSlotTaken, std::string(describe(plan))});
        case Plan::LoaderSlotTaken:
            return std::unexpected(InstallError{InstallErrorCode::LoaderSlotTaken, std::string(describe(plan))});
        case Plan::LegacyOriginalMissing:
            return std::unexpected(InstallError{InstallErrorCode::LegacyOriginalMissing, std::string(describe(plan))});
        case Plan::FreshInstall:
        case Plan::Update:
            break;
        }

        const std::filesystem::path loader = _gameRoot / kLoaderName;
        const std::filesystem::path bootstrap = _gameRoot / kBootstrapName;
        const std::filesystem::path loaderStaging = std::filesystem::path(loader) += kStagingSuffix;
        const std::filesystem::path bootstrapStaging = std::filesystem::path(bootstrap) += kStagingSuffix;
        const auto discardStaging = [&] {
            discardFile(loaderStaging);
            discardFile(bootstrapStaging);
        };

        if (auto staged = stagePayload(loaderStaging, payload.loader); !staged)
            return std::unexpected(staged.error());
        if (auto staged = stagePayload(bootstrapStaging, payload.bootstrap); !staged) {
            discardStaging();
            return std::unexpected(staged.error());
        }

        const auto loaderPlaced = place(loaderStaging, loader, facts.loader == FileKind::Loader);
        if (!loaderPlaced) {
            discardStaging();
            return std::unexpected(loaderPlaced.error());
        }
        const auto bootstrapPlaced = place(bootstrapStaging, bootstrap, facts.bootstrap != FileKind::Missing);
        if (!bootstrapPlaced) {
            InstallError error = bootstrapPlaced.error();
            undoPlacement(*loaderPlaced, error);
            discardStaging();
            return std::unexpected(std::move(error));
        }

        InstallReport report;
        report.performed = plan;
        if (restoresLegacyOriginal(facts)) {
            if (auto restored = restoreLegacyOriginal(_gameRoot, facts.legacyProxy); !restored) {
                InstallError error = restored.error();
                undoPlacement(*bootstrapPlaced, error);
                undoPlacement(*loaderPlaced, error);
                return std::unexpected(std::move(error));
            }
            report.legacyOriginalRestored = true;
        }

        for (const Placement* placement : {&*loaderPlaced, &*bootstrapPlaced}) {
            if (placement->hadPrevious)
                discardFile(placement->retired);
        }
        writeModsEnvironment(_gameRoot, report);
        return report;
    }

} // namespace crabe::installer
