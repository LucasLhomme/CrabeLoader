/*
** CrabeLoader
** File description:
** lua_runtime
*/

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <format>
#include <string>
#include <vector>
#include <windows.h>

#include "infrastructure/avatar_relay_hook.hpp"
#include "presentation/input_hook.hpp"
#include "application/loader.hpp"
#include "infrastructure/message_hook.hpp"
#include "application/lua_runtime.hpp"
#include "infrastructure/luacall.hpp"
#include "infrastructure/memory.hpp"
#include "presentation/render_hook.hpp"
#include "shared/logger.hpp"

namespace {

    constexpr const char* kApiFolderName = "api";

} // namespace

std::filesystem::path LuaRuntime::apiFolder()
{
    return std::filesystem::current_path() / kApiFolderName;
}

// Must stay answerable in a state where none of the globals it tests exist:
// the natives are what tell the game apart from the shader compiler's state,
// which has a full base library and none of them.
LuaRuntime::StateKind LuaRuntime::classifyState(void* L)
{
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

    std::sort(modules.begin(), modules.end());

    if (modules.empty()) {
        logger.error("LuaRuntime: no .lua module in '{}'; the API is unavailable.", folder.string());
        return false;
    }

    bool allOk = true;
    for (const auto& path : modules) {
        if (LuaCall::get().runFile(L, path.string().c_str())) continue;

        logger.error("LuaRuntime: API module '{}' failed to load.", path.filename().string());
        allOk = false;
    }

    if (allOk) logger.info("LuaRuntime: API loaded ({} modules from {}/).", modules.size(), kApiFolderName);
    return allOk;
}
