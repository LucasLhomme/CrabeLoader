/*
** CrabeLoader
** File description:
** luacall
*/

#include "loader/luacall.hpp"
#include "loader/loader.hpp"
#include "logger/logger.hpp"

namespace {
    // A resolved address of 0 means "not found": skip it rather than patch a
    // guess, since a wrong address overwrites live code and crashes the host
    // process on the next execution.
    bool installOne(Hook& hook, uintptr_t addr, void* detour, const char* name)
    {
        Logger& logger = Logger::getInstance();

        if (addr == 0) {
            logger.warning("LuaCall: {} skipped (address not resolved).", name);
            return false;
        }
        if (!hook.install(reinterpret_cast<void*>(addr), detour)) {
            logger.error("LuaCall: failed to hook {} at 0x{:X}.", name, addr);
            return false;
        }
        logger.info("LuaCall: {} hooked.", name);
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

    anyInstalled |= installOne(_hookLoadfile, addrLoadfile,
                            reinterpret_cast<void*>(&LuaCall::hkLoadfile), "luaL_loadfile");
    anyInstalled |= installOne(_hookLoadbuffer, addrLoadbuffer,
                            reinterpret_cast<void*>(&LuaCall::hkLoadbuffer), "luaL_loadbuffer");
    anyInstalled |= installOne(_hookPcall, addrPcall,
                            reinterpret_cast<void*>(&LuaCall::hkPcall), "lua_pcall");

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
    Logger::getInstance().debug("Lua: loadfile {}", filename ? filename : "<null>");
    Loader::get().onLuaState(L);
    return LuaCall::get().originalLoadfile()(L, filename);
}

int __cdecl LuaCall::hkLoadbuffer(void* L, const char* buff, size_t size, const char* name)
{
    Logger::getInstance().debug("Lua: loadbuffer {} ({} bytes)", name ? name : "<null>", size);
    // This game ships its Lua as precompiled bytecode buffers: it never calls
    // loadfile, so this is the real (and only) place a valid, fully-set-up
    // Lua state is observed. onLuaState() is a no-op after the first call, so
    // the extra check here is cheap even though loadbuffer fires constantly.
    Loader::get().onLuaState(L);
    return LuaCall::get().originalLoadbuffer()(L, buff, size, name);
}

int __cdecl LuaCall::hkPcall(void* L, int nargs, int nresults, int errfunc)
{
    // Called thousands of times per second: no logging, no allocation.
    return LuaCall::get().originalPcall()(L, nargs, nresults, errfunc);
}

bool LuaCall::runFile(void* L, const char* path) const
{
    t_luaL_loadfile loadfile = originalLoadfile();
    t_lua_pcall pcall = originalPcall();

    if (!L || !loadfile || !pcall) {
        Logger::getInstance().error("LuaCall: cannot run '{}': Lua state or hooks unavailable.", path);
        return false;
    }

    constexpr int kLuaOk = 0;
    constexpr int kLuaMultret = -1; // LUA_MULTRET

    int loadStatus = loadfile(L, path);
    if (loadStatus != kLuaOk) {
        // The error message luaL_loadfile pushed onto the stack is left there:
        // we don't have lua_tostring/lua_pop resolved yet to read and clear it.
        Logger::getInstance().error("LuaCall: luaL_loadfile('{}') failed (status {}).", path, loadStatus);
        return false;
    }

    int callStatus = pcall(L, 0, kLuaMultret, 0);
    if (callStatus != kLuaOk) {
        Logger::getInstance().error("LuaCall: lua_pcall('{}') failed (status {}).", path, callStatus);
        return false;
    }

    return true;
}
