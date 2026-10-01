/*
** CrabeLoader
** File description:
** Carries out an install plan on a game folder with exclusive writes and no-replace renames.
** The only overwrite is the update of our own DLL; the original is never opened for writing.
** Chooses nothing: install_plan.cpp decides, and this file does exactly that.
**
** Authors: @LucasLhomme
*/

#include "installer/game_installer.hpp"

#include <algorithm>
#include <format>
#include <memory>
#include <system_error>
#include <type_traits>
#include <utility>

#include <windows.h>

#include "installer/sha256.hpp"

namespace crabe::installer {

    namespace {

        constexpr std::size_t kMinimumPayloadBytes = 64 * 1024;
        constexpr std::string_view kUtf8Bom = "\xEF\xBB\xBF";

        struct HandleCloser {
            void operator()(HANDLE handle) const noexcept
            {
                if (handle && handle != INVALID_HANDLE_VALUE)
                    CloseHandle(handle);
            }
        };

        using FileHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleCloser>;

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

        [[nodiscard]] InstallError fromWin32(std::string_view step, DWORD winError)
        {
            return InstallError{classify(winError), std::format("{} (Windows error {})", step, winError)};
        }

        [[nodiscard]] bool pathExists(const std::filesystem::path& path) noexcept
        {
            std::error_code ignored;
            return std::filesystem::exists(path, ignored);
        }

        void discardFile(const std::filesystem::path& path) noexcept
        {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }

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

        [[nodiscard]] std::expected<void, InstallError> moveWithoutReplacing(const std::filesystem::path& from,
                                                                             const std::filesystem::path& to)
        {
            if (!MoveFileExW(from.c_str(), to.c_str(), 0))
                return std::unexpected(fromWin32(std::format("rename {} to {}", from.filename().string(),
                                                             to.filename().string()), GetLastError()));
            return {};
        }

        [[nodiscard]] std::expected<void, InstallError> moveReplacing(const std::filesystem::path& from,
                                                                      const std::filesystem::path& to)
        {
            if (!MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                return std::unexpected(fromWin32(std::format("replace {} with {}", to.filename().string(),
                                                             from.filename().string()), GetLastError()));
            return {};
        }

        [[nodiscard]] std::expected<void, InstallError> checkPayload(std::span<const std::byte> payload,
                                                                     const std::string& sha256,
                                                                     bool isKnownOriginal)
        {
            if (payload.size() < kMinimumPayloadBytes)
                return std::unexpected(InstallError{InstallErrorCode::PayloadInvalid, "the bundled DLL is too small"});
            if (payload[0] != std::byte{'M'} || payload[1] != std::byte{'Z'})
                return std::unexpected(InstallError{InstallErrorCode::PayloadInvalid, "the bundled file is not a DLL"});
            if (isKnownOriginal)
                return std::unexpected(InstallError{InstallErrorCode::PayloadInvalid,
                                                    std::format("the bundled DLL is the game's own ({})", sha256)});
            return {};
        }

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

    GameInstaller::GameInstaller(std::filesystem::path gameRoot)
        : _gameRoot(std::move(gameRoot)),
          _knownOriginals(kKnownOriginalSha256.begin(), kKnownOriginalSha256.end())
    {
    }

    GameInstaller::GameInstaller(std::filesystem::path gameRoot, std::vector<std::string> knownOriginalSha256)
        : _gameRoot(std::move(gameRoot)), _knownOriginals(std::move(knownOriginalSha256))
    {
    }

    bool GameInstaller::isKnownOriginal(std::string_view sha256) const noexcept
    {
        return std::ranges::find(_knownOriginals, sha256) != _knownOriginals.end();
    }

    FolderFacts GameInstaller::inspect() const
    {
        FolderFacts facts;
        facts.hasGameExecutable = pathExists(_gameRoot / kGameExecutable);
        facts.hasBackup = pathExists(_gameRoot / kBackupName);

        const std::filesystem::path proxy = _gameRoot / kProxyName;
        if (!pathExists(proxy)) {
            facts.proxy = ProxySlot::Missing;
            return facts;
        }

        const auto hash = sha256FileHex(proxy);
        facts.proxy = (hash && isKnownOriginal(*hash)) ? ProxySlot::KnownOriginal : ProxySlot::Other;
        return facts;
    }

    std::expected<InstallReport, InstallError> GameInstaller::install(std::span<const std::byte> payload) const
    {
        const auto payloadHash = sha256Hex(payload);
        if (!payloadHash)
            return std::unexpected(InstallError{InstallErrorCode::PayloadInvalid, "the bundled DLL could not be hashed"});
        if (auto valid = checkPayload(payload, *payloadHash, isKnownOriginal(*payloadHash)); !valid)
            return std::unexpected(valid.error());

        const Plan plan = planInstall(inspect());
        switch (plan) {
        case Plan::NotAGameFolder:
            return std::unexpected(InstallError{InstallErrorCode::NotAGameFolder, std::string(describe(plan))});
        case Plan::ProxyMissing:
            return std::unexpected(InstallError{InstallErrorCode::ProxyMissing, std::string(describe(plan))});
        case Plan::ProxyUnrecognized:
            return std::unexpected(InstallError{InstallErrorCode::ProxyUnrecognized, std::string(describe(plan))});
        case Plan::FreshInstall:
        case Plan::Update:
            break;
        }

        const std::filesystem::path proxy = _gameRoot / kProxyName;
        const std::filesystem::path backup = _gameRoot / kBackupName;
        const std::filesystem::path staging = _gameRoot / kStagingName;

        if (auto staged = stagePayload(staging, payload, *payloadHash); !staged)
            return std::unexpected(staged.error());

        if (plan == Plan::Update) {
            if (auto replaced = moveReplacing(staging, proxy); !replaced) {
                discardFile(staging);
                return std::unexpected(replaced.error());
            }
        } else {
            if (auto kept = moveWithoutReplacing(proxy, backup); !kept) {
                discardFile(staging);
                return std::unexpected(kept.error());
            }
            if (auto placed = moveWithoutReplacing(staging, proxy); !placed) {
                const auto restored = moveWithoutReplacing(backup, proxy);
                discardFile(staging);
                InstallError error = placed.error();
                if (!restored)
                    error.detail += std::format("; ALSO could not restore the original: {}", restored.error().detail);
                return std::unexpected(std::move(error));
            }
        }

        InstallReport report;
        report.performed = plan;
        writeModsEnvironment(_gameRoot, report);
        return report;
    }

} // namespace crabe::installer
