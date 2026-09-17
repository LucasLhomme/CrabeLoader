#include "domain/ModManager.hpp"
#include "domain/ModManifest.hpp"
#include "infrastructure/luacall.hpp"
#include "shared/logger.hpp"
#include "shared/version.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <mutex>

namespace Crabe::Domain {

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
    Crabe::Domain::ModManifest manifest(modPath / "mod.json");
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

    Logger& logger = Logger::getInstance();
    Crabe::Domain::ModManifest manifest(modPath / "mod.json");
    if (manifest.isValid() && !manifest.isCompatible()) {
        logger.warning("ModManager: mod '{}' requires CrabeLoader v{}, current is v{}.",
                       modName, manifest.getMinLoaderVersion(), Crabe::Version::String);
        return;
    }

    LuaCall::get().runSnippet(L, std::format(
        "package.path = '{0}/?.lua;{0}/modules/?.lua;' .. package.path",
        modPath.generic_string()));

    auto entryScript = resolveEntryScript(modPath, modName);
    bool loaded = false;

    if (std::filesystem::exists(entryScript)) {
        std::string sandboxSnippet = std::format(
            "if Crabe and Crabe.Sandbox and Crabe.Sandbox.create then\n"
            "    local env = Crabe.Sandbox.create('{1}')\n"
            "    local chunk, err = loadfile('{0}')\n"
            "    if chunk then\n"
            "        setfenv(chunk, env)\n"
            "        local ok, runErr = pcall(chunk)\n"
            "        if not ok then error(runErr) end\n"
            "    else\n"
            "        error(err)\n"
            "    end\n"
            "else\n"
            "    local chunk, err = loadfile('{0}')\n"
            "    if chunk then\n"
            "        local ok, runErr = pcall(chunk)\n"
            "        if not ok then error(runErr) end\n"
            "    else\n"
            "        error(err)\n"
            "    end\n"
            "end",
            entryScript.generic_string(), modName);

        std::string outError;
        loaded = LuaCall::get().runSnippet(L, sandboxSnippet, outError);
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
                if (!LuaCall::get().runFile(L, file.path().string().c_str()))
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
    Logger& logger = Logger::getInstance();

    std::string sandboxSnippet = std::format(
        "if Crabe and Crabe.Sandbox and Crabe.Sandbox.create then\n"
        "    local env = Crabe.Sandbox.create('{1}')\n"
        "    local chunk, err = loadfile('{0}')\n"
        "    if chunk then\n"
        "        setfenv(chunk, env)\n"
        "        local ok, runErr = pcall(chunk)\n"
        "        if not ok then error(runErr) end\n"
        "    else\n"
        "        error(err)\n"
        "    end\n"
        "else\n"
        "    local chunk, err = loadfile('{0}')\n"
        "    if chunk then\n"
        "        local ok, runErr = pcall(chunk)\n"
        "        if not ok then error(runErr) end\n"
        "    else\n"
        "        error(err)\n"
        "    end\n"
        "end",
        scriptPath.generic_string(), modName);

    std::string outError;
    bool loaded = LuaCall::get().runSnippet(L, sandboxSnippet, outError);
    if (loaded) {
        logger.info("ModManager: standalone mod '{}' executed.", scriptPath.filename().string());
    } else {
        logger.warning("ModManager: standalone mod '{}' failed: {}",
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

    Logger& logger = Logger::getInstance();
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

    LuaCall::get().runSnippet(L,
        "if Crabe and Crabe.Mod and Crabe.Mod.dispatchInit then "
        "    Crabe.Mod.dispatchInit() "
        "end");

    LuaCall::get().runSnippet(L,
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
    Logger::getInstance().info("ModManager: reloading all mods...");

    LuaCall::get().runSnippet(L,
        "if Crabe and Crabe.Mod and Crabe.Mod.reload then "
        "    Crabe.Mod.reload() "
        "end");

    auto targetFolder = _modsFolder.empty()
        ? (std::filesystem::current_path() / "mods")
        : _modsFolder;

    discoverAndLoadMods(L, targetFolder);
}

// Dispatches per-frame ImGui draw callbacks to all registered mods.
// Deprecated: calling into Lua from the render thread causes VM race conditions.
void ModManager::dispatchDraw([[maybe_unused]] void* L)
{
}

} // namespace Crabe::Domain
