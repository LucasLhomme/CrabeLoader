/*
** CrabeLoader
** File description:
** Manages in-memory O(1) virtual file system overrides mapping assets to loose mod files.
** Provides thread-safe normalized path resolution with zero disk access on hot lookups.
** Bank dependency overlay generation is delegated to infrastructure/bank_dependency_patch.cpp.
**
** Authors: @LucasLhomme, @Weresthebeef
*/

#ifndef CRABELOADER_INFRASTRUCTURE_VFS_OVERRIDE_MANAGER_HPP_
#define CRABELOADER_INFRASTRUCTURE_VFS_OVERRIDE_MANAGER_HPP_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <memory>
#include "domain/mod_load_plan.hpp"
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace crabe::infrastructure {

class VfsOverrideManager final {
public:
    struct OverrideEntry {
        std::filesystem::path physicalPath;
        std::string originMod;
        mutable std::size_t hitCount{0};
        bool allowLooseMaterials{false};
        std::string modId;
        std::size_t order{0};
        std::size_t mountIndex{0};
        std::string mount;
        bool alias{false};
        std::string reason;
    };

    struct VfsStats {
        std::size_t totalOverrides{0};
        std::size_t totalResolutions{0};
        std::size_t totalHits{0};
    };

    // Returns the singleton instance of VfsOverrideManager.
    static VfsOverrideManager& get() noexcept;

    // Discovers and indexes all loose asset overrides found inside the mods directory.
    void initialize(const std::filesystem::path& gameRoot,
                    std::function<bool(std::string_view)> modFilter = {});

    // Recursively indexes all loose asset files present within subfolders of modsFolder.
    void scanModsDirectory(const std::filesystem::path& modsFolder);

    struct Explanation {
        std::string path;
        std::optional<OverrideEntry> winner;
        std::vector<OverrideEntry> providers;
    };
    // Returns the immutable load plan published with the current VFS index.
    [[nodiscard]] std::shared_ptr<const crabe::domain::ModLoadPlan> getModPlan() const;
    // Returns all providers and the winner without registering a lookup hit.
    [[nodiscard]] Explanation explain(std::string_view path) const;
    // Returns sorted virtual paths with competing providers.
    [[nodiscard]] std::vector<std::string> getConflictPaths() const;

    // Manually registers an override mapping a virtual asset path to a physical disk file.
    bool registerOverride(std::string_view virtualPath,
                          const std::filesystem::path& physicalPath,
                          std::string_view originMod);

    // Resolves a requested asset path to its loose mod file in constant O(1) memory time.
    [[nodiscard]] bool resolve(std::string_view requestedPath,
                               std::filesystem::path& outPhysicalPath) const noexcept;

    // Resolves a requested asset path and reads the whole loose file into memory.
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> readOverrideBytes(
        std::string_view requestedPath, bool materialRequest = false) const noexcept;

    // Optional-returning overload of asset path resolution.
    [[nodiscard]] std::optional<std::filesystem::path> resolve(
        std::string_view requestedPath) const noexcept;

    // Normalizes virtual paths into lowercase forward-slash keys without leading prefixes.
    [[nodiscard]] static std::string normalizeVirtualPath(std::string_view path) noexcept;

    // Returns the total count of currently registered virtual file overrides.
    [[nodiscard]] std::size_t getOverrideCount() const noexcept;

    // Returns a snapshot of operational lookup and hit statistics.
    [[nodiscard]] VfsStats getStats() const noexcept;

    // Returns a list of all active virtual paths and their associated override entries.
    [[nodiscard]] std::vector<std::pair<std::string, OverrideEntry>> getActiveOverrides() const;

    // Returns the virtual path of the most recently redirected asset lookup.
    [[nodiscard]] std::string getLastRedirectedFile() const;

    // Clears all registered overrides and resets counters.
    void clear() noexcept;

private:
    VfsOverrideManager() = default;
    ~VfsOverrideManager() = default;

    VfsOverrideManager(const VfsOverrideManager&) = delete;
    VfsOverrideManager& operator=(const VfsOverrideManager&) = delete;

    struct string_hash { using is_transparent = void; [[nodiscard]] size_t operator()(std::string_view txt) const { return std::hash<std::string_view>{}(txt); } [[nodiscard]] size_t operator()(const std::string &txt) const { return std::hash<std::string>{}(txt); } };
    mutable std::shared_mutex _mutex;
    mutable std::shared_mutex _fastCacheMutex;
    mutable std::unordered_map<std::string, std::optional<std::filesystem::path>, string_hash, std::equal_to<>> _fastCache;
    std::unordered_map<std::string, OverrideEntry> _overrides;
    std::filesystem::path _gameRoot;
    std::function<bool(std::string_view)> _modFilter;
    std::shared_ptr<const crabe::domain::ModLoadPlan> _modPlan;
    std::unordered_map<std::string, std::vector<OverrideEntry>> _providers;
    std::mutex _scanMutex;
    mutable std::atomic<std::size_t> _resolutionCount{0};
    mutable std::atomic<std::size_t> _hitCount{0};
    mutable std::string _lastRedirected;
    mutable std::mutex _lastRedirectedMutex;
};

} // namespace crabe::infrastructure

#endif // CRABELOADER_INFRASTRUCTURE_VFS_OVERRIDE_MANAGER_HPP_

