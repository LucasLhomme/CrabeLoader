/*
** CrabeLoader
** File description:
** cheat_natives -- the Lua face of src/cheats.cpp
*/

#include "loader/cheats.hpp"

#include <cstdint>

#include "loader/entity_registry.hpp"
#include "loader/luacall.hpp"
#include "logger/logger.hpp"


// Crabe._setGodMode(enabled) -> 1, or nil when the caves could not be
// installed. Lua turns that nil into a named error.
int __cdecl CheatNatives::setGodMode(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    // argToNumber cannot read this: lua_tonumber turns a Lua boolean into 0,
    // so every setGodMode(true) arrived as false and the clamp was never armed.
    // That single conversion is why this cheat never worked on this loader.
    bool enabled = lua.argToBoolean(L, 1, false);
    if (!Cheats::setGodMode(enabled)) return 0;

    lua.pushNumber(L, 1.0);
    return 1;
}

// Crabe._getGodMode() -> 1 or 0.
int __cdecl CheatNatives::getGodMode(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    lua.pushNumber(L, Cheats::godMode() ? 1.0 : 0.0);
    return 1;
}

// Crabe._setSpeedMultiplier(multiplier) -> the multiplier actually applied
// (it is clamped), or nil when the caves could not be installed.
int __cdecl CheatNatives::setSpeedMultiplier(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    auto multiplier = static_cast<float>(lua.argToNumber(L, 1, 1.0));
    if (!Cheats::setSpeedMultiplier(multiplier)) return 0;

    lua.pushNumber(L, static_cast<double>(Cheats::speedMultiplier()));
    return 1;
}

// Crabe._getSpeedMultiplier() -> multiplier, "hits per site".
int __cdecl CheatNatives::getSpeedMultiplier(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    lua.pushNumber(L, static_cast<double>(Cheats::speedMultiplier()));
    lua.pushString(L, Cheats::speedHitReport());
    return 2;
}

// Crabe._entityCount() -> how many health components have been catalogued.
int __cdecl CheatNatives::entityCount(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    lua.pushNumber(L, static_cast<double>(EntityRegistry::count()));
    return 1;
}

// Crabe._entityAt(index) -> component, maxHealth, health, seen, damaged.
// One-based, ordered most-seen first, so index 1 is the automatic pick.
int __cdecl CheatNatives::entityAt(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    auto index = static_cast<size_t>(lua.argToNumber(L, 1, 0.0));
    if (index == 0) return 0;

    EntityRegistry::Entry rows[EntityRegistry::kMaxEntries];
    size_t written = EntityRegistry::snapshot(rows, EntityRegistry::kMaxEntries);
    if (index > written) return 0;

    const EntityRegistry::Entry& row = rows[index - 1];
    lua.pushNumber(L, static_cast<double>(row.component));
    lua.pushNumber(L, static_cast<double>(row.maxHealth));
    lua.pushNumber(L, static_cast<double>(row.health));
    lua.pushNumber(L, static_cast<double>(row.seen));
    lua.pushNumber(L, static_cast<double>(row.damaged));
    return 5;
}

// Crabe._selectTarget(address) -> the address now protected. Zero restores
// the automatic pick.
int __cdecl CheatNatives::selectTarget(void* L)
{
    LuaCall& lua = LuaCall::get();

    auto component = static_cast<uintptr_t>(lua.argToNumber(L, 1, 0.0));
    EntityRegistry::selectManual(component);

    if (!lua.hasReturnSupport()) return 0;
    lua.pushNumber(L, static_cast<double>(EntityRegistry::target()));
    return 1;
}

// Crabe._targetInfo() -> target, manual, count, lastDamaged, lastDelta.
// lastDamaged is the ground truth for identifying the player: take a hit and
// read it back.
int __cdecl CheatNatives::targetInfo(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    lua.pushNumber(L, static_cast<double>(EntityRegistry::target()));
    lua.pushNumber(L, static_cast<double>(EntityRegistry::manualSelection()));
    lua.pushNumber(L, static_cast<double>(EntityRegistry::count()));
    lua.pushNumber(L, static_cast<double>(EntityRegistry::lastDamagedComponent()));
    lua.pushNumber(L, static_cast<double>(EntityRegistry::lastDamagedDelta()));
    lua.pushNumber(L, static_cast<double>(Cheats::blockedHits()));
    return 6;
}

int __cdecl CheatNatives::playerObject(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    uintptr_t object = Cheats::playerObject();
    if (!object) return 0;

    lua.pushNumber(L, static_cast<double>(object));
    return 1;
}

// Crabe._playerFloat(offset) -> float at [player+offset], or nil. The whole
// entity struct is walkable this way, which is how offsets past the known
// health/velocity ones get identified without a rebuild.
int __cdecl CheatNatives::playerFloat(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    // Defaults to the health field, the one offset a caller is most likely to
    // want and the only one this file needs to know about.
    constexpr uintptr_t kHealthOffset = 0x08;

    auto offset = static_cast<uintptr_t>(lua.argToNumber(L, 1, static_cast<double>(kHealthOffset)));
    float value = 0.0f;

    if (!Cheats::readPlayerFloat(offset, value)) return 0;

    lua.pushNumber(L, static_cast<double>(value));
    return 1;
}

// Crabe._moveObject() -> address of the player's movement structure, or nil.
int __cdecl CheatNatives::moveObject(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    uintptr_t object = Cheats::moveObject();
    if (!object) return 0;

    lua.pushNumber(L, static_cast<double>(object));
    return 1;
}

// Crabe._moveSnapshot() -> 1, or nil when no movement object has been seen.
int __cdecl CheatNatives::moveSnapshot(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;
    if (!Cheats::snapshotMove()) return 0;

    lua.pushNumber(L, 1.0);
    return 1;
}

// Crabe._moveDiff(floor) -> "0x298:+1.2 0x2A0:-0.4 ..." for the offsets that
// changed since the snapshot.
int __cdecl CheatNatives::moveDiff(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    auto floor = static_cast<float>(lua.argToNumber(L, 1, 0.5));
    lua.pushString(L, Cheats::diffMove(floor));
    return 1;
}

// Crabe._position() -> x, y, z from the candidate offsets, or nil.
int __cdecl CheatNatives::position(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    if (!Cheats::position(x, y, z)) return 0;

    lua.pushNumber(L, static_cast<double>(x));
    lua.pushNumber(L, static_cast<double>(y));
    lua.pushNumber(L, static_cast<double>(z));
    return 3;
}

// Crabe._trackPosition() -> 1, or nil when the velocity AOBs did not resolve.
// Arms the pointer capture without touching the movement multiplier.
int __cdecl CheatNatives::trackPosition(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    if (!Cheats::trackPosition()) return 0;

    lua.pushNumber(L, 1.0);
    return 1;
}

// Crabe._setPlayerFloat(offset, value) -> 1, or nil when nothing is captured.
int __cdecl CheatNatives::setPlayerFloat(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    const auto offset = static_cast<uintptr_t>(lua.argToNumber(L, 1, 0.0));
    const auto value = static_cast<float>(lua.argToNumber(L, 2, 0.0));

    if (!Cheats::writePlayerFloat(offset, value)) return 0;

    lua.pushNumber(L, 1.0);
    return 1;
}

// Crabe._setPosition(x, y, z) -> 1, or nil
int __cdecl CheatNatives::setPosition(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    auto x = static_cast<float>(lua.argToNumber(L, 1, 0.0));
    auto y = static_cast<float>(lua.argToNumber(L, 2, 0.0));
    auto z = static_cast<float>(lua.argToNumber(L, 3, 0.0));

    if (!Cheats::setPosition(x, y, z)) return 0;

    lua.pushNumber(L, 1.0);
    return 1;
}

// Crabe._teleportDelta(dx, dy, dz) -> 1, or nil
int __cdecl CheatNatives::teleportDelta(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    auto dx = static_cast<float>(lua.argToNumber(L, 1, 0.0));
    auto dy = static_cast<float>(lua.argToNumber(L, 2, 0.0));
    auto dz = static_cast<float>(lua.argToNumber(L, 3, 0.0));

    if (!Cheats::teleportDelta(dx, dy, dz)) return 0;

    lua.pushNumber(L, 1.0);
    return 1;
}

// Crabe._unlockEditor() -> 1, or nil
int __cdecl CheatNatives::unlockEditor(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    if (!Cheats::unlockEditorEverywhere()) return 0;

    lua.pushNumber(L, 1.0);
    return 1;
}

bool CheatNatives::registerAll(void* L)
{
    static constexpr struct {
        const char* name;
        int(__cdecl* fn)(void*);
    } kNatives[] = {
        { "_setGodMode",          &CheatNatives::setGodMode },
        { "_getGodMode",          &CheatNatives::getGodMode },
        { "_setSpeedMultiplier",  &CheatNatives::setSpeedMultiplier },
        { "_getSpeedMultiplier",  &CheatNatives::getSpeedMultiplier },
        { "_playerObject",        &CheatNatives::playerObject },
        { "_playerFloat",         &CheatNatives::playerFloat },
        { "_setPlayerFloat",      &CheatNatives::setPlayerFloat },
        { "_entityCount",         &CheatNatives::entityCount },
        { "_entityAt",            &CheatNatives::entityAt },
        { "_selectTarget",        &CheatNatives::selectTarget },
        { "_targetInfo",          &CheatNatives::targetInfo },
        { "_trackPosition",       &CheatNatives::trackPosition },
        { "_moveObject",          &CheatNatives::moveObject },
        { "_moveSnapshot",        &CheatNatives::moveSnapshot },
        { "_moveDiff",            &CheatNatives::moveDiff },
        { "_position",            &CheatNatives::position },
        { "_setPosition",         &CheatNatives::setPosition },
        { "_teleportDelta",       &CheatNatives::teleportDelta },
        { "_unlockEditor",        &CheatNatives::unlockEditor },
    };

    bool allOk = true;
    for (const auto& entry : kNatives) {
        if (!LuaCall::get().registerNativeFunction(L, "Crabe", entry.name, entry.fn)) {
            Logger::getInstance().error("CheatNatives: failed to register Crabe.{}", entry.name);
            allOk = false;
        }
    }
    return allOk;
}

