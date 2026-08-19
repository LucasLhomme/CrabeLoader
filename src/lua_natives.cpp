/*
** CrabeLoader
** File description:
** lua_natives -- the raw C functions exposed to the game's Lua
*/

#include <cstring>
#include <format>
#include <string>
#include <vector>
#include <windows.h>

#include "loader/avatar_relay_hook.hpp"
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

    // Pushes addresses as the comma-joined string the Lua side parses
    // (src/api/40_inspect.lua). One wire format, defined here only.
    int pushJoined(LuaCall& lua, void* L, const std::vector<uintptr_t>& values)
    {
        std::string joined;

        for (size_t i = 0; i < values.size(); ++i) {
            if (i) joined += ',';
            joined += std::format("{}", values[i]);
        }

        lua.pushString(L, joined);
        return 1;
    }

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

    // Crabe._readBytes(address, count) -> "55 8B EC ...", or nil if the range is
    // not committed. Capped: a stray argument asking for megabytes would build
    // that whole string before Lua ever sees it.
    int __cdecl nativeReadBytes(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto address = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        auto count = static_cast<size_t>(lua.argToNumber(L, 2, 16));

        constexpr size_t kMaxBytes = 512;
        if (!address || count == 0) return 0;
        if (count > kMaxBytes) count = kMaxBytes;
        if (!Memory::isReadable(address, count)) return 0;

        const auto* bytes = reinterpret_cast<const uint8_t*>(address);
        std::string hex;
        hex.reserve(count * 3);

        for (size_t i = 0; i < count; ++i) {
            if (i) hex += ' ';
            hex += std::format("{:02X}", bytes[i]);
        }

        lua.pushString(L, hex);
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

    typedef int(__thiscall* t_ThisCall3)(void* self, int a1, int a2, int a3);
    typedef int(__thiscall* t_ThisCall1)(void* self, int a1);
    typedef int(__thiscall* t_ThisCall0)(void* self);

    // Crabe._callThis(fnAddress, thisPtr, arg1, arg2, arg3) -> eax, or nil on
    // fault. __thiscall, 3 stack args -- must match the callee's own `ret N`
    // exactly (thiscall is callee-cleanup) or the caller's frame corrupts.
    int __cdecl nativeCallThis(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto fnAddress = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        auto thisPtr = static_cast<uintptr_t>(lua.argToNumber(L, 2));
        auto a1 = static_cast<int>(lua.argToNumber(L, 3));
        auto a2 = static_cast<int>(lua.argToNumber(L, 4));
        auto a3 = static_cast<int>(lua.argToNumber(L, 5));

        if (!fnAddress || !thisPtr) return 0;

        auto fn = reinterpret_cast<t_ThisCall3>(fnAddress);
        int result = 0;

        __try {
            result = fn(reinterpret_cast<void*>(thisPtr), a1, a2, a3);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            Logger::getInstance().error("LuaRuntime: _callThis(0x{:X}, this=0x{:X}) faulted.", fnAddress, thisPtr);
            return 0;
        }

        lua.pushNumber(L, static_cast<double>(result));
        return 1;
    }

    // Crabe._callThis1(fnAddress, thisPtr, arg1) -> eax, or nil on fault.
    // __thiscall, 1 stack arg. Only the low byte is meaningful for a
    // bool-returning function that sets `al` alone -- mask with & 0xFF.
    int __cdecl nativeCallThis1(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto fnAddress = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        auto thisPtr = static_cast<uintptr_t>(lua.argToNumber(L, 2));
        auto a1 = static_cast<int>(lua.argToNumber(L, 3));

        if (!fnAddress || !thisPtr) return 0;

        auto fn = reinterpret_cast<t_ThisCall1>(fnAddress);
        int result = 0;

        __try {
            result = fn(reinterpret_cast<void*>(thisPtr), a1);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            Logger::getInstance().error("LuaRuntime: _callThis1(0x{:X}, this=0x{:X}) faulted.", fnAddress, thisPtr);
            return 0;
        }

        lua.pushNumber(L, static_cast<double>(result));
        return 1;
    }

    // Crabe._callThis0(fnAddress, thisPtr) -> eax, or nil on fault. __thiscall,
    // no stack args (bare `ret`). Use whenever the disassembly ends in `ret`
    // alone -- the 3-arg wrapper would leave unpopped dwords on return.
    int __cdecl nativeCallThis0(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto fnAddress = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        auto thisPtr = static_cast<uintptr_t>(lua.argToNumber(L, 2));

        if (!fnAddress || !thisPtr) return 0;

        auto fn = reinterpret_cast<t_ThisCall0>(fnAddress);
        int result = 0;

        __try {
            result = fn(reinterpret_cast<void*>(thisPtr));
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            Logger::getInstance().error("LuaRuntime: _callThis0(0x{:X}, this=0x{:X}) faulted.", fnAddress, thisPtr);
            return 0;
        }

        lua.pushNumber(L, static_cast<double>(result));
        return 1;
    }

    // __try can't coexist with a C++ object needing unwinding in scope
    // (std::vector here), hence this tiny helper with nothing else on its stack.
    bool writeBytesGuarded(uintptr_t address, const uint8_t* bytes, size_t count)
    {
        __try {
            std::memcpy(reinterpret_cast<void*>(address), bytes, count);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
        return true;
    }

    // Crabe._writeBytes(address, "AA BB CC") -> true, or nil if not writable.
    // The one native here that mutates game memory; bounded and SEH-guarded.
    int __cdecl nativeWriteBytes(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto address = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        const char* hex = lua.argToString(L, 2);
        if (!address || !hex) return 0;

        std::vector<uint8_t> bytes;
        for (const char* p = hex; *p; ) {
            while (*p == ' ') ++p;
            if (!*p) break;
            bytes.push_back(static_cast<uint8_t>(std::strtoul(p, nullptr, 16)));
            p += 2;
        }

        constexpr size_t kMaxBytes = 64;
        if (bytes.empty() || bytes.size() > kMaxBytes) return 0;
        if (!Memory::isReadable(address, bytes.size())) return 0;

        if (!writeBytesGuarded(address, bytes.data(), bytes.size())) {
            Logger::getInstance().error("LuaRuntime: _writeBytes(0x{:X}, {} bytes) faulted.", address, bytes.size());
            return 0;
        }

        lua.pushNumber(L, 1.0);
        return 1;
    }

    // Crabe._findPointers(value [, limit]) -> "addr1,addr2,...": memory holding
    // a pointer to `value`, across the whole process. Finds live objects by
    // their vtable, which the module-only scans cannot do.
    int __cdecl nativeFindPointers(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto value = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        auto limit = static_cast<size_t>(lua.argToNumber(L, 2, 32));
        if (!value) return 0;

        constexpr size_t kMaxHits = 256;
        if (limit == 0 || limit > kMaxHits) limit = kMaxHits;

        return pushJoined(lua, L, Memory::findPointers(value, limit));
    }

    // Crabe._findCallSites(target [, limit]) -> "addr1,addr2,..." of every
    // `call rel32` targeting `target` -- the reverse of _findCalls.
    int __cdecl nativeFindCallSites(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto target = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        auto limit = static_cast<size_t>(lua.argToNumber(L, 2, 32));
        if (!target) return 0;

        constexpr size_t kMaxSites = 256;
        if (limit == 0 || limit > kMaxSites) limit = kMaxSites;

        return pushJoined(lua, L, Memory::findCallSites(target, limit));
    }

    // Crabe._findString(text [, after]) -> address above `after`, or nil.
    // Pass the previous result back as `after` to walk every occurrence.
    int __cdecl nativeFindString(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        const char* text = lua.argToString(L, 1);
        if (!text) return 0;

        auto after = static_cast<uintptr_t>(lua.argToNumber(L, 2));
        uintptr_t address = Memory::findString(text, after);
        if (!address) return 0;

        lua.pushNumber(L, static_cast<double>(address));
        return 1;
    }

    // Crabe._scanPattern("68 ?? ?? ?? ??" [, after]) -> address, or nil.
    // IDA-style, "??" being a wildcard.
    int __cdecl nativeScanPattern(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        const char* pattern = lua.argToString(L, 1);
        if (!pattern) return 0;

        auto after = static_cast<uintptr_t>(lua.argToNumber(L, 2));
        uintptr_t address = Memory::patternScan(pattern, nullptr, after);
        if (!address) return 0;

        lua.pushNumber(L, static_cast<double>(address));
        return 1;
    }

    // Crabe._findCalls(address, maxScan) -> "addr1,addr2,..." in call order.
    int __cdecl nativeFindCalls(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto address = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        auto maxScan = static_cast<size_t>(lua.argToNumber(L, 2, 128));
        if (!address) return 0;

        constexpr size_t kMaxScan = 4096;
        if (maxScan == 0 || maxScan > kMaxScan) maxScan = kMaxScan;

        return pushJoined(lua, L, Memory::findCalls(address, maxScan));
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
        { "_readBytes",           &nativeReadBytes },
        { "_findCalls",           &nativeFindCalls },
        { "_findString",          &nativeFindString },
        { "_scanPattern",         &nativeScanPattern },
        { "_findCallSites",       &nativeFindCallSites },
        { "_callThis",            &nativeCallThis },
        { "_callThis1",           &nativeCallThis1 },
        { "_callThis0",           &nativeCallThis0 },
        { "_writeBytes",          &nativeWriteBytes },
        { "_findPointers",        &nativeFindPointers },
        { "_inputReport",         &nativeInputReport },
        { "_messageWatch",        &nativeMessageWatch },
        { "_messageReport",       &nativeMessageReport },
        { "_messageClear",        &nativeMessageClear },
        { "_armAvatarRelay",      &nativeArmAvatarRelay },
        { "_disarmAvatarRelay",   &nativeDisarmAvatarRelay },
        { "_avatarRelayStatus",   &nativeAvatarRelayStatus },
        { "_registerLoadOverride", &nativeRegisterLoadOverride },
        { "_clearLoadOverrides",   &nativeClearLoadOverrides },
    };

    bool allOk = true;
    for (const auto& entry : kNatives) {
        if (LuaCall::get().registerNativeFunction(L, "Crabe", entry.name, entry.fn)) continue;

        Logger::getInstance().error("LuaRuntime: failed to register Crabe.{}.", entry.name);
        allOk = false;
    }

    return allOk;
}
