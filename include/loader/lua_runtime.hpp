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

    // Runs every api/*.lua in filename order, which is dependency order.
    // Returns false if any of them failed; failures are logged individually,
    // and a failing module does not stop the ones after it (a broken helper
    // should not cost the whole API).
    bool injectAll(void* L);
    std::filesystem::path apiFolder();

    // Registers every real C++ native (as opposed to the plain Lua wrappers
    // in api/*.lua, which just call the game's own natives). Called after
    // injectAll(L), since it needs the Crabe table injectAll just created,
    // and before onLoadmods(), so mods see the natives already in place.
    bool registerNatives(void* L);
}

#endif /* !LUA_RUNTIME_HPP_ */
