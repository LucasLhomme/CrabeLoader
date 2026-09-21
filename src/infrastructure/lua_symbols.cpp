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

    // The game exports no Lua symbol; each stdlib name is bound to a small
    // wrapper that delegates to the real C API function, so the real address
    // is the n-th `call` inside that wrapper.
    //
    // What used to sit here as well -- the RVA each symbol must land on -- now
    // lives in the GameProfile for the build being run, because an RVA is a
    // measurement of one executable while everything below is a fact about how
    // Lua 5.1 registers its stdlib, true of any build.
    struct LuaSymbol {
        const char* name;        // what we are resolving; also the profile's key
        const char* stdlibName;  // Lua stdlib entry holding the wrapper
        int callIndex;           // 1-based `call` inside the wrapper
        size_t scanBytes = 256;  // how far into the wrapper to look
    };

    // Which LuaApiAddresses field each symbol fills, stated once so the pairing
    // cannot drift. Index is a hint, RVA is the contract: a mismatch refuses
    // rather than hooking a wrong address. "type" resolves to io.type (found
    // before luaB_type); "wrap" is coroutine.wrap.
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

    // Tries every stdlib binding of symbol.stdlibName and keeps the one whose
    // call #callIndex lands on expectedRva; a mismatch refuses rather than
    // hooking a wrong address.
    //
    // expectedRva is 0 only in degraded mode -- no profile matched, so there is
    // nothing to confirm against and the first candidate is taken and logged as
    // unverified. Under a matched profile the refusal is absolute (invariant
    // I8), including when the profile itself states no RVA for the symbol:
    // "the profile does not mention it" is not permission to hook whatever the
    // scan happened to find.
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

Resolution resolveAll(uintptr_t base, const crabe::domain::GameProfile* profile)
{
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

    if (!profile) {
        logger.warning("LuaSymbols: no game profile matched; every symbol will be taken from the "
                       "scan unverified. Addresses are not being checked against a measured RVA.");
    }

    // Twenty-five symbols share eleven stdlib names ("print" alone carries
    // six). Each lookup walks the whole image, so they are cached here and
    // every symbol on the same name reuses one walk.
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
                // Refuse rather than fall through to the unverified path: in a
                // matched profile a missing symbol is a hole in the profile,
                // and hooking on a hole is exactly what I8 forbids.
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

}
