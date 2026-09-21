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

// What one resolveAll pass found.
//
// The count exists because the loader has to answer a question the addresses
// alone cannot: whether *every* symbol was found. That is what decides between
// degraded mode and refusing outright on a build no profile describes -- see
// crabe::domain::decideLoad.
struct Resolution {
    crabe::infrastructure::LuaApiAddresses addresses{};
    unsigned resolved = 0;
    unsigned total = 0;

    [[nodiscard]] bool allResolved() const noexcept { return total != 0 && resolved == total; }
};

// Every Lua C API address the loader needs, resolved against the module base.
// A field left at 0 did not resolve; LuaCall::initialize decides which of
// those are fatal.
//
// Resolution is by signature scan throughout: the Lua stdlib registration
// tables are walked for each wrapper name, and the wrapper's n-th `call` is the
// candidate. `profile` is the confirmation, not the search -- when a profile
// matched, a candidate that does not land on the RVA that profile states is
// refused rather than hooked (invariant I8). `profile` is null in degraded mode,
// where there is no RVA to confirm against and the first candidate is taken and
// logged as unverified.
Resolution resolveAll(uintptr_t base, const crabe::domain::GameProfile* profile);

}

#endif /* !LUA_SYMBOLS_HPP_ */
