/*
** CrabeLoader
** File description:
** Declares one pass of locating the game Lua C API inside the running image.
** The count matters as much as the addresses: it separates degraded mode from refusing to load.
** Makes neither decision itself; decideLoad in domain/game_profile.hpp does.
**
** Authors: @LucasLhomme
*/

#ifndef LUA_SYMBOLS_HPP_
#define LUA_SYMBOLS_HPP_

#include <cstdint>

#include "domain/game_profile.hpp"
#include "infrastructure/lua_call.hpp"

namespace crabe::lua_symbols {

struct Resolution {
    crabe::infrastructure::LuaApiAddresses addresses{};
    unsigned resolved = 0;
    unsigned total = 0;

    // Checks whether all symbols were successfully resolved.
    [[nodiscard]] bool allResolved() const noexcept { return total != 0 && resolved == total; }
};

// Resolves required Lua C API addresses against module base and optional game profile.
// Validates candidate RVAs against the profile if provided.
Resolution resolveAll(uintptr_t base, const crabe::domain::GameProfile* profile);

} // namespace crabe::lua_symbols

#endif /* !LUA_SYMBOLS_HPP_ */
