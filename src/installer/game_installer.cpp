/*
** CrabeLoader
** File description:
** Carries out an install plan on a game folder with exclusive writes and no-replace renames.
** The only overwrites are of CrabeLoader's own files; a game file is never opened for writing.
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

        constexpr std::size_t kMinimumPayloadBytes = 64 * 1024;
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

        /// Tells whether a file is missing, is a CrabeLoader DLL, or is something else.
        /// A file too large or unreadable is something else, so it is never replaced.
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
            return crabe::shared::containsLoaderMarker(bytes.data(), bytes.size()) ? FileKind::Loader : FileKind::Other;
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

        /// Renames a file over an existing one; only ever used on CrabeLoader's own files.
        [[nodiscard]] std::expected<void, InstallError> moveReplacing(const std::filesystem::path& from,
                                                                      const std::filesystem::path& to)
        {
            if (!MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                return std::unexpected(fromWin32(std::format("replace {} with {}", to.filename().string(),
                                                             from.filename().string()), GetLastError()));
            return {};
        }

        /// Refuses a payload that is too small, not a Windows executable, or not a CrabeLoader DLL.
        [[nodiscard]] std::expected<void, InstallError> checkPayload(std::span<const std::byte> payload)
        {
            if (payload.size() < kMinimumPayloadBytes)
                return std::unexpected(InstallError{InstallErrorCode::PayloadInvalid, "the bundled DLL is too small"});
            if (payload[0] != std::byte{'M'} || payload[1] != std::byte{'Z'})
                return std::unexpected(InstallError{InstallErrorCode::PayloadInvalid, "the bundled file is not a DLL"});
            if (!crabe::shared::containsLoaderMarker(payload.data(), payload.size()))
                return std::unexpected(InstallError{InstallErrorCode::PayloadInvalid,
                                                    "the bundled DLL is not CrabeLoader"});
            return {};
        }

        /// Writes the payload beside its final name and checks it reads back identical.
        [[nodiscard]] std::expected<void, InstallError> stagePayload(const std::filesystem::path& staging,
                                                                     std::span<const std::byte> payload,
                                                                     const std::string& expectedSha256)
        {
            discardFile(staging);
            if (auto written = writeNewFile(staging, payload); !written)
                return written;

            const auto actual = sha256FileHex(staging);
            if (!actual || *actual != expectedSha256) {
                discardFile(staging);
                return std::unexpected(InstallError{InstallErrorCode::IoFailure,
                                                    "the written file does not match the bundled DLL"});
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
        facts.legacyProxy = kindOf(_gameRoot / kLegacyProxyName);
        facts.hasLegacyBackup = pathExists(_gameRoot / kLegacyBackupName);
        return facts;
    }

    std::expected<InstallReport, InstallError> GameInstaller::install(std::span<const std::byte> payload) const
    {
        if (auto valid = checkPayload(payload); !valid)
            return std::unexpected(valid.error());
        const auto payloadHash = sha256Hex(payload);
        if (!payloadHash)
            return std::unexpected(InstallError{InstallErrorCode::PayloadInvalid, "the bundled DLL could not be hashed"});

        const FolderFacts facts = inspect();
        const Plan plan = planInstall(facts);
        switch (plan) {
        case Plan::NotAGameFolder:
            return std::unexpected(InstallError{InstallErrorCode::NotAGameFolder, std::string(describe(plan))});
        case Plan::LoaderSlotTaken:
            return std::unexpected(InstallError{InstallErrorCode::LoaderSlotTaken, std::string(describe(plan))});
        case Plan::LegacyOriginalMissing:
            return std::unexpected(InstallError{InstallErrorCode::LegacyOriginalMissing, std::string(describe(plan))});
        case Plan::FreshInstall:
        case Plan::Update:
            break;
        }

        const std::filesystem::path loader = _gameRoot / kLoaderName;
        const std::filesystem::path staging = _gameRoot / kStagingName;

        if (auto staged = stagePayload(staging, payload, *payloadHash); !staged)
            return std::unexpected(staged.error());

        const auto placed = plan == Plan::Update ? moveReplacing(staging, loader) : moveWithoutReplacing(staging, loader);
        if (!placed) {
            discardFile(staging);
            return std::unexpected(placed.error());
        }

        InstallReport report;
        report.performed = plan;
        if (restoresLegacyOriginal(facts)) {
            if (auto restored = restoreLegacyOriginal(_gameRoot, facts.legacyProxy); !restored) {
                InstallError error = restored.error();
                if (plan == Plan::FreshInstall)
                    discardFile(loader);
                else
                    error.detail += "; the new dinput8.dll is in place, the old bink2w32.dll was left as it was";
                return std::unexpected(std::move(error));
            }
            report.legacyOriginalRestored = true;
        }

        writeModsEnvironment(_gameRoot, report);
        return report;
    }

} // namespace crabe::installer
