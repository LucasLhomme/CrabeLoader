/*
** CrabeLoader
** File description:
** Implements the in-memory virtual file system indexer and fast O(1) asset resolver.
** Normalizes relative and absolute game paths to match loose files inside active mods.
** Intercepts no Win32 calls itself; the hooks are in infrastructure/vfs_hook.cpp.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/vfs_override_manager.hpp"

#include <algorithm>
#include <cctype>
#include <system_error>

#include "shared/logger.hpp"

namespace crabe::infrastructure {

VfsOverrideManager& VfsOverrideManager::get() noexcept
{
    static VfsOverrideManager instance;
    return instance;
}

std::string VfsOverrideManager::normalizeVirtualPath(std::string_view path) noexcept
{
    if (path.empty())
        return {};

    std::string normalized;
    normalized.reserve(path.size());

    for (char c : path) {
        if (c == '\\')
            normalized.push_back('/');
        else
            normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    size_t start = normalized.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return {};

    size_t end = normalized.find_last_not_of(" \t\r\n");
    normalized = normalized.substr(start, end - start + 1);

    constexpr std::string_view kAssetsToken = "/assets/";
    size_t assetsPos = normalized.find(kAssetsToken);
    if (assetsPos != std::string::npos) {
        normalized = normalized.substr(assetsPos + kAssetsToken.size());
    } else if (normalized.starts_with("assets/")) {
        normalized = normalized.substr(7);
    } else if (normalized.size() >= 2 && normalized[1] == ':') {
        // A mod's own files (entry scripts, modules) are opened by absolute path:
        // reduced to a basename, mods/a/main.lua would resolve to mods/b/main.lua.
        if (normalized.find("/mods/") != std::string::npos)
            return {};
        size_t lastSlash = normalized.rfind('/');
        if (lastSlash != std::string::npos && lastSlash + 1 < normalized.size())
            normalized = normalized.substr(lastSlash + 1);
    }

    while (normalized.starts_with("./"))
        normalized.erase(0, 2);

    while (normalized.starts_with('/'))
        normalized.erase(0, 1);

    while (normalized.ends_with('/'))
        normalized.pop_back();

    std::string deduplicated;
    deduplicated.reserve(normalized.size());
    bool lastWasSlash = false;
    for (char c : normalized) {
        if (c == '/') {
            if (!lastWasSlash)
                deduplicated.push_back(c);
            lastWasSlash = true;
        } else {
            deduplicated.push_back(c);
            lastWasSlash = false;
        }
    }

    return deduplicated;
}

void VfsOverrideManager::initialize(const std::filesystem::path& gameRoot)
{
    std::unique_lock<std::shared_mutex> lock(_mutex);
    _gameRoot = gameRoot;
    std::filesystem::path modsFolder = gameRoot / "mods";

    lock.unlock();
    scanModsDirectory(modsFolder);
}

void VfsOverrideManager::scanModsDirectory(const std::filesystem::path& modsFolder)
{
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
    std::error_code ec;

    if (!std::filesystem::exists(modsFolder, ec) || !std::filesystem::is_directory(modsFolder, ec))
        return;

    std::vector<std::filesystem::path> modDirs;
    for (const auto& entry : std::filesystem::directory_iterator(modsFolder, ec)) {
        if (ec)
            break;
        if (!entry.is_directory(ec))
            continue;

        std::string dirName = entry.path().filename().string();
        if (dirName.empty() || dirName[0] == '.' || dirName[0] == '_')
            continue;

        modDirs.push_back(entry.path());
    }

    std::ranges::sort(modDirs, [](const auto& a, const auto& b) {
        return a.filename().string() < b.filename().string();
    });

    std::unordered_map<std::string, OverrideEntry> fresh;

    for (const auto& modDir : modDirs) {
        const std::string modName = modDir.filename().string();

        for (const auto& fileEntry : std::filesystem::recursive_directory_iterator(modDir, ec)) {
            if (ec)
                break;
            if (!fileEntry.is_regular_file(ec))
                continue;

            std::filesystem::path relPath = std::filesystem::relative(fileEntry.path(), modDir, ec);
            if (ec)
                continue;

            std::string relStr = relPath.generic_string();
            if (relStr == "mod.json" || relStr == "manifest.json")
                continue;

            std::string virtualKey = normalizeVirtualPath(relStr);
            if (virtualKey.empty())
                continue;

            auto it = fresh.find(virtualKey);
            if (it != fresh.end()) {
                logger.info("VFS: Override: '{}' -> '{}' (superseded '{}')",
                            virtualKey, fileEntry.path().string(), it->second.originMod);
                it->second.physicalPath = fileEntry.path();
                it->second.originMod = modName;
            } else {
                fresh[virtualKey] = OverrideEntry{
                    .physicalPath = fileEntry.path(),
                    .originMod = modName,
                    .hitCount = 0
                };
            }
        }
    }

    std::unique_lock<std::shared_mutex> lock(_mutex);
    for (auto& [key, entry] : fresh) {
        auto previous = _overrides.find(key);
        if (previous != _overrides.end())
            entry.hitCount = previous->second.hitCount;
    }
    _overrides = std::move(fresh);

    logger.info("VFS: Indexed {} active virtual asset override(s) across {} mod directory(ies).",
                _overrides.size(), modDirs.size());
}

bool VfsOverrideManager::registerOverride(std::string_view virtualPath,
                                         const std::filesystem::path& physicalPath,
                                         std::string_view originMod)
{
    std::string key = normalizeVirtualPath(virtualPath);
    if (key.empty())
        return false;

    std::unique_lock<std::shared_mutex> lock(_mutex);
    _overrides[key] = OverrideEntry{
        .physicalPath = physicalPath,
        .originMod = std::string(originMod),
        .hitCount = 0
    };
    return true;
}

bool VfsOverrideManager::resolve(std::string_view requestedPath,
                                 std::filesystem::path& outPhysicalPath) const noexcept
{
    _resolutionCount.fetch_add(1, std::memory_order_relaxed);

    std::string key = normalizeVirtualPath(requestedPath);
    if (key.empty())
        return false;

    std::shared_lock<std::shared_mutex> lock(_mutex);
    auto it = _overrides.find(key);
    if (it == _overrides.end())
        return false;

    _hitCount.fetch_add(1, std::memory_order_relaxed);
    ++it->second.hitCount;
    outPhysicalPath = it->second.physicalPath;

    {
        std::lock_guard<std::mutex> lastLock(_lastRedirectedMutex);
        _lastRedirected = key;
    }

    return true;
}

std::optional<std::filesystem::path> VfsOverrideManager::resolve(
    std::string_view requestedPath) const noexcept
{
    std::filesystem::path result;
    if (resolve(requestedPath, result))
        return result;
    return std::nullopt;
}

std::size_t VfsOverrideManager::getOverrideCount() const noexcept
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    return _overrides.size();
}

VfsOverrideManager::VfsStats VfsOverrideManager::getStats() const noexcept
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    return VfsStats{
        .totalOverrides = _overrides.size(),
        .totalResolutions = _resolutionCount.load(std::memory_order_relaxed),
        .totalHits = _hitCount.load(std::memory_order_relaxed)
    };
}

std::vector<std::pair<std::string, VfsOverrideManager::OverrideEntry>>
VfsOverrideManager::getActiveOverrides() const
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    std::vector<std::pair<std::string, OverrideEntry>> entries;
    entries.reserve(_overrides.size());

    for (const auto& [k, v] : _overrides)
        entries.emplace_back(k, v);

    return entries;
}

std::string VfsOverrideManager::getLastRedirectedFile() const
{
    std::lock_guard<std::mutex> lock(_lastRedirectedMutex);
    return _lastRedirected;
}

void VfsOverrideManager::clear() noexcept
{
    std::unique_lock<std::shared_mutex> lock(_mutex);
    _overrides.clear();
    _resolutionCount.store(0, std::memory_order_relaxed);
    _hitCount.store(0, std::memory_order_relaxed);

    std::lock_guard<std::mutex> lastLock(_lastRedirectedMutex);
    _lastRedirected.clear();
}

} // namespace crabe::infrastructure
