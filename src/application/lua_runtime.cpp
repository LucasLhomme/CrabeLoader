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
#include "application/embedded_api.hpp"

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

    // 1. Developer override: If an 'api/' directory exists on disk and contains .lua files,
    // load from disk so developers can test modifications without recompiling the DLL.
    if (std::filesystem::exists(folder)) {
        std::vector<std::filesystem::path> diskModules;
        for (const auto& entry : std::filesystem::directory_iterator(folder)) {
            if (entry.is_regular_file() && entry.path().extension() == ".lua")
                diskModules.push_back(entry.path());
        }

        if (!diskModules.empty()) {
            std::sort(diskModules.begin(), diskModules.end());
            logger.info("LuaRuntime: Dev override active -- loading {} module(s) from '{}'.",
                        diskModules.size(), folder.string());

            bool allOk = true;
            for (const auto& path : diskModules) {
                if (LuaCall::get().runFile(L, path.string().c_str()))
                    continue;

                logger.error("LuaRuntime: API module '{}' failed to load from disk.", path.filename().string());
                allOk = false;
            }

            if (allOk)
                logger.info("LuaRuntime: Disk API override loaded successfully.");
            return allOk;
        }
    }

    // 2. Standard release: Load all embedded API modules directly from DLL memory.
    logger.info("LuaRuntime: Loading embedded API ({} modules)...", Crabe::EmbeddedApi::kModuleCount);

    bool allOk = true;
    for (const auto& mod : Crabe::EmbeddedApi::kModules) {
        if (LuaCall::get().runBuffer(L, reinterpret_cast<const char*>(mod.data), mod.size, mod.name.data()))
            continue;

        logger.error("LuaRuntime: Embedded API module '{}' failed to load.", mod.name);
        allOk = false;
    }

    if (allOk) {
        logger.info("LuaRuntime: Embedded API loaded successfully ({} modules).",
                    Crabe::EmbeddedApi::kModuleCount);
    }
    return allOk;
}
