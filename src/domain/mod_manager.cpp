/*
** CrabeLoader
** File description:
** Discovers mods, filters by profile, assigns ids, resolves order, then runs each chunk sandboxed.
** Mod names come from unpacked archives, so both Lua chunks are constants fed values as arguments.
** Decides no order itself -- the rules are in src/domain/dependency_resolver.cpp.
**
** Authors: @LucasLhomme
*/

#include "domain/mod_manager.hpp"
#include "domain/config.hpp"
#include "domain/dependency_resolver.hpp"
#include "domain/mod_entry.hpp"
#include "domain/mod_id.hpp"
#include "domain/mod_manifest.hpp"
#include "application/loader.hpp"
#include "infrastructure/crash_reporter.hpp"
#include "infrastructure/hook_registry.hpp"
#include "infrastructure/lua_call.hpp"
#include "infrastructure/vfs_override_manager.hpp"
#include "presentation/draw_buffer.hpp"
#include "shared/logger.hpp"
#include "shared/version.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace crabe::domain {

static constexpr std::string_view kSetPackagePathChunk = R"LUA(
    local root = ...
    package.path = root .. '/?.lua;' .. root .. '/modules/?.lua;' .. package.path
)LUA";

static constexpr std::string_view kLoadModChunk = R"LUA(
    local path, modName = ...
    if not (Crabe and Crabe.Sandbox and Crabe.Sandbox.create) then
        error("Crabe.Sandbox.create is unavailable; the embedded API did not load")
    end
    local env = Crabe.Sandbox.create(modName)
    Crabe.Registry.setCurrentOwner(modName)
    local chunk, err = loadfile(path)
    if not chunk then
        Crabe.Registry.setCurrentOwner(nil)
        error(err)
    end
    setfenv(chunk, env)
    local ok, runErr = pcall(chunk)
    Crabe.Registry.setCurrentOwner(nil)
    if not ok then error(runErr) end
)LUA";

namespace {

    [[nodiscard]] std::string versionLabel(const ModManifest& manifest)
    {
        return manifest.getVersion().empty() ? std::string("(no version)") : manifest.getVersion();
    }

    struct Candidate {
        std::string id;
        std::string name;
        std::filesystem::path path;
        bool isDirectory{false};
        ModManifest manifest;
    };

} // namespace

// Returns the singleton instance of ModManager.
ModManager& ModManager::get()
{
    static ModManager instance;
    return instance;
}

// Whether this state is behind the latest hot-reload request. Asked from
// inside the state's own pcall, which is the only thread allowed to touch it.
bool ModManager::needsReload(void* L) const
{
    return _reloads.needsReload(L);
}

// Flags a hot reload. Every live state catches up the next time it runs; none
// is reloaded from here, because "here" is whichever thread pressed the key.
void ModManager::requestHotReload()
{
    _reloads.requestReload();
    crabe::shared::Logger::getInstance().info(
        "ModManager: hot-reload requested (F4). Re-scanning VFS overrides & invalidating texture cache...");
    const std::filesystem::path modsFolder = _modsFolder.empty()
        ? (std::filesystem::current_path() / "mods")
        : _modsFolder;
    crabe::infrastructure::VfsOverrideManager::get().scanModsDirectory(modsFolder);
}

// Returns the collection of discovered and tracked mods.
const std::vector<Mod>& ModManager::getMods() const noexcept
{
    return _mods;
}

// Returns the currently configured mods folder path.
const std::filesystem::path& ModManager::getModsFolder() const noexcept
{
    return _modsFolder;
}

// Loads a modular directory into an isolated sandbox environment.
bool ModManager::loadModDirectory(void* L, const std::filesystem::path& modPath,
                                  const std::string& modName, const ModManifest& manifest)
{
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

    const std::array<std::string, 1> pathArgs{ modPath.generic_string() };
    std::string pathError;
    if (!crabe::infrastructure::LuaCall::get().runChunkWithArgs(
            L, kSetPackagePathChunk, pathArgs, pathError)) {
        logger.error("ModManager: mod '{}': could not extend the Lua module search path: {}",
                     modName, pathError);
    }

    const ModEntryPlan plan = planModEntry(modPath, modName, manifest);

    if (plan.isContentOnly()) {
        logger.debug("ModManager: mod '{}' ships no Lua; its {} were read at startup.",
                     modName, plan.describeContentDirectories());
        return true;
    }

    if (plan.isEmpty()) {
        logger.error("ModManager: mod '{}' ran nothing -- no entry script, no .lua file at the "
                     "top level of '{}', and no characters/ or skilltrees/ folder either. "
                     "Looked for {}. Lua in any other subdirectory is a module tree, not an "
                     "entry point: add a main.lua that requires it, or name the entry script "
                     "with \"entry\" in mod.json.",
                     modName, modPath.generic_string(), plan.describeTriedEntryNames());
        return false;
    }

    if (plan.entryScriptExists) {
        const std::array<std::string, 2> loadArgs{ plan.entryScript.generic_string(), modName };

        std::string outError;
        const bool loaded = crabe::infrastructure::LuaCall::get().runChunkWithArgs(
            L, kLoadModChunk, loadArgs, outError);
        if (!loaded) {
            logger.error("ModManager: mod '{}' failed to run: {}", modName, outError);
        } else {
            logger.debug("ModManager: mod '{}' ran entry '{}'.",
                         modName, plan.entryScript.filename().string());
        }
        return loaded;
    }

    bool anyFailed = false;
    for (const std::filesystem::path& file : plan.looseScripts) {
        if (!crabe::infrastructure::LuaCall::get().runFile(L, file.string().c_str()))
            anyFailed = true;
    }
    logger.debug("ModManager: mod directory '{}' ran the {} .lua file(s) at its top level.",
                 modName, plan.looseScripts.size());

    return !anyFailed;
}

// Executes a standalone Lua mod script in an isolated sandbox.
bool ModManager::loadStandaloneScript(void* L, const std::filesystem::path& scriptPath,
                                      const std::string& modName)
{
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

    const std::array<std::string, 2> loadArgs{ scriptPath.generic_string(), modName };

    std::string outError;
    const bool loaded = crabe::infrastructure::LuaCall::get().runChunkWithArgs(
        L, kLoadModChunk, loadArgs, outError);
    if (!loaded) {
        logger.error("ModManager: standalone mod '{}' failed: {}",
                     scriptPath.filename().string(), outError);
    }

    return loaded;
}

// Discovers and loads mods from the given directory in resolved dependency order.
void ModManager::discoverAndLoadMods(void* L, const std::filesystem::path& modsFolder)
{
    if (!L)
        return;

    _reloads.markReloaded(L);

    std::lock_guard<std::mutex> lock(_mutex);
    _modsFolder = modsFolder;
    _mods.clear();

    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
    logger.debug("ModManager: reading mods folder '{}'...", modsFolder.string());

    std::error_code ec;
    if (!std::filesystem::exists(modsFolder, ec)) {
        logger.info("ModManager: mods folder does not exist, creating...");
        std::filesystem::create_directories(modsFolder, ec);
        return;
    }

    std::vector<Candidate> candidates;

    for (const auto& entry : std::filesystem::directory_iterator(modsFolder, ec)) {
        if (entry.is_directory()) {
            std::string folderName = entry.path().filename().string();
            if (folderName.empty() || folderName[0] == '.' || folderName[0] == '_')
                continue;
            Candidate candidate;
            candidate.name = std::move(folderName);
            candidate.path = entry.path();
            candidate.isDirectory = true;
            candidate.manifest = ModManifest(entry.path() / "mod.json");
            candidates.push_back(std::move(candidate));
        } else if (entry.is_regular_file() && entry.path().extension() == ".lua") {
            Candidate candidate;
            candidate.name = entry.path().stem().string();
            candidate.path = entry.path();
            candidate.isDirectory = false;
            candidates.push_back(std::move(candidate));
        }
    }

    for (Candidate& candidate : candidates) {
        if (candidate.manifest.isMalformed() && candidate.manifest.diagnostic().has_value()) {
            logger.error("ModManager: mod '{}' has an unusable mod.json, ignoring it: {}",
                         candidate.name, candidate.manifest.diagnostic()->what());
            candidate.manifest = ModManifest{};
        } else if (candidate.manifest.isValid() && !candidate.manifest.getIgnoredKeys().empty()) {
            std::string keys;
            for (const std::string& key : candidate.manifest.getIgnoredKeys()) {
                if (!keys.empty())
                    keys += ", ";
                keys += key;
            }
            logger.debug("ModManager: mod '{}': mod.json (manifestVersion {}) has keys this "
                         "loader does not read: {}",
                         candidate.name, candidate.manifest.getManifestVersion(), keys);
        }
    }

    {
        crabe::domain::Config& activeConfig = crabe::domain::Config::active();
        std::vector<Candidate> enabledCandidates;
        enabledCandidates.reserve(candidates.size());
        for (Candidate& candidate : candidates) {
            const std::string provisionalId =
                (candidate.manifest.isValid() && !candidate.manifest.getId().empty())
                    ? candidate.manifest.getId()
                    : synthesiseModId(candidate.name);
            if (activeConfig.isModEnabled(provisionalId)) {
                enabledCandidates.push_back(std::move(candidate));
            } else {
                logger.debug("ModManager: mod '{}' ({}) skipped: disabled by profile '{}'.",
                             candidate.name, provisionalId, activeConfig.activeProfileName());
            }
        }
        candidates = std::move(enabledCandidates);
    }

    std::vector<ModIdCandidate> idCandidates;
    idCandidates.reserve(candidates.size());
    for (const Candidate& candidate : candidates) {
        idCandidates.push_back(ModIdCandidate{
            .declaredId = (candidate.manifest.isValid() && !candidate.manifest.getId().empty())
                              ? candidate.manifest.getId()
                              : std::string{},
            .folderName = candidate.name,
            .sortKey = candidate.path.generic_string(),
        });
    }

    std::vector<ModIdRenumbering> renumbered;
    const std::vector<std::string> assignedIds = assignModIds(idCandidates, &renumbered);
    for (std::size_t index = 0; index < candidates.size(); ++index)
        candidates[index].id = assignedIds[index];

    for (const ModIdRenumbering& collision : renumbered) {
        logger.warning("ModManager: mod '{}' would be called '{}', which is already taken; "
                       "calling it '{}' instead. Rename the folder, or give it a mod.json with "
                       "an \"id\", if anything needs to depend on it by name.",
                       candidates[collision.index].name, collision.wantedId, collision.chosenId);
    }

    std::vector<ResolverInput> inputs;
    inputs.reserve(candidates.size());
    for (const Candidate& candidate : candidates)
        inputs.push_back(ResolverInput{ candidate.id, &candidate.manifest });

    const SemVer loaderVersion(crabe::version::Major, crabe::version::Minor,
                               crabe::version::Patch);
    const ResolutionResult resolution = resolve(inputs, loaderVersion);

    logger.info("ModManager: {} mod(s) found in '{}': {} to load, {} rejected (loader v{}).",
                candidates.size(), modsFolder.string(), resolution.loadOrder.size(),
                resolution.rejected.size(), crabe::version::String);

    std::vector<bool> ran(candidates.size(), false);
    _mods.reserve(resolution.loadOrder.size());

    crabe::infrastructure::CrashReporter::forgetMods();

    for (std::size_t position = 0; position < resolution.loadOrder.size(); ++position) {
        const std::size_t index = resolution.sourceIndex[position];
        Candidate& candidate = candidates[index];
        ran[index] = true;

        const crabe::infrastructure::ScopedMod scopedMod(candidate.id);
        crabe::infrastructure::CrashReporter::pushBreadcrumb("ModManager: running ", candidate.id);

        const auto startedAt = std::chrono::steady_clock::now();
        const bool loaded = candidate.isDirectory
            ? loadModDirectory(L, candidate.path, candidate.name, candidate.manifest)
            : loadStandaloneScript(L, candidate.path, candidate.name);
        const double elapsedMs = std::chrono::duration<double, std::milli>(
                                     std::chrono::steady_clock::now() - startedAt)
                                     .count();

        crabe::infrastructure::CrashReporter::registerMod(
            candidate.id, candidate.manifest.getVersion(), loaded);

        logger.info("ModManager: [{}/{}] {} {} in '{}' -- {} in {:.1f} ms",
                    position + 1, resolution.loadOrder.size(), candidate.id,
                    versionLabel(candidate.manifest), candidate.name,
                    loaded ? "loaded" : "FAILED TO RUN", elapsedMs);

        _mods.push_back(Mod{
            .id = candidate.id,
            .name = candidate.name,
            .rootPath = candidate.path,
            .isLoaded = loaded
        });
    }

    std::multimap<std::string, std::size_t> rejectedCopies;
    for (std::size_t index = 0; index < candidates.size(); ++index) {
        if (!ran[index])
            rejectedCopies.emplace(candidates[index].id, index);
    }

    for (const RejectedMod& rejection : resolution.rejected) {
        std::string version = "(no version)";
        std::string folder = "(no folder)";
        const auto copy = rejectedCopies.find(rejection.id);
        if (copy != rejectedCopies.end()) {
            version = versionLabel(candidates[copy->second].manifest);
            folder = candidates[copy->second].name;
            rejectedCopies.erase(copy);
        }
        logger.warning("ModManager: [--] {} {} in '{}' -- not loaded, {}: {}",
                       rejection.id.empty() ? std::string("(unnamed)") : rejection.id, version,
                       folder, describe(rejection.reason), rejection.detail);
    }

    crabe::infrastructure::LuaCall::get().runSnippet(L,
        "if Crabe and Crabe.Mod and Crabe.Mod.dispatchInit then "
        "    Crabe.Mod.dispatchInit() "
        "end");

    crabe::infrastructure::LuaCall::get().runSnippet(L,
        "if Crabe and Crabe.Events and Crabe.Events.emit then "
        "    Crabe.Events.emit('init') "
        "end");
}

// Triggers lifecycle shutdown and completely reloads all mods from disk.
void ModManager::reloadAllMods(void* L)
{
    if (!L)
        return;

    const bool firstOfGeneration = _reloads.markReloaded(L);

    crabe::shared::Logger::getInstance().info(
        "ModManager: reloading all mods in Lua state 0x{:X} (generation {}).",
        reinterpret_cast<uintptr_t>(L), _reloads.generation());

    crabe::infrastructure::LuaCall::get().runSnippet(L,
        "if Crabe and Crabe.Mod and Crabe.Mod.reload then "
        "    Crabe.Mod.reload() "
        "end");

    std::vector<std::string> owners;
    if (firstOfGeneration) {
        std::lock_guard<std::mutex> lock(_mutex);
        owners.reserve(_mods.size());
        for (const Mod& mod : _mods)
            owners.push_back(mod.id);
    }

    std::size_t revokedHooks = 0;
    for (const std::string& owner : owners)
        revokedHooks += crabe::infrastructure::coreRegistry().removeAllOwnedBy(owner);
    if (revokedHooks != 0) {
        crabe::shared::Logger::getInstance().info(
            "ModManager: revoked {} mod-owned hook(s) across {} mod(s) before reloading.",
            revokedHooks, owners.size());
    }

    auto targetFolder = _modsFolder.empty()
        ? (std::filesystem::current_path() / "mods")
        : _modsFolder;

    discoverAndLoadMods(L, targetFolder);

    const std::size_t behind = _reloads.statesBehind();
    if (behind != 0) {
        crabe::shared::Logger::getInstance().info(
            "ModManager: {} other tracked state(s) still behind generation {}; each reloads "
            "itself the next time it runs.",
            behind, _reloads.generation());
    }
}

// Records one frame of mod ImGui calls on the script thread.
void ModManager::dispatchDraw(void* L)
{
    if (!L || !crabe::application::Loader::get().isRuntimeReady())
        return;

    crabe::presentation::DrawBuffer& buffer = crabe::presentation::DrawBuffer::get();

    buffer.beginFrame();
    crabe::infrastructure::LuaCall::get().dispatchModDraw(L);
    buffer.endFrame();
}

} // namespace crabe::domain
