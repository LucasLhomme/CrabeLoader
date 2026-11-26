/*
** CrabeLoader
** File description:
** Implements the in-memory virtual file system indexer and fast O(1) asset resolver.
** Normalizes relative and absolute game paths to match loose files inside active mods.
** Intercepts no Win32 calls itself; the hooks are in infrastructure/vfs_hook.cpp.
**
** Authors: @LucasLhomme, @Weresthebeef
*/

#include "infrastructure/vfs_override_manager.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <system_error>
#include <set>
#include <stdexcept>

#include "shared/logger.hpp"
#include "infrastructure/bank_dependency_patch.hpp"
#include "third_party/json.hpp"
#include <cstring>

namespace crabe::infrastructure {
namespace {
thread_local bool scanningMods = false;
// Prevents hooked I/O during rescans from reading the previous generation of overrides.
struct ScanScope {
    ScanScope() { scanningMods = true; }
    ~ScanScope() { scanningMods = false; }
};
}


// Returns the singleton instance of VfsOverrideManager.
VfsOverrideManager& VfsOverrideManager::get() noexcept
{
    static VfsOverrideManager instance;
    return instance;
}

// Normalizes a path into a canonical virtual asset key.
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

// Initializes the VFS override manager with the root directory of the game.
void VfsOverrideManager::initialize(const std::filesystem::path& gameRoot,
    std::function<bool(std::string_view)> modFilter)
{
    std::unique_lock<std::shared_mutex> lock(_mutex);
    _gameRoot = gameRoot;
    _modFilter = std::move(modFilter);
    std::filesystem::path modsFolder = gameRoot / "mods";

    lock.unlock();
    scanModsDirectory(modsFolder);
}

// Scans the mods directory recursively to index asset overrides.
void VfsOverrideManager::scanModsDirectory(const std::filesystem::path& modsFolder)
{
    std::lock_guard scanLock(_scanMutex);
    ScanScope scanScope;
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
    try {
    auto plan = std::make_shared<const crabe::domain::ModLoadPlan>(
        crabe::domain::discoverModLoadPlan(modsFolder, _modFilter));
    std::unordered_map<std::string, OverrideEntry> fresh;
    std::unordered_map<std::string, std::vector<OverrideEntry>> providers;
    std::vector<std::filesystem::path> bankPatches;
    std::vector<std::string> patchMods;
    const auto contained = [](const std::filesystem::path& file, const std::filesystem::path& root) {
        const auto relative = std::filesystem::weakly_canonical(file).lexically_relative(std::filesystem::weakly_canonical(root));
        return !relative.empty() && !relative.is_absolute() && *relative.begin() != "..";
    };
    const auto add = [&](const std::string& key, OverrideEntry entry) {
        auto& history = providers[key];
        if (std::ranges::any_of(history, [&](const auto& previous) {
            return previous.physicalPath == entry.physicalPath && previous.modId == entry.modId && previous.mountIndex == entry.mountIndex;
        })) return;
        const auto existing = fresh.find(key);
        const bool sameModMount = existing != fresh.end() && existing->second.order == entry.order && existing->second.mountIndex == entry.mountIndex;
        const bool losesToExact = sameModMount && !existing->second.alias && entry.alias;
        entry.reason = existing == fresh.end() ? "only provider" :
            existing->second.order != entry.order ? "later resolved mod wins" :
            existing->second.mountIndex != entry.mountIndex ? "later declared mount wins" :
            existing->second.alias != entry.alias ? "exact path preferred over texture alias" : "deterministic file order";
        history.push_back(entry);
        if (!losesToExact) fresh[key] = std::move(entry);
        else fresh[key].reason = "exact path preferred over texture alias";
    };
    for (std::size_t position = 0; position < plan->resolution.sourceIndex.size(); ++position) {
        const auto& mod = plan->candidates[plan->resolution.sourceIndex[position]];
        if (!mod.isDirectory) continue;
        const auto& modDir = mod.path;
        if (!contained(modDir, modsFolder)) throw std::runtime_error("mod directory escapes mods root: " + modDir.string());
        const auto bankPatch = modDir / "bank_dependencies.json";
        if (std::filesystem::is_regular_file(bankPatch)) {
            if (!contained(bankPatch, modDir)) throw std::runtime_error("bank patch escapes its mod directory");
            bankPatches.push_back(bankPatch);
            patchMods.push_back(mod.id);
        }
        const auto& mounts = mod.manifest.getAssetMounts();
        for (std::size_t mountIndex = 0; mountIndex < mounts.size(); ++mountIndex) {
            const auto& mount = mounts[mountIndex];
            const auto directory = modDir / mount.source;
            if (!std::filesystem::exists(directory)) continue;
            if (!contained(directory, modDir)) throw std::runtime_error("asset mount escapes its mod directory: " + directory.string());
            if (!std::filesystem::is_directory(directory)) throw std::runtime_error("asset mount is not a directory: " + directory.string());
            std::vector<std::filesystem::path> mountedFiles;
            for (const auto& file : std::filesystem::recursive_directory_iterator(directory)) {
                if (!file.is_regular_file()) continue;
                if (!contained(file.path(), modDir)) throw std::runtime_error("mounted file escapes its mod directory: " + file.path().string());
                mountedFiles.push_back(file.path());
            }
            std::ranges::sort(mountedFiles);
            for (const auto& file : mountedFiles) {
                const auto relative = file.lexically_relative(directory).generic_string();
                const auto extension = normalizeVirtualPath(file.extension().string());
                if (extension == ".asi" || extension == ".dll" || extension == ".exe") continue;
                if (mount.source == "characters" && mount.target == "characters" && (extension == ".lua" || extension == ".patch")) continue;
                const auto key = normalizeVirtualPath(mount.target.empty() ? relative : mount.target + "/" + relative);
                if (key.empty()) continue;
                OverrideEntry entry{.physicalPath = file, .originMod = mod.name, .hitCount = 0,
                    .allowLooseMaterials = mod.manifest.allowsLooseMaterials(), .modId = mod.id,
                    .order = position, .mountIndex = mountIndex, .mount = mount.source + " -> " + mount.target};
                add(key, entry);
                if (extension == ".tbody") {
                    const auto filename = normalizeVirtualPath(file.filename().string());
                    entry.alias = true;
                    add(filename, entry);
                    add("textures/" + filename, entry);
                    if (filename.size() >= 2) add("textures/" + filename.substr(0, 2) + "/" + filename, entry);
                }
            }
        }
    }

    if (!bankPatches.empty()) {
        const auto gameRoot = modsFolder.parent_path();
        const auto effective = [&](std::string_view key) {
            auto entry = fresh.find(std::string(key));
            return entry != fresh.end() ? entry->second.physicalPath : gameRoot / "assets" / key;
        };
        try {
            const auto source = effective("sound/bankloads.bdp");
            const auto generated = buildBankDependencyOverlay(source, bankPatches,
                modsFolder / ".crabeloader-cache" / "bank-dependencies",
                [&](std::string_view name) {
                    const auto path = effective("sound/" + std::string(name));
                    const auto exists = [](const std::filesystem::path& file) {
                        return std::filesystem::is_regular_file(file) && std::filesystem::file_size(file) > 0;
                    };
                    if (exists(path)) return true;
                    if (!(name.ends_with(".dbnk") || name.ends_with(".dlg"))) return false;
                    std::set<std::string> languages;
                    const auto dialog = gameRoot / "assets/sound/dialog";
                    if (std::filesystem::is_directory(dialog)) {
                        for (const auto& language : std::filesystem::directory_iterator(dialog))
                            if (language.is_directory()) languages.insert(language.path().filename().string());
                    }
                    for (const auto& [key, entry] : fresh) {
                        constexpr std::string_view prefix = "sound/dialog/";
                        if (!key.starts_with(prefix)) continue;
                        const auto slash = key.find('/', prefix.size());
                        if (slash != std::string::npos) languages.insert(key.substr(prefix.size(), slash - prefix.size()));
                    }
                    for (const auto& language : languages)
                        if (exists(effective("sound/dialog/" + language + "/" + std::string(name)))) return true;
                    return false;
                });
            if (generated) {
                std::string reason = "generated additive bank overlay; patches:";
                for (const auto& id : patchMods) reason += " " + id;
                OverrideEntry overlay{.physicalPath = *generated, .originMod = "bank-dependency-overlay",
                    .modId = "@bank-dependency-overlay", .order = plan->resolution.sourceIndex.size(),
                    .mount = "generated cache", .reason = reason};
                providers["sound/bankloads.bdp"].push_back(overlay);
                fresh["sound/bankloads.bdp"] = std::move(overlay);
                logger.info("BankDependencies: merged {} patch file(s) over '{}' -> '{}' (restart game to reload sound banks).",
                    bankPatches.size(), source.string(), generated->string());
            } else {
                logger.info("BankDependencies: requested dependencies already present; keeping effective table.");
            }
        } catch (const std::exception& error) {
            logger.warning("BankDependencies: overlay rejected; keeping effective table: {}", error.what());
        }
    }

    std::unique_lock<std::shared_mutex> lock(_mutex);
    for (auto& [key, entry] : fresh) {
        auto previous = _overrides.find(key);
        if (previous != _overrides.end())
            entry.hitCount = previous->second.hitCount;
    }
    _overrides = std::move(fresh);
    _providers = std::move(providers);
    _modPlan = std::move(plan);

    {
        std::unique_lock<std::shared_mutex> fastLock(_fastCacheMutex);
        _fastCache.clear();
    }

    const auto count = _overrides.size();
    const auto modCount = _modPlan->resolution.loadOrder.size();
    lock.unlock();
    const auto conflicts = getConflictPaths();
    logger.info("VFS: Indexed {} assets from {} resolved mods; {} conflicting paths.", count, modCount, conflicts.size());
    std::size_t loggedConflicts = 0;
    for (const auto& key : conflicts) {
        if (loggedConflicts++ >= 64) break;
        const auto detail = explain(key);
        logger.info("VFS conflict: '{}' -> '{}' ({}, {} providers). Use Crabe.Vfs.explain for details.",
            key, detail.winner->modId, detail.winner->reason, detail.providers.size());
    }
    if (conflicts.size() > 64) logger.info("VFS: {} more conflict paths available via Crabe.Vfs.conflicts().", conflicts.size() - 64);
    } catch (const std::exception& error) {
        logger.error("VFS: scan failed; keeping previous index and mod plan: {}", error.what());
    }
}

// Returns the shared dependency plan from the same generation as the asset index.
std::shared_ptr<const crabe::domain::ModLoadPlan> VfsOverrideManager::getModPlan() const {
    std::shared_lock lock(_mutex);
    return _modPlan;
}
// Explains providers and winner without changing lookup counters.
VfsOverrideManager::Explanation VfsOverrideManager::explain(std::string_view path) const {
    Explanation result;
    result.path = normalizeVirtualPath(path);
    std::shared_lock lock(_mutex);
    if (const auto found = _overrides.find(result.path); found != _overrides.end()) result.winner = found->second;
    if (const auto found = _providers.find(result.path); found != _providers.end()) result.providers = found->second;
    return result;
}
// Lists conflicts deterministically for author tooling and log output.
std::vector<std::string> VfsOverrideManager::getConflictPaths() const {
    std::vector<std::string> result;
    std::shared_lock lock(_mutex);
    for (const auto& [key, providers] : _providers) if (providers.size() > 1) result.push_back(key);
    std::ranges::sort(result);
    return result;
}

// Manually registers a virtual asset override mapping.
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
        .hitCount = 0,
        .modId = std::string(originMod),
        .reason = "manual runtime override"
    };

    _providers[key].push_back(_overrides[key]);
    std::unique_lock<std::shared_mutex> fastLock(_fastCacheMutex);
    _fastCache.clear();
    return true;
}

// Resolves a requested asset path to an overridden physical file path.
bool VfsOverrideManager::resolve(std::string_view requestedPath,
                                 std::filesystem::path& outPhysicalPath) const noexcept
{
    if (scanningMods) return false;
    {
        std::shared_lock<std::shared_mutex> fastLock(_fastCacheMutex);
        auto cacheIt = _fastCache.find(requestedPath);
        if (cacheIt != _fastCache.end()) {
            _resolutionCount.fetch_add(1, std::memory_order_relaxed);
            if (cacheIt->second.has_value()) {
                _hitCount.fetch_add(1, std::memory_order_relaxed);
                outPhysicalPath = cacheIt->second.value();
                return true;
            }
            return false;
        }
    }

    _resolutionCount.fetch_add(1, std::memory_order_relaxed);

    auto updateCache = [&](bool found) {
        std::unique_lock<std::shared_mutex> fastLock(_fastCacheMutex);
        if (found) {
            _fastCache.insert_or_assign(std::string(requestedPath), std::make_optional(outPhysicalPath));
        } else {
            _fastCache.insert_or_assign(std::string(requestedPath), std::nullopt);
        }
        return found;
    };

    std::string key = normalizeVirtualPath(requestedPath);
    if (key.empty())
        return updateCache(false);

    std::shared_lock<std::shared_mutex> lock(_mutex);
    auto it = _overrides.find(key);
    if (it != _overrides.end()) {
        _hitCount.fetch_add(1, std::memory_order_relaxed);
        ++it->second.hitCount;
        outPhysicalPath = it->second.physicalPath;

        std::lock_guard<std::mutex> lastLock(_lastRedirectedMutex);
        _lastRedirected = key;
        return updateCache(true);
    }

    if (key.ends_with(".tbody")) {
        const size_t lastSlash = key.rfind('/');
        const std::string filename = (lastSlash != std::string::npos) ? key.substr(lastSlash + 1) : key;

        auto itFn = _overrides.find(filename);
        if (itFn != _overrides.end()) {
            _hitCount.fetch_add(1, std::memory_order_relaxed);
            ++itFn->second.hitCount;
            outPhysicalPath = itFn->second.physicalPath;

            std::lock_guard<std::mutex> lastLock(_lastRedirectedMutex);
            _lastRedirected = filename;
            return updateCache(true);
        }

        const std::string texKey = "textures/" + filename;
        auto itTex = _overrides.find(texKey);
        if (itTex != _overrides.end()) {
            _hitCount.fetch_add(1, std::memory_order_relaxed);
            ++itTex->second.hitCount;
            outPhysicalPath = itTex->second.physicalPath;

            std::lock_guard<std::mutex> lastLock(_lastRedirectedMutex);
            _lastRedirected = texKey;
            return updateCache(true);
        }
    }

    return updateCache(false);
}

// Resolves a requested asset path and returns an optional path if found.
std::optional<std::filesystem::path> VfsOverrideManager::resolve(
    std::string_view requestedPath) const noexcept
{
    std::filesystem::path result;
    if (resolve(requestedPath, result))
        return result;
    return std::nullopt;
}

// Reads the full binary content of an overridden asset file into memory.
std::optional<std::vector<std::uint8_t>> VfsOverrideManager::readOverrideBytes(
    std::string_view requestedPath, bool materialRequest) const noexcept
{
    constexpr std::streamoff kMaxOverrideBytes = 256LL * 1024 * 1024;
    std::filesystem::path physical;

    if (materialRequest) {
        const auto key = normalizeVirtualPath(requestedPath);
        if (!key.ends_with(".mtb") ||
            !(key.starts_with("ui/icons/") || key.starts_with("ui/virtualreadercharacters/")))
            return std::nullopt;
        std::shared_lock<std::shared_mutex> lock(_mutex);
        const auto entry = _overrides.find(key);
        if (entry == _overrides.end() || !entry->second.allowLooseMaterials)
            return std::nullopt;
        physical = entry->second.physicalPath;
    }
    else if (!resolve(requestedPath, physical))
        return std::nullopt;

    try {
        std::ifstream in(physical, std::ios::binary | std::ios::ate);
        if (!in)
            return std::nullopt;

        const std::streamoff length = in.tellg();
        if (length <= 0 || length > kMaxOverrideBytes)
            return std::nullopt;

        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
        in.seekg(0);
        if (!in.read(reinterpret_cast<char*>(bytes.data()), length))
            return std::nullopt;
        if (materialRequest) {
            if (bytes.size() < 16 || std::memcmp(bytes.data(), "BNDL", 4) != 0)
                return std::nullopt;
            std::uint32_t version = 0, declaredSize = 0;
            std::memcpy(&version, bytes.data() + 4, 4);
            std::memcpy(&declaredSize, bytes.data() + 8, 4);
            if (version != 1 || declaredSize != bytes.size())
                return std::nullopt;
        }
        return bytes;
    } catch (...) {
        return std::nullopt;
    }
}

// Returns the total number of currently registered asset overrides.
std::size_t VfsOverrideManager::getOverrideCount() const noexcept
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    return _overrides.size();
}

// Returns runtime performance and lookup statistics for the VFS.
VfsOverrideManager::VfsStats VfsOverrideManager::getStats() const noexcept
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    return VfsStats{
        .totalOverrides = _overrides.size(),
        .totalResolutions = _resolutionCount.load(std::memory_order_relaxed),
        .totalHits = _hitCount.load(std::memory_order_relaxed)
    };
}

// Returns a snapshot of all active override entries.
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

// Returns the canonical virtual path of the most recently redirected file.
std::string VfsOverrideManager::getLastRedirectedFile() const
{
    std::lock_guard<std::mutex> lock(_lastRedirectedMutex);
    return _lastRedirected;
}

// Clears all registered overrides and resets lookup statistics.
void VfsOverrideManager::clear() noexcept
{
    std::unique_lock<std::shared_mutex> lock(_mutex);
    _overrides.clear();
    _providers.clear();
    _modPlan.reset();
    _resolutionCount.store(0, std::memory_order_relaxed);
    _hitCount.store(0, std::memory_order_relaxed);

    std::unique_lock<std::shared_mutex> fastLock(_fastCacheMutex);
    _fastCache.clear();

    std::lock_guard<std::mutex> lastLock(_lastRedirectedMutex);
    _lastRedirected.clear();
}

} // namespace crabe::infrastructure
