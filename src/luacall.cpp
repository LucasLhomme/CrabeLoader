/*
** CrabeLoader
** File description:
** luacall
*/

#include <format>

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

bool LuaCall::initialize(const LuaApiAddresses& addresses)
{
    bool anyInstalled = false;

    anyInstalled |= installOne(_hookLoadfile, addresses.loadfile,
                            reinterpret_cast<void*>(&LuaCall::hkLoadfile), "luaL_loadfile");
    anyInstalled |= installOne(_hookLoadbuffer, addresses.loadbuffer,
                            reinterpret_cast<void*>(&LuaCall::hkLoadbuffer), "luaL_loadbuffer");
    anyInstalled |= installOne(_hookPcall, addresses.pcall,
                            reinterpret_cast<void*>(&LuaCall::hkPcall), "lua_pcall");

    // Called, never hooked: these are how the loader reads values back out of
    // the Lua stack. An unresolved one degrades a feature (runSnippet returns
    // no text) rather than breaking the hooks, so it is only a warning.
    _getfield = reinterpret_cast<t_lua_getfield>(addresses.getfield);
    _tolstring = reinterpret_cast<t_lua_tolstring>(addresses.tolstring);
    _settop = reinterpret_cast<t_lua_settop>(addresses.settop);

    if (!_tolstring || !_settop) {
        Logger::getInstance().warning(
            "LuaCall: lua_tolstring/lua_settop unresolved; snippet results and errors will be unavailable.");
    }

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

std::string LuaCall::popString(void* L, int index) const
{
    if (!_tolstring || !_settop) 
        return {};

    // lua_tolstring converts the stack slot to a string in place. That is fine
    // for values we own and drop right away, but must never be aimed at a slot
    // still belonging to the game.
    const char* text = _tolstring(L, index, nullptr);
    std::string out = text ? text : "";

    _settop(L, -2); // lua_pop(L, 1)
    return out;
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
    // Called thousands of times per second: no logging, no allocation on the
    // path where nothing is queued (Loader::drainPendingKeybindCalls returns
    // immediately in that case). This is also the only place it is safe to
    // run keybind-triggered Lua calls from: it's the game's own thread
    // already holding this L, unlike the input-polling thread that queued them.
    Loader::get().drainPendingKeybindCalls(L);
    Loader::get().drainPendingSnippets(L);
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
        Logger::getInstance().error("LuaCall: luaL_loadfile('{}') failed (status {}): {}",
                                    path, loadStatus, popString(L, -1));
        return false;
    }

    // LUA_MULTRET leaves an unknown number of results, so the error object can
    // only be read on the failure path, where pcall leaves exactly one.
    int callStatus = pcall(L, 0, kLuaMultret, 0);
    if (callStatus != kLuaOk) {
        Logger::getInstance().error("LuaCall: lua_pcall('{}') failed (status {}): {}",
                                    path, callStatus, popString(L, -1));
        return false;
    }

    return true;
}

bool LuaCall::runGlobalIfExists(void* L, const std::string& functionName) const
{
    t_luaL_loadbuffer loadbuffer = originalLoadbuffer();
    t_lua_pcall pcall = originalPcall();

    if (!L || !loadbuffer || !pcall)
        return false;

    std::string chunk = std::format("if {0} then {0}() end", functionName);

    constexpr int kLuaOk = 0;
    constexpr int kLuaMultret = -1; // LUA_MULTRET

    int loadStatus = loadbuffer(L, chunk.c_str(), chunk.size(), functionName.c_str());
    if (loadStatus != kLuaOk) {
        Logger::getInstance().error("LuaCall: failed to compile keybind chunk for '{}' (status {}).",
                                    functionName, loadStatus);
        return false;
    }

    return pcall(L, 0, kLuaMultret, 0) == kLuaOk;
}

bool LuaCall::runSnippet(void* L, const std::string& code, std::string& out) const
{
    out.clear();

    t_luaL_loadbuffer loadbuffer = originalLoadbuffer();
    t_lua_pcall pcall = originalPcall();

    if (!L || !loadbuffer || !pcall) {
        out = "Lua state or hooks unavailable.";
        return false;
    }

    constexpr int kLuaOk = 0;

    // No Lua-side pcall wrapper: lua_pcall already traps runtime errors, and
    // the error object it leaves on the stack is now readable directly.
    int loadStatus = loadbuffer(L, code.c_str(), code.size(), "=console");
    if (loadStatus != kLuaOk) {
        out = popString(L, -1);
        if (out.empty())
            out = std::format("compile failed (status {}).", loadStatus);
        return false;
    }

    // nresults = 1: lua_pcall pads with nil, so exactly one slot is always
    // there to pop, whether or not the chunk returned anything.
    int callStatus = pcall(L, 0, 1, 0);
    out = popString(L, -1);

    if (callStatus != kLuaOk) {
        if (out.empty()) 
            out = std::format("runtime error (status {}).", callStatus);
        return false;
    }
    return true;
}
