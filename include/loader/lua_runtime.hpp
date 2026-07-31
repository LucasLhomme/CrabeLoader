/*
** CrabeLoader
** File description:
** lua_runtime
*/

#ifndef LUA_RUNTIME_HPP_
#define LUA_RUNTIME_HPP_

#include <cstddef>

// The modding API CrabeLoader exposes to mods (the global `Game` table, the
// console output bridge, the per-frame hook).
//
// Injected into the game's own lua_State right before the mods are loaded, so
// `Game` already exists by the time any mod runs.
namespace LuaRuntime {
    // Injects every module, in dependency order. Returns false if any of them
    // failed; failures are logged individually, and a failing module does not
    // stop the ones after it (a broken helper should not cost the whole API).
    bool injectAll(void* L);

    // Lua source of the modules, in injection order. Exposed for the log line
    // that names a failing module, not meant to be run directly.
    const char* moduleName(size_t index);
    const char* moduleSource(size_t index);
    size_t moduleCount();
}

#endif /* !LUA_RUNTIME_HPP_ */
