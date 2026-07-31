/*
** CrabeLoader
** File description:
** lua_runtime
*/

#include <algorithm>
#include <filesystem>
#include <vector>

#include "loader/lua_runtime.hpp"
#include "loader/luacall.hpp"
#include "logger/logger.hpp"

namespace {

    // Where the loader's own Lua lives, next to the game executable. Kept apart
    // from mods/ on purpose: this is the loader talking, not a user mod.
    constexpr const char* kApiFolderName = "api";

} // namespace

std::filesystem::path LuaRuntime::apiFolder()
{
    return std::filesystem::current_path() / kApiFolderName;
}

LuaRuntime::StateKind LuaRuntime::classifyState(void* L)
{
    // Nothing but global lookups and concatenation: this has to be answerable
    // in a state where none of the globals it asks about exist yet. It stays a
    // C++ string rather than an api/ file because it runs *before* the decision
    // to load any of them.
    //
    // The natives are what tell the game's script state apart from the shader
    // compiler's, which has a perfectly good base library and none of these.
    static constexpr const char* kProbe = R"lua(
if not (type and pairs and tostring and table and pcall and error) then return 'unusable' end
if not (UI_GetSparks and Players_GetHostPlayerID) then return 'notgame' end
return 'game'
)lua";

    std::string result;
    if (!LuaCall::get().runSnippet(L, kProbe, result))
        return StateKind::Unusable;

    if (result == "game") return StateKind::Game;
    if (result == "notgame") return StateKind::NotTheGame;
    return StateKind::Unusable;
}

bool LuaRuntime::injectAll(void* L)
{
    Logger& logger = Logger::getInstance();
    std::filesystem::path folder = apiFolder();

    if (!std::filesystem::exists(folder)) {
        logger.error("LuaRuntime: '{}' not found; the API is unavailable and every mod will fail.",
                    folder.string());
        return false;
    }

    std::vector<std::filesystem::path> modules;
    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.is_regular_file() && entry.path().extension() == ".lua")
            modules.push_back(entry.path());
    }

    // Load order is dependency order, encoded in the filenames' numeric
    // prefixes (00_core, 10_game, ...). Directory iteration order is not
    // specified, so it is sorted here rather than trusted.
    std::sort(modules.begin(), modules.end());

    if (modules.empty()) {
        logger.error("LuaRuntime: no .lua module in '{}'; the API is unavailable.", folder.string());
        return false;
    }

    bool allOk = true;
    for (const auto& path : modules) {
        // Keep going: a broken module costs its own features, not the API.
        if (LuaCall::get().runFile(L, path.string().c_str())) continue;

        logger.error("LuaRuntime: API module '{}' failed to load.", path.filename().string());
        allOk = false;
    }

    if (allOk) logger.info("LuaRuntime: API loaded ({} modules from {}/).", modules.size(), kApiFolderName);
    return allOk;
}
