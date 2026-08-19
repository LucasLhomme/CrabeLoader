/*
** CrabeLoader
** File description:
** lua_symbols -- locating the game's Lua C API
*/

#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
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

    // Which LuaApiAddresses field each symbol fills, stated once so the pairing
    // cannot drift. Index is a hint, RVA is the contract: a mismatch refuses
    // rather than hooking a wrong address. "type" resolves to io.type (found
    // before luaB_type); "wrap" is coroutine.wrap.
    struct Binding {
        uintptr_t LuaApiAddresses::* field;
        LuaSymbol symbol;
    };

    constexpr Binding kBindings[] = {
        { &LuaApiAddresses::loadfile,     { "luaL_loadfile",     "loadfile",   2,  0xF0EBF0 } },
        { &LuaApiAddresses::loadbuffer,   { "luaL_loadbuffer",   "loadstring", 3,  0xF0EDE0 } },
        { &LuaApiAddresses::pcall,        { "lua_pcall",         "xpcall",     4,  0xF0DF60 } },

        { &LuaApiAddresses::gettop,       { "lua_gettop",        "print",      1,  0xF0D0E0 } },
        { &LuaApiAddresses::getfield,     { "lua_getfield",      "print",      2,  0xF0DA00 } },
        { &LuaApiAddresses::pushvalue,    { "lua_pushvalue",     "print",      3,  0xF0D2A0 } },
        { &LuaApiAddresses::call,         { "lua_call",          "print",      5,  0xF0DF00 } },
        { &LuaApiAddresses::tolstring,    { "lua_tolstring",     "print",      6,  0xF0D5A0, 512 } },
        { &LuaApiAddresses::settop,       { "lua_settop",        "print",      11, 0xF0D0F0, 512 } },

        { &LuaApiAddresses::pushcclosure, { "lua_pushcclosure",  "wrap",       2,  0xF0D8E0 } },

        { &LuaApiAddresses::checktype,    { "luaL_checktype",    "rawset",     1,  0xF0EFE0 } },
        { &LuaApiAddresses::checkany,     { "luaL_checkany",     "rawset",     2,  0xF0F010 } },
        { &LuaApiAddresses::rawset,       { "lua_rawset",        "rawset",     5,  0xF0DCA0 } },
        { &LuaApiAddresses::rawget,       { "lua_rawget",        "rawget",     4,  0xF0DA60 } },

        { &LuaApiAddresses::checklstring, { "luaL_checklstring", "require",    1,  0xF0F040 } },
        { &LuaApiAddresses::toboolean,    { "lua_toboolean",     "require",    5,  0xF0D570 } },

        { &LuaApiAddresses::isnumber,     { "lua_isnumber",      "tonumber",   3,  0xF0D350 } },
        { &LuaApiAddresses::tonumber,     { "lua_tonumber",      "tonumber",   4,  0xF0D4F0 } },
        { &LuaApiAddresses::pushnumber,   { "lua_pushnumber",    "tonumber",   5,  0xF0D7C0 } },

        { &LuaApiAddresses::touserdata,   { "lua_touserdata",    "type",       2,  0xF0D6D0 } },
        { &LuaApiAddresses::getmetatable, { "lua_getmetatable",  "type",       4,  0xF0DB20 } },
        { &LuaApiAddresses::rawequal,     { "lua_rawequal",      "type",       5,  0xF0D3F0 } },
        { &LuaApiAddresses::pushlstring,  { "lua_pushlstring",   "type",       6,  0xF0D800 } },
        { &LuaApiAddresses::pushnil,      { "lua_pushnil",       "type",       8,  0xF0D7A0 } },

        { &LuaApiAddresses::pushboolean,  { "lua_pushboolean",   "rawequal",   4,  0xF0D960 } },
    };

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
    uintptr_t resolveLuaFunction(const LuaSymbol& symbol, uintptr_t base,
                                const std::vector<uintptr_t>& wrappers)
    {
        Logger& logger = Logger::getInstance();

        if (wrappers.empty()) {
            logger.error("LuaSymbols: {}: no '{}' entry in the Lua stdlib table.",
                        symbol.name, symbol.stdlibName);
            return 0;
        }

        auto index = static_cast<size_t>(symbol.callIndex);
        if (symbol.callIndex <= 0) {
            logger.error("LuaSymbols: {}: invalid call index {}.", symbol.name, symbol.callIndex);
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

            logger.debug("LuaSymbols: {} @ 0x{:X} (RVA 0x{:X}) [{}]",
                        symbol.name, addr, rva, firstBytes(addr, 8));
            return addr;
        }

        if (!symbol.expectedRva && fallback) {
            logger.debug("LuaSymbols: {} @ 0x{:X} (RVA 0x{:X}) [{}] (unverified: no expected RVA)",
                        symbol.name, fallback, fallback - base, firstBytes(fallback, 8));
            return fallback;
        }

        logger.error("LuaSymbols: {}: no '{}' binding ({} candidate(s)) whose call #{} lands on RVA 0x{:X}; refusing.",
                    symbol.name, symbol.stdlibName, wrappers.size(), symbol.callIndex, symbol.expectedRva);

        for (uintptr_t wrapper : wrappers) {
            std::vector<uintptr_t> calls = Memory::findCalls(wrapper, symbol.scanBytes);
            logger.debug("LuaSymbols: {}: candidate '{}' @ 0x{:X}, {} call(s)",
                        symbol.name, symbol.stdlibName, wrapper, calls.size());

            for (size_t i = 0; i < calls.size(); ++i) {
                logger.debug("LuaSymbols: {}:   call #{} -> RVA 0x{:X}",
                            symbol.name, i + 1, calls[i] - base);
            }
        }
        return 0;
    }

} // namespace

namespace LuaSymbols {

LuaApiAddresses resolveAll(uintptr_t base)
{
    // Twenty-five symbols share eleven stdlib names ("print" alone carries
    // six). Each lookup walks the whole image, so they are cached here and
    // every symbol on the same name reuses one walk.
    std::unordered_map<std::string_view, std::vector<uintptr_t>> wrappers;
    LuaApiAddresses addresses;

    for (const Binding& binding : kBindings) {
        auto [it, inserted] = wrappers.try_emplace(binding.symbol.stdlibName);
        if (inserted)
            it->second = Memory::findRegisteredFunctions(binding.symbol.stdlibName);

        addresses.*binding.field = resolveLuaFunction(binding.symbol, base, it->second);
    }

    return addresses;
}

}
