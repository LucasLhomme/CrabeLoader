/*
** CrabeLoader
** File description:
** Loads .asi plugins with LoadLibraryExW under an SEH guard, after checking each is a 32-bit DLL.
** Every outcome is logged and kept, so Lua and the overlay can show what loaded and why not.
** Never unloads a plugin: native code cannot be taken back out of a running game safely.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/asi_loader.hpp"

#include <windows.h>

#include <array>
#include <cstddef>
#include <format>
#include <fstream>
#include <optional>

#include "domain/config.hpp"
#include "shared/logger.hpp"

namespace crabe::infrastructure {

    namespace {

        namespace fs = std::filesystem;

        constexpr std::size_t kHeaderBytes = 4096;

        struct GuardedLoad {
            HMODULE module;
            DWORD lastError;
            DWORD exceptionCode;
        };

        /// Reads the first 4 KiB of a file, fewer when it is shorter, nothing when it cannot be opened.
        std::optional<std::vector<std::byte>> readHeader(const fs::path& file)
        {
            std::ifstream stream(file, std::ios::binary);
            if (!stream)
                return std::nullopt;
            std::vector<std::byte> bytes(kHeaderBytes);
            stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            bytes.resize(static_cast<std::size_t>(stream.gcount()));
            return bytes;
        }

        /// Returns a path as UTF-8 for the log and Lua, which never throws whatever the file name holds.
        std::string utf8(const fs::path& path)
        {
            const std::u8string text = path.u8string();
            return std::string(text.begin(), text.end());
        }

        /// Describes a Win32 error code on one line, in English when Windows has the English text.
        std::string describeWin32Error(DWORD code)
        {
            std::array<char, 512> text{};
            const DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
            DWORD length = FormatMessageA(flags, nullptr, code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                                          text.data(), static_cast<DWORD>(text.size()), nullptr);
            if (length == 0)
                length = FormatMessageA(flags, nullptr, code, 0, text.data(), static_cast<DWORD>(text.size()), nullptr);
            std::string message(text.data(), length);
            while (!message.empty() && (message.back() == '\n' || message.back() == '\r' || message.back() == ' ' ||
                                        message.back() == '.'))
                message.pop_back();
            return message.empty() ? std::format("Win32 error {}", code) : std::format("{} (error {})", message, code);
        }

        /// Calls LoadLibraryExW under SEH, so a plugin whose DllMain faults is reported instead of ending the game.
        /// The plugin's own folder is searched first for the DLLs it depends on.
        GuardedLoad loadLibraryGuarded(const wchar_t* path) noexcept
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

        /// Writes one record to the log, at a level matching how it went.
        void logRecord(const AsiLoadRecord& record)
        {
            crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
            const std::string name = utf8(record.path.filename());
            switch (record.status) {
                case AsiLoadStatus::Loaded:
                    logger.info("AsiLoader: loaded {} ({}){}.", name, record.source,
                                record.detail.empty() ? std::string{} : std::format(", {}", record.detail));
                    break;
                case AsiLoadStatus::AlreadyLoaded:
                    logger.info("AsiLoader: {} ({}) was already loaded by another loader; left as it is.", name,
                                record.source);
                    break;
                case AsiLoadStatus::Skipped:
                    logger.info("AsiLoader: skipped {} ({}): {}.", name, record.source, record.detail);
                    break;
                case AsiLoadStatus::Failed:
                    logger.warning("AsiLoader: could not load {} ({}): {}.", name, record.source, record.detail);
                    break;
                case AsiLoadStatus::Crashed:
                    logger.error("AsiLoader: {} ({}) crashed while starting and was skipped: {}.", name,
                                 record.source, record.detail);
                    break;
            }
        }

    } // namespace

    std::string_view asiStatusName(AsiLoadStatus status) noexcept
    {
        switch (status) {
            case AsiLoadStatus::Loaded:
                return "loaded";
            case AsiLoadStatus::AlreadyLoaded:
                return "already_loaded";
            case AsiLoadStatus::Skipped:
                return "skipped";
            case AsiLoadStatus::Failed:
                return "failed";
            case AsiLoadStatus::Crashed:
                return "crashed";
        }
        return "failed";
    }

    AsiLoadRecord loadAsiPlugin(const crabe::domain::AsiCandidate& candidate)
    {
        AsiLoadRecord record{candidate.path, candidate.source, AsiLoadStatus::Skipped, candidate.skipReason};
        if (!candidate.shouldLoad())
            return record;

        const std::optional<std::vector<std::byte>> header = readHeader(candidate.path);
        if (!header) {
            record.status = AsiLoadStatus::Failed;
            record.detail = "the file could not be read";
            return record;
        }
        const std::string problem = crabe::domain::pluginBinaryProblem(crabe::domain::classifyPluginBinary(*header));
        if (!problem.empty()) {
            record.status = AsiLoadStatus::Failed;
            record.detail = problem;
            return record;
        }

        const std::wstring path = candidate.path.wstring();
        if (const HMODULE present = GetModuleHandleW(path.c_str())) {
            record.status = AsiLoadStatus::AlreadyLoaded;
            record.detail.clear();
            record.module = present;
            return record;
        }

        const GuardedLoad load = loadLibraryGuarded(path.c_str());
        if (load.exceptionCode != 0) {
            record.status = AsiLoadStatus::Crashed;
            record.detail = std::format("exception 0x{:08X} in its DllMain", load.exceptionCode);
            return record;
        }
        if (!load.module) {
            record.status = AsiLoadStatus::Failed;
            record.detail = load.lastError == ERROR_DLL_INIT_FAILED
                ? std::string("its DllMain refused to start (returned FALSE)")
                : describeWin32Error(load.lastError);
            return record;
        }
        record.status = AsiLoadStatus::Loaded;
        record.module = load.module;
        record.detail = std::format("at 0x{:08X}", reinterpret_cast<std::uintptr_t>(load.module));
        return record;
    }

    AsiLoader& AsiLoader::get()
    {
        static AsiLoader instance;
        return instance;
    }

    void AsiLoader::loadAll(const fs::path& gameRoot)
    {
        std::call_once(_once, [&] {
            {
                const std::lock_guard lock(_mutex);
                _gameRoot = gameRoot;
            }
            const crabe::domain::Config config = crabe::domain::Config::peek(gameRoot);
            const std::vector<crabe::domain::AsiCandidate> candidates =
                crabe::domain::discoverAsiPlugins(gameRoot, config);

            std::size_t loaded = 0;
            std::size_t problems = 0;
            for (const crabe::domain::AsiCandidate& candidate : candidates) {
                AsiLoadRecord record = loadAsiPlugin(candidate);
                logRecord(record);
                if (record.status == AsiLoadStatus::Loaded)
                    ++loaded;
                else if (record.status == AsiLoadStatus::Failed || record.status == AsiLoadStatus::Crashed)
                    ++problems;
                const std::lock_guard lock(_mutex);
                _records.push_back(std::move(record));
            }
            if (!candidates.empty())
                crabe::shared::Logger::getInstance().info(
                    "AsiLoader: {} of {} .asi plugin(s) loaded, {} failed.", loaded, candidates.size(), problems);
        });
    }

    std::vector<AsiLoadRecord> AsiLoader::records() const
    {
        const std::lock_guard lock(_mutex);
        return _records;
    }

    fs::path AsiLoader::gameRoot() const
    {
        const std::lock_guard lock(_mutex);
        return _gameRoot;
    }

} // namespace crabe::infrastructure
