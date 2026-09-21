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
#include "presentation/draw_buffer.hpp"
#include "shared/logger.hpp"
#include "shared/version.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace crabe::domain {

// Mod names and paths come from whatever an archive unpacked into mods/, so
// they are attacker-controlled. Both chunks below are compile-time constants
// and receive every such value as a `...` argument: a directory called
// `Bob's mod` is then a plain string, and one called `x'); os.exit() --` is
// still a plain string instead of a second statement running outside the
// sandbox.

static constexpr std::string_view kSetPackagePathChunk = R"LUA(
    local root = ...
    package.path = root .. '/?.lua;' .. root .. '/modules/?.lua;' .. package.path
)LUA";

// setCurrentOwner brackets the chunk so every subscription it registers is
// attributed to this mod and revoked on the next hot reload (Crabe.Registry,
// src/api/02b_registry.lua). The owner is cleared on both exits, so a mod that
// fails to load never leaves the next one registering under its name.
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

    // The v0 id gap, and the one place this loader decides what to do about
    // it -- manifestVersion 0 has no "id" key, so ModManifest::getId() is
    // empty for every mod that shipped before T8 and for every mod with no
    // mod.json at all, and the dependency resolver is keyed on id throughout.
    // Naming them ("local.<sanitised-folder-name>", numbered apart on
    // collision) is policy, not something the resolver or ModManifest decide
    // -- see domain/mod_id.hpp, which T16 pulled this out into so that
    // crabe-cli's `resolve` command computes exactly the id ModManager would,
    // rather than a guess that can drift out of step with it.

    // One mod as it was found on disk, before anything has been run.
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

// Queries whether a hot reload cycle has been requested.
bool ModManager::isHotReloadRequested() const noexcept
{
    return _hotReloadRequested.load();
}

// Flags a hot reload request to be processed on the next available cycle.
void ModManager::requestHotReload() noexcept
{
    _hotReloadRequested.store(true);
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
//
// Whether this mod should load at all was settled before the call: the manifest
// was parsed during discovery and the resolver ruled on it. This runs it.
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

    // A folder with nothing to run used to return true and be reported as
    // `loaded in 0.1 ms`, which is how mods/crabe_heroes -- whose only Lua sat
    // in a characters/ subdirectory -- went a whole play session looking green
    // while never executing a line. Nothing downstream can detect that: the
    // sandbox is never entered, so there is no error to propagate. It has to be
    // refused here or not at all.
    if (plan.runsNothing()) {
        logger.error("ModManager: mod '{}' ran nothing -- no entry script, and no .lua file at "
                     "the top level of '{}'. Looked for {}. Lua in a subdirectory is a module "
                     "tree, not an entry point: add a main.lua that requires it, or name the "
                     "entry script with \"entry\" in mod.json.",
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
            // The load report is the one INFO line per mod; this names the file
            // that ran, which only matters when something went wrong.
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

// Discovers and loads mods from the given directory.
//
// Five steps, in this order: find the candidates, name them, resolve them, run
// them in the order that came back, report. The order used to be whatever
// std::filesystem::directory_iterator yielded, which the standard does not
// specify and which nothing could configure.
void ModManager::discoverAndLoadMods(void* L, const std::filesystem::path& modsFolder)
{
    if (!L)
        return;

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

    // ---- 1. Discover, parsing each mod.json exactly once. ----
    std::vector<Candidate> candidates;

    for (const auto& entry : std::filesystem::directory_iterator(modsFolder, ec)) {
        if (entry.is_directory()) {
            std::string folderName = entry.path().filename().string();
            // Unchanged: a leading '.' or '_' marks a directory that is not a
            // mod (mods/_template is a worked example, not something to run).
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

    // ---- 2. Report what the manifests say about themselves. ----
    for (Candidate& candidate : candidates) {
        if (candidate.manifest.isMalformed() && candidate.manifest.diagnostic().has_value()) {
            logger.error("ModManager: mod '{}' has an unusable mod.json, ignoring it: {}",
                         candidate.name, candidate.manifest.diagnostic()->what());
            // Unchanged behaviour, deliberately: the mod then loads as though it
            // had no manifest at all, because refusing it would be a stricter
            // contract than the one mods were written against. Handing the
            // malformed manifest to the resolver would refuse it -- which is the
            // right answer for a resolver and the wrong one here -- so it is
            // replaced by the absent state it falls back to.
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

    // ---- 2b. Filter by the active profile (T11). A provisional id only --
    //          the declared one, or the same unnumbered synthesiseModId()
    //          step 3 would assign for real -- because a collision between
    //          two mods a profile drops on the floor never needs
    //          disambiguating. Filtering happens here, before naming and
    //          before resolution: a disabled mod is simply not a candidate
    //          from this point on, never a rejection the resolver reports.
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

    // ---- 3. Name every mod, via the same pure rule crabe-cli's `resolve`
    //         command applies to the same folder (domain/mod_id.hpp).
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

    // ---- 4. Resolve. Pure, so everything above this line is what decides the
    //         answer -- and everything below it is bookkeeping.
    std::vector<ResolverInput> inputs;
    inputs.reserve(candidates.size());
    for (const Candidate& candidate : candidates)
        inputs.push_back(ResolverInput{ candidate.id, &candidate.manifest });

    const SemVer loaderVersion(crabe::version::Major, crabe::version::Minor,
                               crabe::version::Patch);
    const ResolutionResult resolution = resolve(inputs, loaderVersion);

    // ---- 5. Run, in order, and report one line per mod. ----
    logger.info("ModManager: {} mod(s) found in '{}': {} to load, {} rejected (loader v{}).",
                candidates.size(), modsFolder.string(), resolution.loadOrder.size(),
                resolution.rejected.size(), crabe::version::String);

    std::vector<bool> ran(candidates.size(), false);
    _mods.reserve(resolution.loadOrder.size());

    // The reporter's own mod list is rebuilt here rather than read from _mods
    // at crash time: a handler must not walk a std::vector of std::string that
    // another thread may be resizing, and domain::Mod carries no version to
    // report in the first place.
    crabe::infrastructure::CrashReporter::forgetMods();

    for (std::size_t position = 0; position < resolution.loadOrder.size(); ++position) {
        const std::size_t index = resolution.sourceIndex[position];
        Candidate& candidate = candidates[index];
        ran[index] = true;

        // This is the one place in C++ where the running mod is actually
        // known. Once control is inside Lua, Crabe.Mod.dispatchDraw() picks
        // the mod and the loader cannot see which -- so a fault during a draw
        // is attributed to the hook, not to a mod, and that is honest.
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

    // Each rejection is matched back to the copy on disk it came from, so the
    // line names a folder the reader can go and look at. A duplicated id has
    // several rejections and several copies, so they are consumed in step and
    // each line names a different folder.
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

    _hotReloadRequested.store(false);
    crabe::shared::Logger::getInstance().info("ModManager: reloading all mods...");

    crabe::infrastructure::LuaCall::get().runSnippet(L,
        "if Crabe and Crabe.Mod and Crabe.Mod.reload then "
        "    Crabe.Mod.reload() "
        "end");

    // Lua-side subscriptions are revoked by the chunk above; native hooks are
    // not, because nothing in Lua knows about them. A hook left installed
    // across a reload points its detour at a closure the new Lua state does not
    // contain, which is a crash on the next call rather than a stale callback.
    //
    // Nothing owns a hook yet -- mods reach the registry only once T17 lands --
    // so today every call below returns 0. The wiring is here rather than
    // waiting for the first owner precisely so that it cannot be forgotten
    // then: a mod that installs a hook must not be the change that also has to
    // remember to tear it down.
    //
    // The ids are copied out under the lock and the registry is called without
    // it: discoverAndLoadMods below takes _mutex for its whole body, and
    // holding it across a call into another subsystem's lock is how an ordering
    // problem gets built for a later thread to find.
    std::vector<std::string> owners;
    {
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
}

// Records one frame of mod ImGui calls on the script thread. Nothing is drawn
// here: the commands are replayed later by RenderHook inside Present, which is
// the only place allowed to touch Direct3D (Architecture Blueprint, Rule 3).
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
