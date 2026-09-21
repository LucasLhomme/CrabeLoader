#include "domain/mod_manager.hpp"
#include "domain/mod_manifest.hpp"
#include "application/loader.hpp"
#include "infrastructure/lua_call.hpp"
#include "presentation/draw_buffer.hpp"
#include "shared/logger.hpp"
#include "shared/version.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <mutex>
#include <string_view>

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
    const std::string& modName) const
{
    crabe::domain::ModManifest manifest(modPath / "mod.json");
    if (manifest.isValid() && !manifest.getEntry().empty())
        return modPath / manifest.getEntry();

    auto mainScript = modPath / "main.lua";
    if (std::filesystem::exists(mainScript))
        return mainScript;

    return modPath / (modName + ".lua");
}

// Loads a modular directory into an isolated sandbox environment.
void ModManager::loadModDirectory(void* L, const std::filesystem::path& modPath)
{
    std::string modName = modPath.filename().string();
    if (modName.empty() || modName[0] == '.' || modName[0] == '_')
        return;

    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
    crabe::domain::ModManifest manifest(modPath / "mod.json");
    if (manifest.isValid() && !manifest.isCompatible()) {
        logger.warning("ModManager: mod '{}' requires CrabeLoader v{}, current is v{}.",
                       modName, manifest.getMinLoaderVersion(), crabe::version::String);
        return;
    }

    const std::array<std::string, 1> pathArgs{ modPath.generic_string() };
    std::string pathError;
    if (!crabe::infrastructure::LuaCall::get().runChunkWithArgs(
            L, kSetPackagePathChunk, pathArgs, pathError)) {
        logger.error("ModManager: mod '{}': could not extend the Lua module search path: {}",
                     modName, pathError);
    }

    auto entryScript = resolveEntryScript(modPath, modName);
    bool loaded = false;

    if (std::filesystem::exists(entryScript)) {
        const std::array<std::string, 2> loadArgs{ entryScript.generic_string(), modName };

        std::string outError;
        loaded = crabe::infrastructure::LuaCall::get().runChunkWithArgs(
            L, kLoadModChunk, loadArgs, outError);
        if (!loaded) {
            logger.error("ModManager: mod '{}' failed to run: {}", modName, outError);
        } else {
            logger.info("ModManager: mod '{}' loaded (entry: {}).",
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
        logger.info("ModManager: mod directory '{}' loaded.", modName);
    }

    _mods.push_back(Mod{
        .name = modName,
        .rootPath = modPath,
        .isLoaded = loaded
    });
}

// Executes a standalone Lua mod script in an isolated sandbox.
void ModManager::loadStandaloneScript(void* L, const std::filesystem::path& scriptPath)
{
    std::string modName = scriptPath.stem().string();
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

    const std::array<std::string, 2> loadArgs{ scriptPath.generic_string(), modName };

    std::string outError;
    bool loaded = crabe::infrastructure::LuaCall::get().runChunkWithArgs(
        L, kLoadModChunk, loadArgs, outError);
    if (loaded) {
        logger.info("ModManager: standalone mod '{}' executed.", scriptPath.filename().string());
    } else {
        logger.error("ModManager: standalone mod '{}' failed: {}",
                     scriptPath.filename().string(), outError);
    }

    _mods.push_back(Mod{
        .name = modName,
        .rootPath = scriptPath,
        .isLoaded = loaded
    });
}

// Discovers and loads mods from the given directory.
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

    for (const auto& entry : std::filesystem::directory_iterator(modsFolder, ec)) {
        if (entry.is_directory()) {
            loadModDirectory(L, entry.path());
        } else if (entry.is_regular_file() && entry.path().extension() == ".lua") {
            loadStandaloneScript(L, entry.path());
        }
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
