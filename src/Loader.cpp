/*
** CrabeLoader
** File description:
** Loader
*/

#include <cstdint>
#include <format>
#include <string>
#include <windows.h>

#include "loader/loader.hpp"
#include "loader/luacall.hpp"
#include "loader/memory.hpp"
#include "logger/logger.hpp"

namespace {

    // The game exports no Lua symbol, and byte signatures proved unreliable on
    // this build. What does work is the standard library's own registration
    // table: each stdlib name is bound to a small wrapper, which delegates to the
    // C API function we actually want. So: find the wrapper by name, then follow
    // the n-th `call` inside it.
    //
    // Call indexes were read off the shipped binary; resolveLuaFunction() logs the
    // resulting address and its first bytes so a mismatch after a game update is
    // visible in the log instead of crashing the process.
    struct LuaSymbol {
        const char* name;        // what we are resolving, for the log
        const char* stdlibName;  // Lua stdlib entry holding the wrapper
        int callIndex;           // 1-based `call` inside the wrapper
    };

    constexpr LuaSymbol kLoadfile   { "luaL_loadfile",   "loadfile",   2 };
    constexpr LuaSymbol kLoadbuffer { "luaL_loadbuffer", "loadstring", 3 };
    constexpr LuaSymbol kPcall      { "lua_pcall",       "xpcall",     4 };

    std::string firstBytes(uintptr_t addr, size_t count)
    {
        if (!Memory::IsReadable(addr, count)) return "<unreadable>";

        const auto* code = reinterpret_cast<const uint8_t*>(addr);
        std::string out;

        for (size_t i = 0; i < count; ++i) {
            if (i) out += ' ';
            out += std::format("{:02X}", code[i]);
        }
        return out;
    }

    uintptr_t resolveLuaFunction(const LuaSymbol& symbol, uintptr_t base)
    {
        Logger& logger = Logger::getInstance();

        uintptr_t wrapper = Memory::FindRegisteredFunction(symbol.stdlibName);
        if (!wrapper) {
            logger.error("Loader: {}: no '{}' entry in the Lua stdlib table.",
                        symbol.name, symbol.stdlibName);
            return 0;
        }

        uintptr_t addr = Memory::FindNthCall(wrapper, symbol.callIndex);
        if (!addr) {
            logger.error("Loader: {}: call #{} not found in the '{}' wrapper (0x{:X}).",
                        symbol.name, symbol.callIndex, symbol.stdlibName, wrapper);
            return 0;
        }

        logger.info("Loader: {} @ 0x{:X} (RVA 0x{:X}) [{}]",
                    symbol.name, addr, addr - base, firstBytes(addr, 8));
        return addr;
    }

} // namespace

Loader& Loader::get()
{
    static Loader instance;
    return instance;
}

bool Loader::initialize()
{
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));

    uintptr_t loadfile = resolveLuaFunction(kLoadfile, base);
    uintptr_t loadbuffer = resolveLuaFunction(kLoadbuffer, base);
    uintptr_t pcall = resolveLuaFunction(kPcall, base);

    if (!LuaCall::get().initialize(loadfile, loadbuffer, pcall)) {
        Logger::getInstance().error("Loader: failed to initialize LuaCall.");
        return false;
    }

    Logger::getInstance().info("Loader: initialized.");
    return true;
}

void Loader::uninitialize()
{
    LuaCall::get().uninitialize();
}
