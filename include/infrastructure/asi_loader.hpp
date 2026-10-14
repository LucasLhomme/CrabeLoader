/*
** CrabeLoader
** File description:
** Declares the .asi plugin loader: loads what domain/asi_plugins.hpp lists, once per process.
** A plugin that crashes or refuses to start is reported and skipped; the game keeps running.
** Decides neither which files nor when; main.cpp and entry_point_hook.hpp do.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_ASI_LOADER_HPP_
#define CRABELOADER_INFRASTRUCTURE_ASI_LOADER_HPP_

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "domain/asi_plugins.hpp"

namespace crabe::infrastructure {

    enum class AsiLoadStatus : std::uint8_t {
        Loaded,
        AlreadyLoaded,
        Skipped,
        Failed,
        Crashed,
    };

    struct AsiLoadRecord {
        std::filesystem::path path;
        std::string source;
        AsiLoadStatus status{AsiLoadStatus::Skipped};
        std::string detail;
    };

    /// Returns the word Lua and the log use for a status: "loaded", "already_loaded", "skipped", ...
    [[nodiscard]] std::string_view asiStatusName(AsiLoadStatus status) noexcept;

    /// Loads one plugin, or explains why not: a skipped candidate, a file that is no 32-bit DLL,
    /// a plugin some other loader already loaded, a DllMain that refused or crashed.
    [[nodiscard]] AsiLoadRecord loadAsiPlugin(const crabe::domain::AsiCandidate& candidate);

    class AsiLoader final {
    public:
        [[nodiscard]] static AsiLoader& get();

        /// Lists and loads the plugins of `gameRoot` the first time it is called, and does nothing after.
        /// Reads crabe.toml without writing it, since the loader thread may be creating it at the same moment.
        void loadAll(const std::filesystem::path& gameRoot);

        /// Returns what loadAll() did with every plugin it found, in load order.
        [[nodiscard]] std::vector<AsiLoadRecord> records() const;

        /// Returns the folder loadAll() searched, or an empty path before it ran.
        [[nodiscard]] std::filesystem::path gameRoot() const;

    private:
        AsiLoader() = default;

        mutable std::mutex _mutex;
        std::once_flag _once;
        std::filesystem::path _gameRoot;
        std::vector<AsiLoadRecord> _records;
    };

} // namespace crabe::infrastructure

#endif /* !CRABELOADER_INFRASTRUCTURE_ASI_LOADER_HPP_ */
