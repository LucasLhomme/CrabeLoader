/*
** CrabeLoader
** File description:
** lua_symbols -- locating the game's Lua C API
*/

#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include "loader/lua_symbols.hpp"
#include "loader/memory.hpp"
#include "logger/logger.hpp"

namespace {

    // The game exports no Lua symbol; each stdlib name is bound to a small
    // wrapper that delegates to the real C API function, so the real address
    // is the n-th `call` inside that wrapper.
    struct LuaSymbol {
        const char* name;        // what we are resolving, for the log
        const char* stdlibName;  // Lua stdlib entry holding the wrapper
        int callIndex;           // 1-based `call` inside the wrapper
        uintptr_t expectedRva;   // RVA read off the shipped binary, 0 to skip the check
        size_t scanBytes = 256;  // how far into the wrapper to look
    };

    // Index is a hint, RVA is the contract: a mismatch refuses rather than
    // hooking a wrong address. "type" resolves to io.type (found before
    // luaB_type); "wrap" is coroutine.wrap.
    constexpr LuaSymbol kLoadfile     { "luaL_loadfile",     "loadfile",   2,  0xF0EBF0 };
    constexpr LuaSymbol kLoadbuffer   { "luaL_loadbuffer",   "loadstring", 3,  0xF0EDE0 };
    constexpr LuaSymbol kPcall        { "lua_pcall",         "xpcall",     4,  0xF0DF60 };

    constexpr LuaSymbol kGettop       { "lua_gettop",        "print",      1,  0xF0D0E0 };
    constexpr LuaSymbol kGetfield     { "lua_getfield",      "print",      2,  0xF0DA00 };
    constexpr LuaSymbol kPushvalue    { "lua_pushvalue",     "print",      3,  0xF0D2A0 };
    constexpr LuaSymbol kCall         { "lua_call",          "print",      5,  0xF0DF00 };
    constexpr LuaSymbol kTolstring    { "lua_tolstring",     "print",      6,  0xF0D5A0, 512 };
    constexpr LuaSymbol kSettop       { "lua_settop",        "print",      11, 0xF0D0F0, 512 };

    constexpr LuaSymbol kPushcclosure { "lua_pushcclosure",  "wrap",       2,  0xF0D8E0 };

    constexpr LuaSymbol kChecktype    { "luaL_checktype",    "rawset",     1,  0xF0EFE0 };
    constexpr LuaSymbol kCheckany     { "luaL_checkany",     "rawset",     2,  0xF0F010 };
    constexpr LuaSymbol kRawset       { "lua_rawset",        "rawset",     5,  0xF0DCA0 };
    constexpr LuaSymbol kRawget       { "lua_rawget",        "rawget",     4,  0xF0DA60 };

    constexpr LuaSymbol kChecklstring { "luaL_checklstring", "require",    1,  0xF0F040 };
    constexpr LuaSymbol kToboolean    { "lua_toboolean",     "require",    5,  0xF0D570 };

    constexpr LuaSymbol kIsnumber     { "lua_isnumber",      "tonumber",   3,  0xF0D350 };
    constexpr LuaSymbol kTonumber     { "lua_tonumber",      "tonumber",   4,  0xF0D4F0 };
    constexpr LuaSymbol kPushnumber   { "lua_pushnumber",    "tonumber",   5,  0xF0D7C0 };

    constexpr LuaSymbol kTouserdata   { "lua_touserdata",    "type",       2,  0xF0D6D0 };
    constexpr LuaSymbol kGetmetatable { "lua_getmetatable",  "type",       4,  0xF0DB20 };
    constexpr LuaSymbol kRawequal     { "lua_rawequal",      "type",       5,  0xF0D3F0 };
    constexpr LuaSymbol kPushlstring  { "lua_pushlstring",   "type",       6,  0xF0D800 };
    constexpr LuaSymbol kPushnil      { "lua_pushnil",       "type",       8,  0xF0D7A0 };

    constexpr LuaSymbol kPushboolean  { "lua_pushboolean",   "rawequal",   4,  0xF0D960 };

    std::string firstBytes(uintptr_t addr, size_t count)
    {
        if (!Memory::isReadable(addr, count)) return "<unreadable>";

        const auto* code = reinterpret_cast<const uint8_t*>(addr);
        std::string out;

        for (size_t i = 0; i < count; ++i) {
            if (i) out += ' ';
            out += std::format("{:02X}", code[i]);
        }
        return out;
    }

    // Tries every stdlib binding of symbol.stdlibName and keeps the one whose
    // call #callIndex lands on expectedRva; a mismatch refuses rather than
    // hooking a wrong address.
    uintptr_t resolveLuaFunction(const LuaSymbol& symbol, uintptr_t base)
    {
        Logger& logger = Logger::getInstance();

        std::vector<uintptr_t> wrappers = Memory::findRegisteredFunctions(symbol.stdlibName);
        if (wrappers.empty()) {
            logger.error("Loader: {}: no '{}' entry in the Lua stdlib table.",
                        symbol.name, symbol.stdlibName);
            return 0;
        }

        auto index = static_cast<size_t>(symbol.callIndex);
        if (symbol.callIndex <= 0) {
            logger.error("Loader: {}: invalid call index {}.", symbol.name, symbol.callIndex);
            return 0;
        }

        uintptr_t fallback = 0;

        for (uintptr_t wrapper : wrappers) {
            std::vector<uintptr_t> calls = Memory::findCalls(wrapper, symbol.scanBytes);
            if (index > calls.size()) continue;

            uintptr_t addr = calls[index - 1];
            uintptr_t rva = addr - base;

            if (!symbol.expectedRva) {
                if (!fallback) fallback = addr;
                continue;
            }
            if (rva != symbol.expectedRva) continue;

            logger.debug("Loader: {} @ 0x{:X} (RVA 0x{:X}) [{}]",
                        symbol.name, addr, rva, firstBytes(addr, 8));
            return addr;
        }

        if (!symbol.expectedRva && fallback) {
            logger.debug("Loader: {} @ 0x{:X} (RVA 0x{:X}) [{}] (unverified: no expected RVA)",
                        symbol.name, fallback, fallback - base, firstBytes(fallback, 8));
            return fallback;
        }

        logger.error("Loader: {}: no '{}' binding ({} candidate(s)) whose call #{} lands on RVA 0x{:X}; refusing.",
                    symbol.name, symbol.stdlibName, wrappers.size(), symbol.callIndex, symbol.expectedRva);

        for (uintptr_t wrapper : wrappers) {
            std::vector<uintptr_t> calls = Memory::findCalls(wrapper, symbol.scanBytes);
            logger.debug("Loader: {}: candidate '{}' @ 0x{:X}, {} call(s)",
                        symbol.name, symbol.stdlibName, wrapper, calls.size());

            for (size_t i = 0; i < calls.size(); ++i) {
                logger.debug("Loader: {}:   call #{} -> RVA 0x{:X}",
                            symbol.name, i + 1, calls[i] - base);
            }
        }
        return 0;
    }

} // namespace

namespace LuaSymbols {

LuaApiAddresses resolveAll(uintptr_t base)
{
    LuaApiAddresses addresses;
    addresses.loadfile = resolveLuaFunction(kLoadfile, base);
    addresses.loadbuffer = resolveLuaFunction(kLoadbuffer, base);
    addresses.pcall = resolveLuaFunction(kPcall, base);
    addresses.call = resolveLuaFunction(kCall, base);

    addresses.gettop = resolveLuaFunction(kGettop, base);
    addresses.settop = resolveLuaFunction(kSettop, base);
    addresses.pushvalue = resolveLuaFunction(kPushvalue, base);

    addresses.tolstring = resolveLuaFunction(kTolstring, base);
    addresses.tonumber = resolveLuaFunction(kTonumber, base);
    addresses.toboolean = resolveLuaFunction(kToboolean, base);
    addresses.touserdata = resolveLuaFunction(kTouserdata, base);
    addresses.isnumber = resolveLuaFunction(kIsnumber, base);

    addresses.pushnil = resolveLuaFunction(kPushnil, base);
    addresses.pushnumber = resolveLuaFunction(kPushnumber, base);
    addresses.pushlstring = resolveLuaFunction(kPushlstring, base);
    addresses.pushboolean = resolveLuaFunction(kPushboolean, base);
    addresses.pushcclosure = resolveLuaFunction(kPushcclosure, base);

    addresses.getfield = resolveLuaFunction(kGetfield, base);
    addresses.rawget = resolveLuaFunction(kRawget, base);
    addresses.rawset = resolveLuaFunction(kRawset, base);
    addresses.rawequal = resolveLuaFunction(kRawequal, base);
    addresses.getmetatable = resolveLuaFunction(kGetmetatable, base);

    addresses.checkany = resolveLuaFunction(kCheckany, base);
    addresses.checktype = resolveLuaFunction(kChecktype, base);
    addresses.checklstring = resolveLuaFunction(kChecklstring, base);

    return addresses;
}

}
