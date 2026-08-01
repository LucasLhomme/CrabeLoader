/*
** CrabeLoader
** File description:
** lua_runtime
*/

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <vector>

#include "loader/lua_runtime.hpp"
#include "loader/luacall.hpp"
#include "loader/render_hook.hpp"
#include "logger/logger.hpp"

namespace {

    constexpr const char* kApiFolderName = "api";

    // The only real C++ native so far. No native anywhere in the game exposes
    // window/fullscreen state to Lua (see docs/nativedb.md), so this is a pure
    // Win32 concern -- RenderHook is the only thing that can do it, and it must
    // happen on the render thread, not here (this runs on whichever thread owns
    // L when the game's script code calls it), hence just posting a request.
    int __cdecl nativeSetWindowMode(void* L)
    {
        const char* mode = LuaCall::get().argToString(L, 1);
        bool borderless = mode && std::strcmp(mode, "borderless") == 0;

        RenderHook::get().requestWindowMode(borderless ? WindowMode::BorderlessWindowed : WindowMode::Windowed);
        return 0; // no Lua return values
    }

} // namespace

std::filesystem::path LuaRuntime::apiFolder()
{
    return std::filesystem::current_path() / kApiFolderName;
}

LuaRuntime::StateKind LuaRuntime::classifyState(void* L)
{
    // Must stay answerable in a state where none of the globals it tests exist.
    // The natives are what tell the game apart from the shader compiler's
    // state, which has a full base library and none of them.
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

    // Dependency order is the numeric prefixes; directory iteration order is
    // not specified.
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

bool LuaRuntime::registerNatives(void* L)
{
    // Registered under an underscore-prefixed name: it is the raw native,
    // wrapped by the ergonomic Crabe.SetWindowMode/GetWindowMode pair in
    // src/api/05_window.lua, same as every other internal detail in Crabe.
    bool ok = LuaCall::get().registerNativeFunction(L, "Crabe", "_setWindowModeNative", &nativeSetWindowMode);
    if (!ok)
        Logger::getInstance().error("LuaRuntime: failed to register Crabe._setWindowModeNative.");
    return ok;
}
