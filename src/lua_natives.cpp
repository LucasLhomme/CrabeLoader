/*
** CrabeLoader
** File description:
** lua_natives -- the raw C functions exposed to the game's Lua
*/

#include <cstring>
#include <string>
#include <windows.h>

#include "loader/avatar_relay_hook.hpp"
#include "loader/cheats.hpp"
#include "loader/debug_watch.hpp"
#include "loader/freecam.hpp"
#include "loader/speedhack.hpp"
#include "loader/input_hook.hpp"
#include "loader/loader.hpp"
#include "loader/lua_runtime.hpp"
#include "loader/luacall.hpp"
#include "loader/memory.hpp"
#include "loader/message_hook.hpp"
#include "loader/render_hook.hpp"
#include "logger/logger.hpp"

// Every function here is a Lua C function: it reads its arguments off the Lua
// stack, pushes its results back, and returns how many it pushed. They are the
// loader's whole surface to Lua -- memory access, hook control, window mode --
// and src/api/*.lua wraps them into something ergonomic.

namespace {

    // No native exposes window/fullscreen state to Lua; window calls must
    // happen on the render thread, not here, hence just posting a request.
    int __cdecl nativeSetWindowMode(void* L)
    {
        const char* mode = LuaCall::get().argToString(L, 1);
        bool borderless = mode && std::strcmp(mode, "borderless") == 0;

        RenderHook::get().requestWindowMode(borderless ? WindowMode::BorderlessWindowed : WindowMode::Windowed);
        return 0;
    }

    // Crabe._findGameNative(name) -> address, or nil. Looks in the image's
    // registration table, unlike type(_G[name]).
    int __cdecl nativeFindGameNative(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        const char* name = lua.argToString(L, 1);
        if (!name) return 0;

        uintptr_t address = Memory::findRegisteredFunction(name);
        if (!address) return 0;

        lua.pushNumber(L, static_cast<double>(address));
        return 1;
    }

    // Crabe._moduleBase() -> base address of the game image, so Lua can turn the
    // absolute addresses above into RVAs comparable with a static disassembly.
    int __cdecl nativeModuleBase(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        lua.pushNumber(L, static_cast<double>(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))));
        return 1;
    }

    // Crabe._inputReport() -> which XInput slots the game has polled so far.
    int __cdecl nativeInputReport(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        lua.pushString(L, InputHook::get().report());
        return 1;
    }

    // Crabe._messageWatch(substring) -> start recording engine messages whose
    // name contains `substring`. The bus carries every message in the game, so
    // nothing is recorded until something is watched.
    int __cdecl nativeMessageWatch(void* L)
    {
        const char* substring = LuaCall::get().argToString(L, 1);
        if (substring) MessageHook::get().watch(substring);
        return 0;
    }

    // Crabe._messageReport() -> "name(a,b) | name(a,b) | ...", oldest first.
    int __cdecl nativeMessageReport(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        lua.pushString(L, MessageHook::get().report());
        return 1;
    }

    int __cdecl nativeMessageClear(void* L)
    {
        (void)L;
        MessageHook::get().clear();
        return 0;
    }

    // Crabe._armAvatarRelay(gamePlayersThis, targetPlayerIndex) -> 1, or 0 if
    // the hook is not installed or gamePlayersThis is 0. See avatar_relay_hook.hpp.
    int __cdecl nativeArmAvatarRelay(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto gamePlayersThis = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        auto targetPlayerIndex = static_cast<int>(lua.argToNumber(L, 2));

        bool ok = AvatarRelayHook::get().arm(gamePlayersThis, targetPlayerIndex);
        lua.pushNumber(L, ok ? 1.0 : 0.0);
        return 1;
    }

    // Crabe._disarmAvatarRelay() -> cancels a pending arm.
    int __cdecl nativeDisarmAvatarRelay(void* L)
    {
        (void)L;
        AvatarRelayHook::get().disarm();
        return 0;
    }

    // Crabe._avatarRelayStatus() -> "armed=0 fired=1 lastHandle=0x... lastRelayResult=...".
    int __cdecl nativeAvatarRelayStatus(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        lua.pushString(L, AvatarRelayHook::get().report());
        return 1;
    }

    // Crabe._registerLoadOverride(matchSubstring, luaSource). See
    // Loader::registerLoadOverride/findLoadOverride.
    int __cdecl nativeRegisterLoadOverride(void* L)
    {
        LuaCall& lua = LuaCall::get();
        const char* matchSubstring = lua.argToString(L, 1);
        const char* luaSource = lua.argToString(L, 2);
        if (!matchSubstring || !luaSource) return 0;

        Loader::get().registerLoadOverride(matchSubstring, luaSource);
        return 0;
    }

    // Crabe._clearLoadOverrides() -- drops every registered override.
    int __cdecl nativeClearLoadOverrides(void* L)
    {
        (void)L;
        Loader::get().clearLoadOverrides();
        return 0;
    }

} // namespace

// All registered under underscore-prefixed names: these are the raw natives,
// wrapped by the ergonomic API in src/api/*.lua, same as every other
// internal detail in Crabe.
bool LuaRuntime::registerNatives(void* L)
{
    struct Entry {
        const char* name;
        LuaCall::t_lua_cfunction fn;
    };

    static constexpr Entry kNatives[] = {
        { "_setWindowModeNative", &nativeSetWindowMode },
        { "_findGameNative",      &nativeFindGameNative },
        { "_moduleBase",          &nativeModuleBase },
        { "_inputReport",         &nativeInputReport },
        { "_keyDown",             &InputNatives::keyDown },
        { "_messageWatch",        &nativeMessageWatch },
        { "_messageReport",       &nativeMessageReport },
        { "_messageClear",        &nativeMessageClear },
        { "_armAvatarRelay",      &nativeArmAvatarRelay },
        { "_disarmAvatarRelay",   &nativeDisarmAvatarRelay },
        { "_avatarRelayStatus",   &nativeAvatarRelayStatus },
        { "_registerLoadOverride", &nativeRegisterLoadOverride },
        { "_clearLoadOverrides",   &nativeClearLoadOverrides },
    };

    bool allOk = DebugWatchNatives::registerAll(L);
    allOk = SpeedHackNatives::registerAll(L) && allOk;
    allOk = CheatNatives::registerAll(L) && allOk;
    allOk = FreecamNatives::registerAll(L) && allOk;
    for (const auto& entry : kNatives) {
        if (LuaCall::get().registerNativeFunction(L, "Crabe", entry.name, entry.fn)) continue;

        Logger::getInstance().error("LuaRuntime: failed to register Crabe.{}.", entry.name);
        allOk = false;
    }

    return allOk;
}
