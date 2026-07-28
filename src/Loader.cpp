/*
** CrabeLoader
** File description:
** Loader
*/

#include <cstdint>
#include <windows.h>

#include "loader/loader.hpp"
#include "loader/luacall.hpp"
#include "loader/memory.hpp"
#include "logger/logger.hpp"

namespace {

    constexpr uintptr_t kOffsetLoadfile = 0;
    constexpr uintptr_t kOffsetLoadbuffer = 0;
    constexpr uintptr_t kOffsetPcall = 0x15ddf5a;
    constexpr const char* kSignaturePcall = "8B FF 55 8B EC 83 EC 10 57 8B 7D 0C 89 7D";

    uintptr_t resolve(uintptr_t base, uintptr_t offset)
    {
        return offset == 0 ? 0 : base + offset;
    }

    // Prefers the pattern scan (survives game rebuilds) falls back to the
    // known-verified RVA if the scan fails, and warns if the two disagree
    // rather than trusting an unverified signature over a confirmed address.
    uintptr_t resolvePcall(uintptr_t base)
    {
        Logger& logger = Logger::getInstance();
        uintptr_t rvaAddr = resolve(base, kOffsetPcall);
        uintptr_t scanAddr = Memory::PatternScan(kSignaturePcall);

        if (scanAddr == 0) {
            logger.warning("Loader: lua_pcall pattern scan failed, falling back to known RVA.");
            return rvaAddr;
        }
        if (scanAddr != rvaAddr) {
            logger.warning("Loader: lua_pcall pattern scan (0x{:X}) disagrees with known RVA (0x{:X}); using the verified RVA.", scanAddr, rvaAddr);
            return rvaAddr;
        }
        logger.debug("Loader: lua_pcall pattern scan matches known RVA (0x{:X}).", scanAddr);
        return scanAddr;
    }
}

Loader& Loader::get()
{
    static Loader instance;
    return instance;
}

bool Loader::initialize()
{
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));

    if (!LuaCall::get().initialize(resolve(base, kOffsetLoadfile), resolve(base, kOffsetLoadbuffer), resolvePcall(base))) {
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
