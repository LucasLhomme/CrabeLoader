/*
** CrabeLoader
** File description:
** freecam_natives -- Lua native bindings for the 6-DOF Freecam engine
*/

#include "loader/freecam.hpp"
#include "loader/luacall.hpp"
#include "logger/logger.hpp"

int __cdecl FreecamNatives::toggle(void* L)
{
    Freecam::get().toggle();
    LuaCall::get().pushNumber(L, Freecam::get().isEnabled() ? 1.0 : 0.0);
    return 1;
}

int __cdecl FreecamNatives::setEnabled(void* L)
{
    LuaCall& lua = LuaCall::get();
    const bool enabled = lua.argToBoolean(L, 1, false) || (lua.argToNumber(L, 1, 0.0) != 0.0);
    Freecam::get().setEnabled(enabled);
    lua.pushNumber(L, Freecam::get().isEnabled() ? 1.0 : 0.0);
    return 1;
}

int __cdecl FreecamNatives::isEnabled(void* L)
{
    LuaCall::get().pushNumber(L, Freecam::get().isEnabled() ? 1.0 : 0.0);
    return 1;
}

int __cdecl FreecamNatives::setSpeed(void* L)
{
    LuaCall& lua = LuaCall::get();
    const auto speed = static_cast<float>(lua.argToNumber(L, 1, 25.0));
    Freecam::get().setSpeed(speed);
    lua.pushNumber(L, Freecam::get().getSpeed());
    return 1;
}

int __cdecl FreecamNatives::getSpeed(void* L)
{
    LuaCall::get().pushNumber(L, Freecam::get().getSpeed());
    return 1;
}

int __cdecl FreecamNatives::setSensitivity(void* L)
{
    LuaCall& lua = LuaCall::get();
    const auto sens = static_cast<float>(lua.argToNumber(L, 1, 0.003));
    Freecam::get().setSensitivity(sens);
    lua.pushNumber(L, Freecam::get().getSensitivity());
    return 1;
}

int __cdecl FreecamNatives::getSensitivity(void* L)
{
    LuaCall::get().pushNumber(L, Freecam::get().getSensitivity());
    return 1;
}

int __cdecl FreecamNatives::update(void* L)
{
    LuaCall& lua = LuaCall::get();
    const auto dt = static_cast<float>(lua.argToNumber(L, 1, 0.033));
    Freecam::get().update(dt);
    lua.pushNumber(L, 1.0);
    return 1;
}

int __cdecl FreecamNatives::getPosition(void* L)
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    Freecam::get().getPosition(x, y, z);

    LuaCall& lua = LuaCall::get();
    lua.pushNumber(L, x);
    lua.pushNumber(L, y);
    lua.pushNumber(L, z);
    return 3;
}

int __cdecl FreecamNatives::setPosition(void* L)
{
    LuaCall& lua = LuaCall::get();
    const auto x = static_cast<float>(lua.argToNumber(L, 1, 0.0));
    const auto y = static_cast<float>(lua.argToNumber(L, 2, 0.0));
    const auto z = static_cast<float>(lua.argToNumber(L, 3, 0.0));

    Freecam::get().setPosition(x, y, z);
    lua.pushNumber(L, 1.0);
    return 1;
}

int __cdecl FreecamNatives::getRotation(void* L)
{
    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;
    Freecam::get().getRotation(pitch, yaw, roll);

    LuaCall& lua = LuaCall::get();
    lua.pushNumber(L, pitch);
    lua.pushNumber(L, yaw);
    lua.pushNumber(L, roll);
    return 3;
}

int __cdecl FreecamNatives::setRotation(void* L)
{
    LuaCall& lua = LuaCall::get();
    const auto pitch = static_cast<float>(lua.argToNumber(L, 1, 0.0));
    const auto yaw = static_cast<float>(lua.argToNumber(L, 2, 0.0));
    const auto roll = static_cast<float>(lua.argToNumber(L, 3, 0.0));

    Freecam::get().setRotation(pitch, yaw, roll);
    lua.pushNumber(L, 1.0);
    return 1;
}

int __cdecl FreecamNatives::teleportPlayer(void* L)
{
    const bool ok = Freecam::get().teleportPlayerToCamera();
    LuaCall::get().pushNumber(L, ok ? 1.0 : 0.0);
    return 1;
}

int __cdecl FreecamNatives::setWorldFrozen(void* L)
{
    LuaCall& lua = LuaCall::get();
    const bool frozen = lua.argToBoolean(L, 1, false) || (lua.argToNumber(L, 1, 0.0) != 0.0);

    Freecam::get().setWorldFrozen(frozen);
    lua.pushNumber(L, Freecam::get().isWorldFrozen() ? 1.0 : 0.0);
    return 1;
}

int __cdecl FreecamNatives::isWorldFrozen(void* L)
{
    LuaCall& lua = LuaCall::get();
    lua.pushNumber(L, Freecam::get().isWorldFrozen() ? 1.0 : 0.0);
    return 1;
}

bool FreecamNatives::registerAll(void* L)
{
    static constexpr struct {
        const char* name;
        int(__cdecl* fn)(void*);
    } kNatives[] = {
        { "_freecamToggle",          toggle },
        { "_freecamSetEnabled",      setEnabled },
        { "_freecamIsEnabled",       isEnabled },
        { "_freecamSetSpeed",        setSpeed },
        { "_freecamGetSpeed",        getSpeed },
        { "_freecamUpdate",          update },
        { "_freecamGetPosition",     getPosition },
        { "_freecamSetPosition",     setPosition },
        { "_freecamGetRotation",     getRotation },
        { "_freecamSetRotation",     setRotation },
        { "_freecamTeleportPlayer",  teleportPlayer },
        { "_freecamSetWorldFrozen",  setWorldFrozen },
        { "_freecamIsWorldFrozen",   isWorldFrozen },
        { "_freecamSetSensitivity",  setSensitivity },
        { "_freecamGetSensitivity",  getSensitivity },
    };

    bool allOk = true;
    for (const auto& entry : kNatives) {
        if (!LuaCall::get().registerNativeFunction(L, "Crabe", entry.name, entry.fn)) {
            Logger::getInstance().error("FreecamNatives: failed to register Crabe.{}", entry.name);
            allOk = false;
        }
    }
    return allOk;
}
