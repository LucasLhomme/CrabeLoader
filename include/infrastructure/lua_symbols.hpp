/*
** CrabeLoader
** File description:
** lua_symbols -- locating the game's Lua C API
*/

#ifndef LUA_SYMBOLS_HPP_
#define LUA_SYMBOLS_HPP_

#include <cstdint>

#include "infrastructure/lua_call.hpp"

namespace crabe::lua_symbols {

// Every Lua C API address the loader needs, resolved against the module base.
// A field left at 0 did not resolve; LuaCall::initialize decides which of
// those are fatal.
crabe::infrastructure::LuaApiAddresses resolveAll(uintptr_t base);

}

#endif /* !LUA_SYMBOLS_HPP_ */
