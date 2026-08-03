/*
** CrabeLoader
** File description:
** lua_runtime
*/

#ifndef LUA_RUNTIME_HPP_
#define LUA_RUNTIME_HPP_

#include <filesystem>

// The modding API CrabeLoader exposes to mods: the `Crabe` namespace
namespace LuaRuntime {
    // How usable a given lua_State is. The process runs more than one: the
    // first the loader sees belongs to the Slang shader compiler, and the
    // earliest states of any kind are seen before luaopen_base has filled _G.
    enum class StateKind {
        Unusable,   // no base library yet (`type`, `pairs`... still nil)
        NotTheGame, // base library is up, but none of the game's natives are
        Game,       // the script state the API and the mods belong in
    };

    StateKind classifyState(void* L);

    // Runs every api/*.lua in filename order (dependency order). A failing
    // module is logged and skipped, not fatal to the rest.
    bool injectAll(void* L);
    std::filesystem::path apiFolder();

    // Registers the real C++ natives. Must run after injectAll (needs the
    // Crabe table) and before onLoadmods (mods expect natives in place).
    bool registerNatives(void* L);
}

#endif /* !LUA_RUNTIME_HPP_ */
