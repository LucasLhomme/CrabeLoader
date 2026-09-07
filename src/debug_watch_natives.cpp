/*
** CrabeLoader
** File description:
** debug_watch_natives -- the Lua face of src/debug_watch.cpp
*/

#include "loader/debug_watch.hpp"

#include <cstdio>
#include <windows.h>

#include "logger/logger.hpp"
#include "loader/luacall.hpp"

namespace {

    // Addresses are reported module relative as well as absolute. ASLR moves
    // the image every run, so the absolute value is worthless for comparing a
    // find against a disassembly, and the RVA is the only stable half.
    uintptr_t moduleBase()
    {
        return reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    }

    // Crabe._watchArm(address [, size] [, readAlso]) -> 1 or nil.
    int __cdecl nativeWatchArm(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto address = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        auto size = static_cast<size_t>(lua.argToNumber(L, 2, 4.0));
        bool readAlso = lua.argToBoolean(L, 3, false);

        const auto access = readAlso ? DebugWatch::Access::ReadWrite : DebugWatch::Access::Write;
        if (!DebugWatch::arm(address, size, access)) return 0;

        lua.pushNumber(L, 1.0);
        return 1;
    }

    int __cdecl nativeWatchDisarm(void* L)
    {
        DebugWatch::disarm();

        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;
        lua.pushNumber(L, 1.0);
        return 1;
    }

    int __cdecl nativeWatchReset(void* L)
    {
        DebugWatch::reset();

        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;
        lua.pushNumber(L, 1.0);
        return 1;
    }

    // Crabe._watchStatus() -> address, hitCount, armed
    int __cdecl nativeWatchStatus(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        lua.pushNumber(L, static_cast<double>(DebugWatch::watching()));
        lua.pushNumber(L, static_cast<double>(DebugWatch::hitCount()));
        lua.pushNumber(L, DebugWatch::armed() ? 1.0 : 0.0);
        return 3;
    }

    // Crabe._watchHit(index) -> instruction, rva, count, eax, ecx, edx, esi, edi
    //
    // A data breakpoint traps once the store has retired, so `instruction` is
    // the address the store ends at, not where it begins. Disassemble a few
    // bytes back from the RVA to find the actual opcode.
    int __cdecl nativeWatchHit(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto index = static_cast<size_t>(lua.argToNumber(L, 1, 1.0));
        if (index == 0) return 0;

        DebugWatch::Hit rows[DebugWatch::kMaxHits];
        const size_t count = DebugWatch::hits(rows, DebugWatch::kMaxHits);
        if (index > count) return 0;

        const DebugWatch::Hit& hit = rows[index - 1];
        const uintptr_t base = moduleBase();

        lua.pushNumber(L, static_cast<double>(hit.instruction));
        lua.pushNumber(L, static_cast<double>(hit.instruction - base));
        lua.pushNumber(L, static_cast<double>(hit.count));
        lua.pushNumber(L, static_cast<double>(hit.registers[0]));
        lua.pushNumber(L, static_cast<double>(hit.registers[1]));
        lua.pushNumber(L, static_cast<double>(hit.registers[2]));
        lua.pushNumber(L, static_cast<double>(hit.registers[6]));
        lua.pushNumber(L, static_cast<double>(hit.registers[7]));
        return 8;
    }

    int __cdecl nativeWatchReport(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        lua.pushString(L, DebugWatch::report());
        return 1;
    }

    struct Entry {
        const char* name;
        int (__cdecl* fn)(void*);
    };

} // namespace

// Assembled here rather than appended to the table in lua_natives.cpp, which
// sits within a dozen lines of the 500-line cap.
std::string DebugWatch::report(size_t maxRows)
{
    Hit rows[kMaxHits];
    const size_t count = hits(rows, maxRows < kMaxHits ? maxRows : kMaxHits);

    if (!armed() && count == 0) return "not watching anything";
    if (count == 0) {
        char idle[96];
        std::snprintf(idle, sizeof(idle), "watching 0x%X, nothing has touched it yet",
                      static_cast<unsigned>(watching()));
        return idle;
    }

    const uintptr_t base = moduleBase();
    std::string text;
    char line[160];

    for (size_t i = 0; i < count; ++i) {
        std::snprintf(line, sizeof(line), "%srva 0x%X x%u eax=0x%X ecx=0x%X esi=0x%X",
                      i ? "\n" : "",
                      static_cast<unsigned>(rows[i].instruction - base),
                      rows[i].count,
                      rows[i].registers[0], rows[i].registers[1], rows[i].registers[6]);
        text += line;
    }
    return text;
}

bool DebugWatchNatives::registerAll(void* L)
{
    static const Entry kEntries[] = {
        { "_watchArm",     &nativeWatchArm },
        { "_watchDisarm",  &nativeWatchDisarm },
        { "_watchReset",   &nativeWatchReset },
        { "_watchStatus",  &nativeWatchStatus },
        { "_watchHit",     &nativeWatchHit },
        { "_watchReport",  &nativeWatchReport },
    };

    bool allOk = true;
    for (const auto& entry : kEntries) {
        if (LuaCall::get().registerNativeFunction(L, "Crabe", entry.name, entry.fn)) continue;

        Logger::getInstance().error("DebugWatch: failed to register Crabe.{}.", entry.name);
        allOk = false;
    }
    return allOk;
}
