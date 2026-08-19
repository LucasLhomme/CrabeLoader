/*
** CrabeLoader
** File description:
** cheats
*/

#ifndef CHEATS_HPP_
#define CHEATS_HPP_

#include <cstdint>

// Invulnerability and movement speed, both driven by manual code caves on
// mid-function SSE sites (see codecave.hpp for why not MinHook).
//
// God mode is two caves: one on the HUD health read captures the player object
// (the only entity whose maxHealth reaches 150), one on the damage-apply site
// refuses any write that would lower that object's health. Enemies keep taking
// damage because only the captured pointer is clamped.
//
// Speed is four caves, one per analog-stick-to-velocity store, each inserting
// `mulss xmm0, [multiplier]` ahead of the original store.
//
// Both are installed lazily on first use: until the player asks for a cheat,
// the game's code is left untouched.
namespace Cheats {

    // Scans and installs the god-mode caves. Idempotent; false means an AOB
    // did not resolve (game version mismatch) and nothing was patched.
    bool installGodMode();

    // Installs the caves if needed, then gates the damage clamp. False if the
    // caves are unavailable, in which case the state is not changed.
    bool setGodMode(bool enabled);
    bool godMode();

    // Player object captured by the god-mode cave, or 0 before the HUD has
    // drawn a health bar once. Note this needs installGodMode() to have run.
    uintptr_t playerObject();

    // Float at [playerObject() + offset]. False when no object is captured or
    // the read would fault. Used to probe the entity struct from the console.
    bool readPlayerFloat(uintptr_t offset, float& out);

    // Scans and installs the four velocity caves. Idempotent.
    bool installSpeed();

    // Installs the caves if needed, then sets the velocity multiplier
    // (1.0 = stock). Clamped to a sane range; false if the caves failed.
    bool setSpeedMultiplier(float multiplier);
    float speedMultiplier();

} // namespace Cheats

// Lua C functions for the above. They live with the feature rather than in
// lua_natives.cpp only because that file is already near the 500-line cap.
// The name table stays where every other native is registered, in the
// registerNatives method of LuaRuntime.
namespace CheatNatives {
    int __cdecl setGodMode(void* L);
    int __cdecl getGodMode(void* L);
    int __cdecl setSpeedMultiplier(void* L);
    int __cdecl getSpeedMultiplier(void* L);
    int __cdecl playerObject(void* L);
    int __cdecl playerFloat(void* L);
}

#endif /* !CHEATS_HPP_ */
