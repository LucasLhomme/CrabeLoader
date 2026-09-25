/*
** CrabeLoader
** File description:
** Manages in-memory O(1) virtual file system overrides mapping assets to loose mod files.
** Provides thread-safe normalized path resolution with zero disk access on hot lookups.
** Mod content lives under mods/; this service knows nothing about game-specific formats.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_VFS_OVERRIDE_MANAGER_HPP_
#define CRABELOADER_INFRASTRUCTURE_VFS_OVERRIDE_MANAGER_HPP_

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace crabe::infrastructure {

/// Manages virtual-to-physical asset path mappings loaded from the mods directory.
class VfsOverrideManager final {
public:
    struct OverrideEntry {
        std::filesystem::path physicalPath;
        std::string originMod;
        mutable std::size_t hitCount{0};
    };

    struct VfsStats {
        std::size_t totalOverrides{0};
        std::size_t totalResolutions{0};
        std::size_t totalHits{0};
    };

    static VfsOverrideManager& get() noexcept;

    /// Discovers and indexes all loose asset overrides found inside the mods directory.
    void initialize(const std::filesystem::path& gameRoot);

    /// Recursively indexes all loose asset files present within subfolders of modsFolder.
    void scanModsDirectory(const std::filesystem::path& modsFolder);

    /// Manually registers an override mapping a virtual asset path to a physical disk file.
    bool registerOverride(std::string_view virtualPath,
                          const std::filesystem::path& physicalPath,
                          std::string_view originMod);

    /// Resolves a requested asset path to its loose mod file in constant O(1) memory time.
    [[nodiscard]] bool resolve(std::string_view requestedPath,
                               std::filesystem::path& outPhysicalPath) const noexcept;

    /// Optional-returning overload of asset path resolution for convenient query callers.
    [[nodiscard]] std::optional<std::filesystem::path> resolve(
        std::string_view requestedPath) const noexcept;

    /// Normalizes virtual paths into lowercase forward-slash keys without leading prefixes.
    [[nodiscard]] static std::string normalizeVirtualPath(std::string_view path) noexcept;

    /// Returns the total count of currently registered virtual file overrides.
    [[nodiscard]] std::size_t getOverrideCount() const noexcept;

    /// Returns a snapshot of operational lookup and hit statistics.
    [[nodiscard]] VfsStats getStats() const noexcept;

    /// Returns a list of all active virtual paths and their associated override entries.
    [[nodiscard]] std::vector<std::pair<std::string, OverrideEntry>> getActiveOverrides() const;

    /// Returns the virtual path of the most recently redirected asset lookup.
    [[nodiscard]] std::string getLastRedirectedFile() const;

    /// Clears all registered overrides and resets counters.
    void clear() noexcept;

private:
    VfsOverrideManager() = default;
    ~VfsOverrideManager() = default;

    VfsOverrideManager(const VfsOverrideManager&) = delete;
    VfsOverrideManager& operator=(const VfsOverrideManager&) = delete;

    mutable std::shared_mutex _mutex;
    std::unordered_map<std::string, OverrideEntry> _overrides;
    std::filesystem::path _gameRoot;
    mutable std::atomic<std::size_t> _resolutionCount{0};
    mutable std::atomic<std::size_t> _hitCount{0};
    mutable std::string _lastRedirected;
    mutable std::mutex _lastRedirectedMutex;
};

} // namespace crabe::infrastructure

#endif // CRABELOADER_INFRASTRUCTURE_VFS_OVERRIDE_MANAGER_HPP_
