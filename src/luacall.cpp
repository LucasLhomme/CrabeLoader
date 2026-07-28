/*
** CrabeLoader
** File description:
** luacall
*/

#include "loader/luacall.hpp"
#include "logger/logger.hpp"

namespace {
    constexpr size_t kJumpPatchLen = 5;

    // addr == 0 means "no verified address yet": skip rather than patch a
    // guess, since a wrong address still overwrites live code and crashes
    // the host process immediately on next execution.
    bool installOne(Hook& hook, uintptr_t addr, void* detour, const char* name)
    {
        Logger& logger = Logger::getInstance();

        if (addr == 0) {
            logger.warning("LuaCall: {} skipped (no verified address).", name);
            return false;
        }
        if (!hook.install(reinterpret_cast<void*>(addr), detour, kJumpPatchLen)) {
            logger.error("LuaCall: failed to install {} hook.", name);
            return false;
        }
        logger.info("LuaCall: {} hook installed.", name);
        return true;
    }
}

LuaCall& LuaCall::get()
{
    static LuaCall instance;
    return instance;
}

bool LuaCall::initialize(uintptr_t addrLoadfile, uintptr_t addrLoadbuffer, uintptr_t addrPcall)
{
    bool anyInstalled = false;

    anyInstalled |= installOne(_hookLoadfile, addrLoadfile, reinterpret_cast<void*>(&LuaCall::hkLoadfile), "luaL_loadfile");
    anyInstalled |= installOne(_hookLoadbuffer, addrLoadbuffer, reinterpret_cast<void*>(&LuaCall::hkLoadbuffer), "luaL_loadbuffer");
    anyInstalled |= installOne(_hookPcall, addrPcall, reinterpret_cast<void*>(&LuaCall::hkPcall), "lua_pcall");

    return anyInstalled;
}

void LuaCall::uninitialize()
{
    _hookLoadfile.remove();
    _hookLoadbuffer.remove();
    _hookPcall.remove();
}

LuaCall::t_luaL_loadfile LuaCall::originalLoadfile() const
{
    return reinterpret_cast<t_luaL_loadfile>(_hookLoadfile.getOriginal());
}

LuaCall::t_luaL_loadbuffer LuaCall::originalLoadbuffer() const
{
    return reinterpret_cast<t_luaL_loadbuffer>(_hookLoadbuffer.getOriginal());
}

LuaCall::t_lua_pcall LuaCall::originalPcall() const
{
    return reinterpret_cast<t_lua_pcall>(_hookPcall.getOriginal());
}

int __cdecl LuaCall::hkLoadfile(void* L, const char* filename)
{
    Logger::getInstance().debug("hkLoadfile: {}", filename ? filename : "<null>");
    return LuaCall::get().originalLoadfile()(L, filename);
}

int __cdecl LuaCall::hkLoadbuffer(void* L, const char* buff, size_t size, const char* name)
{
    Logger::getInstance().debug("hkLoadbuffer: {}", name ? name : "<null>");
    return LuaCall::get().originalLoadbuffer()(L, buff, size, name);
}

int __cdecl LuaCall::hkPcall(void* L, int nargs, int nresults, int errfunc)
{
    Logger::getInstance().debug("hkPcall: nargs={} nresults={}", nargs, nresults);
    return LuaCall::get().originalPcall()(L, nargs, nresults, errfunc);
}
