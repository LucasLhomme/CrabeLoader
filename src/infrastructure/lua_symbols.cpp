/*
** CrabeLoader
** File description:
** Locates the game Lua C API by scanning the image for the wrappers that delegate to it.
** The game exports no Lua symbol, so each real address is the n-th call inside a stdlib wrapper.
** Calls none of them; the calls and the detours are src/infrastructure/lua_call.cpp.
**
** Authors: @LucasLhomme
*/

#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "domain/game_profile.hpp"
#include "infrastructure/lua_symbols.hpp"
#include "infrastructure/memory.hpp"
#include "shared/logger.hpp"

namespace {

    struct LuaSymbol {
        const char* name;
        const char* stdlibName;
        int callIndex;
        size_t scanBytes = 256;
    };

    struct Binding {
        uintptr_t crabe::infrastructure::LuaApiAddresses::* field;
        LuaSymbol symbol;
    };

    constexpr Binding kBindings[] = {
        { &crabe::infrastructure::LuaApiAddresses::loadfile,     { "luaL_loadfile",     "loadfile",   2 } },
        { &crabe::infrastructure::LuaApiAddresses::loadbuffer,   { "luaL_loadbuffer",   "loadstring", 3 } },
        { &crabe::infrastructure::LuaApiAddresses::pcall,        { "lua_pcall",         "xpcall",     4 } },

        { &crabe::infrastructure::LuaApiAddresses::gettop,       { "lua_gettop",        "print",      1 } },
        { &crabe::infrastructure::LuaApiAddresses::getfield,     { "lua_getfield",      "print",      2 } },
        { &crabe::infrastructure::LuaApiAddresses::pushvalue,    { "lua_pushvalue",     "print",      3 } },
        { &crabe::infrastructure::LuaApiAddresses::call,         { "lua_call",          "print",      5 } },
        { &crabe::infrastructure::LuaApiAddresses::tolstring,    { "lua_tolstring",     "print",      6,  512 } },
        { &crabe::infrastructure::LuaApiAddresses::settop,       { "lua_settop",        "print",      11, 512 } },

        { &crabe::infrastructure::LuaApiAddresses::pushcclosure, { "lua_pushcclosure",  "wrap",       2 } },

        { &crabe::infrastructure::LuaApiAddresses::checktype,    { "luaL_checktype",    "rawset",     1 } },
        { &crabe::infrastructure::LuaApiAddresses::checkany,     { "luaL_checkany",     "rawset",     2 } },
        { &crabe::infrastructure::LuaApiAddresses::rawset,       { "lua_rawset",        "rawset",     5 } },
        { &crabe::infrastructure::LuaApiAddresses::rawget,       { "lua_rawget",        "rawget",     4 } },

        { &crabe::infrastructure::LuaApiAddresses::checklstring, { "luaL_checklstring", "require",    1 } },
        { &crabe::infrastructure::LuaApiAddresses::toboolean,    { "lua_toboolean",     "require",    5 } },

        { &crabe::infrastructure::LuaApiAddresses::isnumber,     { "lua_isnumber",      "tonumber",   3 } },
        { &crabe::infrastructure::LuaApiAddresses::tonumber,     { "lua_tonumber",      "tonumber",   4 } },
        { &crabe::infrastructure::LuaApiAddresses::pushnumber,   { "lua_pushnumber",    "tonumber",   5 } },

        { &crabe::infrastructure::LuaApiAddresses::touserdata,   { "lua_touserdata",    "type",       2 } },
        { &crabe::infrastructure::LuaApiAddresses::getmetatable, { "lua_getmetatable",  "type",       4 } },
        { &crabe::infrastructure::LuaApiAddresses::rawequal,     { "lua_rawequal",      "type",       5 } },
        { &crabe::infrastructure::LuaApiAddresses::pushlstring,  { "lua_pushlstring",   "type",       6 } },
        { &crabe::infrastructure::LuaApiAddresses::pushnil,      { "lua_pushnil",       "type",       8 } },

        { &crabe::infrastructure::LuaApiAddresses::pushboolean,  { "lua_pushboolean",   "rawequal",   4 } },
    };

    // Returns hex representation of the first count bytes at addr for diagnostics.
    std::string firstBytes(uintptr_t addr, size_t count)
    {
        if (!crabe::memory::isReadable(addr, count)) return "<unreadable>";

        const auto* code = reinterpret_cast<const uint8_t*>(addr);
        std::string out;

        for (size_t i = 0; i < count; ++i) {
            if (i) out += ' ';
            out += std::format("{:02X}", code[i]);
        }
        return out;
    }

    // Resolves a single Lua function by inspecting wrapper calls and checking expected RVA.
    // Falls back to first candidate in unverified mode if expectedRva is zero.
    uintptr_t resolveLuaFunction(const LuaSymbol& symbol, uintptr_t base,
                                const std::vector<uintptr_t>& wrappers,
                                uintptr_t expectedRva)
    {
        crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

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
            std::vector<uintptr_t> calls = crabe::memory::findCalls(wrapper, symbol.scanBytes);
            if (index > calls.size()) continue;

            uintptr_t addr = calls[index - 1];
            uintptr_t rva = addr - base;

            if (!expectedRva) {
                if (!fallback) fallback = addr;
                continue;
            }
            if (rva != expectedRva) continue;

            logger.debug("LuaSymbols: {} @ 0x{:X} (RVA 0x{:X}) [{}]",
                        symbol.name, addr, rva, firstBytes(addr, 8));
            return addr;
        }

        if (!expectedRva && fallback) {
            logger.debug("LuaSymbols: {} @ 0x{:X} (RVA 0x{:X}) [{}] (unverified: no expected RVA)",
                        symbol.name, fallback, fallback - base, firstBytes(fallback, 8));
            return fallback;
        }

        logger.error("LuaSymbols: {}: no '{}' binding ({} candidate(s)) whose call #{} lands on RVA 0x{:X}; refusing.",
                    symbol.name, symbol.stdlibName, wrappers.size(), symbol.callIndex, expectedRva);

        for (uintptr_t wrapper : wrappers) {
            std::vector<uintptr_t> calls = crabe::memory::findCalls(wrapper, symbol.scanBytes);
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

namespace crabe::lua_symbols {

// Resolves all required Lua C API symbols against base and optional game profile.
Resolution resolveAll(uintptr_t base, const crabe::domain::GameProfile* profile)
{
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

    if (!profile) {
        logger.warning("LuaSymbols: no game profile matched; every symbol will be taken from the "
                       "scan unverified. Addresses are not being checked against a measured RVA.");
    }

    std::unordered_map<std::string_view, std::vector<uintptr_t>> wrappers;
    Resolution result;

    for (const Binding& binding : kBindings) {
        auto [it, inserted] = wrappers.try_emplace(binding.symbol.stdlibName);
        if (inserted)
            it->second = crabe::memory::findRegisteredFunctions(binding.symbol.stdlibName);

        uintptr_t expectedRva = 0;
        if (profile) {
            expectedRva = profile->luaSymbolRva(binding.symbol.name);
            if (!expectedRva) {
                logger.error("LuaSymbols: {}: profile '{}' states no RVA for this symbol; "
                             "refusing to resolve it by scan alone.",
                             binding.symbol.name, profile->id);
                ++result.total;
                continue;
            }
        }

        const uintptr_t addr = resolveLuaFunction(binding.symbol, base, it->second, expectedRva);
        result.addresses.*binding.field = addr;
        ++result.total;
        if (addr) ++result.resolved;
    }

    logger.info("LuaSymbols: {}/{} symbols resolved{}.", result.resolved, result.total,
                profile ? std::format(" against profile '{}'", profile->id)
                        : std::string(" (unverified: no profile matched)"));
    return result;
}

} // namespace crabe::lua_symbols
