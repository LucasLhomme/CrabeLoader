#include "domain/mod_manager.hpp"
#include "domain/dependency_resolver.hpp"
#include "domain/mod_manifest.hpp"
#include "application/loader.hpp"
#include "infrastructure/lua_call.hpp"
#include "presentation/draw_buffer.hpp"
#include "shared/logger.hpp"
#include "shared/version.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
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

    // -----------------------------------------------------------------------
    // The v0 id gap, and the one place this loader decides what to do about it.
    //
    // manifestVersion 0 has no "id" key -- it predates the schema -- so
    // ModManifest::getId() is empty for every mod that shipped before T8, and
    // for every mod that has no mod.json at all. The dependency resolver is
    // keyed on id from end to end, so refusing those mods would undo the
    // backward compatibility T8 was written to guarantee.
    //
    // ModManager therefore names them itself: "local." followed by the folder
    // (or script) name lowercased, with every character outside [a-z0-9] folded
    // to '-', runs of '-' collapsed, and the ends trimmed. The result satisfies
    // ^[a-z0-9]+(\.[a-z0-9-]+)+$ -- what validateModId() asks for -- and the
    // "local." prefix cannot collide with the reverse-DNS id a real manifest
    // would declare.
    //
    // This is policy, and it lives here rather than in the resolver (which is
    // handed ids and stays deliberately ignorant of where they came from) or in
    // ModManifest, whose getId() says at its own declaration that it does not
    // decide this.
    //
    // Three awkward cases, all handled by the caller or by the last line here:
    //
    //  * a name that sanitises to nothing ("!!!", or a script in a non-Latin
    //    alphabet this byte-wise fold cannot transliterate) becomes
    //    "local.unnamed";
    //  * two folders that sanitise to the same id ("My Mod" and "My-Mod") are
    //    numbered apart by discoverAndLoadMods, which also warns, because the
    //    author needs to know the name they would write in a dependency list is
    //    not the one this mod answers to;
    //  * a synthesised id that lands on an id some manifest actually declares
    //    loses -- declared ids are assigned first and are never renamed.
    // -----------------------------------------------------------------------
    [[nodiscard]] std::string synthesiseModId(std::string_view folderName)
    {
        std::string suffix;
        suffix.reserve(folderName.size() + 1);

        for (const char character : folderName) {
            if ((character >= 'a' && character <= 'z') || (character >= '0' && character <= '9')) {
                suffix += character;
            } else if (character >= 'A' && character <= 'Z') {
                suffix += static_cast<char>(character - 'A' + 'a');
            } else if (!suffix.empty() && suffix.back() != '-') {
                suffix += '-';
            }
        }
        while (!suffix.empty() && suffix.back() == '-')
            suffix.pop_back();

        if (suffix.empty())
            suffix = "unnamed";

        return "local." + suffix;
    }

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

// Resolves the primary Lua entry script for a given mod directory.
std::filesystem::path ModManager::resolveEntryScript(
    const std::filesystem::path& modPath,
    const std::string& modName,
    const ModManifest& manifest) const
{
    if (manifest.isValid() && !manifest.getEntry().empty())
        return modPath / manifest.getEntry();

    auto mainScript = modPath / "main.lua";
    if (std::filesystem::exists(mainScript))
        return mainScript;

    return modPath / (modName + ".lua");
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

    auto entryScript = resolveEntryScript(modPath, modName, manifest);
    bool loaded = false;

    if (std::filesystem::exists(entryScript)) {
        const std::array<std::string, 2> loadArgs{ entryScript.generic_string(), modName };

        std::string outError;
        loaded = crabe::infrastructure::LuaCall::get().runChunkWithArgs(
            L, kLoadModChunk, loadArgs, outError);
        if (!loaded) {
            logger.error("ModManager: mod '{}' failed to run: {}", modName, outError);
        } else {
            // The load report is the one INFO line per mod; this names the file
            // that ran, which only matters when something went wrong.
            logger.debug("ModManager: mod '{}' ran entry '{}'.",
                         modName, entryScript.filename().string());
        }
    } else {
        bool anyFailed = false;
        for (const auto& file : std::filesystem::directory_iterator(modPath)) {
            if (file.is_regular_file() && file.path().extension() == ".lua") {
                if (!crabe::infrastructure::LuaCall::get().runFile(L, file.path().string().c_str()))
                    anyFailed = true;
            }
        }
        loaded = !anyFailed;
        logger.debug("ModManager: mod directory '{}' ran every .lua file it holds.", modName);
    }

    return loaded;
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

    // ---- 3. Name every mod. Declared ids go first and are never renamed, so a
    //         synthesised one can never take a name a manifest actually claims.
    std::set<std::string> takenIds;
    for (Candidate& candidate : candidates) {
        if (candidate.manifest.isValid() && !candidate.manifest.getId().empty()) {
            candidate.id = candidate.manifest.getId();
            takenIds.insert(candidate.id);
        }
    }

    // Synthesised in path order rather than in directory_iterator order: when
    // two folders sanitise to the same id the numbering below has to land on the
    // same folder every time, and the filesystem promises nothing about the
    // order it hands them over in.
    std::vector<std::size_t> unnamed;
    for (std::size_t index = 0; index < candidates.size(); ++index) {
        if (candidates[index].id.empty())
            unnamed.push_back(index);
    }
    std::sort(unnamed.begin(), unnamed.end(), [&candidates](std::size_t lhs, std::size_t rhs) {
        return candidates[lhs].path.generic_string() < candidates[rhs].path.generic_string();
    });

    for (std::size_t index : unnamed) {
        const std::string wanted = synthesiseModId(candidates[index].name);
        std::string chosen = wanted;
        for (unsigned int attempt = 2; takenIds.count(chosen) != 0; ++attempt)
            chosen = std::format("{}-{}", wanted, attempt);

        if (chosen != wanted) {
            logger.warning("ModManager: mod '{}' would be called '{}', which is already taken; "
                           "calling it '{}' instead. Rename the folder, or give it a mod.json with "
                           "an \"id\", if anything needs to depend on it by name.",
                           candidates[index].name, wanted, chosen);
        }
        takenIds.insert(chosen);
        candidates[index].id = std::move(chosen);
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

    for (std::size_t position = 0; position < resolution.loadOrder.size(); ++position) {
        const std::size_t index = resolution.sourceIndex[position];
        Candidate& candidate = candidates[index];
        ran[index] = true;

        const auto startedAt = std::chrono::steady_clock::now();
        const bool loaded = candidate.isDirectory
            ? loadModDirectory(L, candidate.path, candidate.name, candidate.manifest)
            : loadStandaloneScript(L, candidate.path, candidate.name);
        const double elapsedMs = std::chrono::duration<double, std::milli>(
                                     std::chrono::steady_clock::now() - startedAt)
                                     .count();

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
