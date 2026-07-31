/*
** CrabeLoader
** File description:
** luacall
*/

#ifndef LUACALL_HPP_
#define LUACALL_HPP_

#include <cstdint>
#include <cstddef>
#include <mutex>
#include <string>

#include "loader/hook.hpp"

// Resolved addresses of the Lua C API. loadfile/loadbuffer/pcall are hooked
// (to observe the game); everything else is only ever called.
//
// Several of these are unused so far. They are resolved anyway: the cost is one
// scan at startup, whereas recovering an address afterwards means redoing the
// whole wrapper-dump exercise. The set below is exactly what a native
// (C++-side) Game API needs -- read the arguments, push the result, register
// the function in a table.
struct LuaApiAddresses {
    // Chunk loading and calling.
    uintptr_t loadfile = 0;
    uintptr_t loadbuffer = 0;
    uintptr_t pcall = 0;
    uintptr_t call = 0;

    // Stack inspection.
    uintptr_t gettop = 0;
    uintptr_t settop = 0;       // lua_pop is a 5.1 macro over this
    uintptr_t pushvalue = 0;

    // Reading values off the stack.
    uintptr_t tolstring = 0;    // lua_tostring is a 5.1 macro over this
    uintptr_t tonumber = 0;
    uintptr_t toboolean = 0;
    uintptr_t touserdata = 0;
    uintptr_t isnumber = 0;

    // Pushing values onto the stack.
    uintptr_t pushnil = 0;
    uintptr_t pushnumber = 0;
    uintptr_t pushlstring = 0;  // lua_pushstring is a 5.1 macro over this
    uintptr_t pushboolean = 0;
    uintptr_t pushcclosure = 0; // what makes a C++ function callable from Lua

    // Table access.
    uintptr_t getfield = 0;     // lua_getglobal is a 5.1 macro over this
    uintptr_t rawget = 0;
    uintptr_t rawset = 0;
    uintptr_t rawequal = 0;
    uintptr_t getmetatable = 0;

    // Argument-checking helpers (luaL_*).
    uintptr_t checkany = 0;
    uintptr_t checktype = 0;
    uintptr_t checklstring = 0;
};

class LuaCall {
    public:
        static LuaCall& get();

        bool initialize(const LuaApiAddresses& addresses);
        void uninitialize();

        // Loads `path` with the real (unhooked) luaL_loadfile and runs it with
        // the real lua_pcall, in the given state. This is how mod scripts get
        // executed outside of the game's own loading path.
        bool runFile(void* L, const char* path) const;

        bool runGlobalIfExists(void* L, const std::string& functionName) const;

        // Compiles and runs `code` in `L`, writing the chunk's first return
        // value or the compile/runtime error message into `out`. Reading
        // either back requires lua_tolstring, so this returns false with an
        // empty `out` when that address could not be resolved.
        bool runSnippet(void* L, const std::string& code, std::string& out) const;
        bool callTick(void* L, double dt) const;

    protected:
    private:
        LuaCall() = default;
        ~LuaCall() = default;
        LuaCall(const LuaCall&) = delete;
        LuaCall& operator=(const LuaCall&) = delete;

        typedef int(__cdecl* t_luaL_loadfile)(void* L, const char* filename);
        typedef int(__cdecl* t_luaL_loadbuffer)(void* L, const char* buff, size_t size, const char* name);
        typedef int(__cdecl* t_lua_pcall)(void* L, int nargs, int nresults, int errfunc);
        typedef void(__cdecl* t_lua_getfield)(void* L, int idx, const char* k);
        typedef const char*(__cdecl* t_lua_tolstring)(void* L, int idx, size_t* len);
        typedef void(__cdecl* t_lua_settop)(void* L, int idx);
        typedef int(__cdecl* t_lua_gettop)(void* L);
        typedef void(__cdecl* t_lua_pushnumber)(void* L, double n);
        typedef int(__cdecl* t_lua_toboolean)(void* L, int idx);

        static int __cdecl hkLoadfile(void* L, const char* filename);
        static int __cdecl hkLoadbuffer(void* L, const char* buff, size_t size, const char* name);
        static int __cdecl hkPcall(void* L, int nargs, int nresults, int errfunc);

        t_luaL_loadfile originalLoadfile() const;
        t_luaL_loadbuffer originalLoadbuffer() const;
        t_lua_pcall originalPcall() const;
        std::string popString(void* L, int index) const;

        Hook _hookLoadfile;
        Hook _hookLoadbuffer;
        Hook _hookPcall;

        // Every resolved address, including the ones nothing calls yet.
        LuaApiAddresses _api;
        t_lua_getfield _getfield = nullptr;
        t_lua_tolstring _tolstring = nullptr;
        t_lua_settop _settop = nullptr;
        t_lua_gettop _gettop = nullptr;
        t_lua_pushnumber _pushnumber = nullptr;
        t_lua_toboolean _toboolean = nullptr;
        std::mutex _stateMutex;

};

#endif /* !LUACALL_HPP_ */
