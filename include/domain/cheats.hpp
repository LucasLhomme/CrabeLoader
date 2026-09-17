/*
** CrabeLoader
** File description:
** cheats
*/

#ifndef CHEATS_HPP_
#define CHEATS_HPP_

#include <cstdint>
#include <string>

// Invulnerability and movement speed, both driven by manual code caves on
// mid-function SSE sites (see codecave.hpp for why not MinHook).
//
// God mode is two caves. One sits on the HUD health read and reports every
// component it sees to EntityRegistry; one sits on the damage-apply site,
// reports the delta, and then refuses any write that would lower the health of
// the component the registry currently points at. Enemies keep taking damage
// because only that one pointer is clamped.
//
// Which component is the player is decided by the registry from observation,
// never from the health value: the avatar and NPC meters share one engine
// function, so no threshold can separate them.
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

    // Health component the clamp is currently protecting, or 0 before the HUD
    // has drawn a health bar once. Needs installGodMode to have run.
    uintptr_t playerObject();

    // How many incoming hits the clamp has actually cancelled. Counted by the
    // cave itself, so a zero here while taking damage means the clamp is not
    // being reached -- not that the cheat is merely mis-targeted.
    uint32_t blockedHits();

    // Float at [playerObject() + offset]. False when no object is captured or
    // the read would fault. Used to probe the entity struct from the console.
    bool readPlayerFloat(uintptr_t offset, float& out);

    // Float at [playerObject() + offset], written. False when no object is
    // captured or the address is not writable.
    bool writePlayerFloat(uintptr_t offset, float value);

    // Scans and installs the four velocity caves. Idempotent.
    bool installSpeed();

    // Installs the caves if needed, then sets the velocity multiplier
    // (1.0 = stock). Clamped to a sane range; false if the caves failed.
    bool setSpeedMultiplier(float multiplier);
    float speedMultiplier();

    // How many times each velocity site has executed, slash-separated. These
    // four AOBs were inherited unvalidated, so a site that never fires is the
    // likeliest explanation for a multiplier that changes nothing.
    std::string speedHitReport();

    // Installs the velocity caves with the multiplier left at 1.0, purely so
    // their 'mov [g_moveObject], eax' captures the pointer. Position tracking
    // used to require enabling the speed cheat, which is backwards: the speed
    // multiplier does not work, and teleport does not need it to.
    bool trackPosition();

    // The player's movement structure, captured by the velocity caves, or 0
    // before the player has moved once. This is the pointer teleport and
    // noclip need; position is an unknown offset inside it.
    uintptr_t moveObject();

    // Snapshot the structure, move the player, then diff: offsets that changed
    // are candidates for the position fields.
    bool snapshotMove();
    std::string diffMove(float minimumChange);

    // The position candidate found by the walk test. False when no movement
    // structure has been captured or the floats are not finite.
    bool position(float& x, float& y, float& z);
    bool setPosition(float x, float y, float z);
    bool teleportDelta(float dx, float dy, float dz);
    bool unlockEditorEverywhere();

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
    int __cdecl setPlayerFloat(void* L);

    // The observed-entity table behind god mode. entityAt takes a 1-based
    // index and returns component, maxHealth, health, seen, damaged.
    int __cdecl entityCount(void* L);
    int __cdecl entityAt(void* L);
    int __cdecl selectTarget(void* L);
    int __cdecl targetInfo(void* L);

    // The movement-structure probe: capture a pointer, snapshot it, diff it.
    int __cdecl trackPosition(void* L);
    int __cdecl moveObject(void* L);
    int __cdecl moveSnapshot(void* L);
    int __cdecl moveDiff(void* L);
    int __cdecl position(void* L);
    int __cdecl setPosition(void* L);
    int __cdecl teleportDelta(void* L);
    int __cdecl unlockEditor(void* L);

    bool registerAll(void* L);
}

#endif /* !CHEATS_HPP_ */
