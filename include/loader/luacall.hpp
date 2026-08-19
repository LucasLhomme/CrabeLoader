/*
** CrabeLoader
** File description:
** luacall
*/

#ifndef LUACALL_HPP_
#define LUACALL_HPP_

#include <cstdint>
#include <cstddef>
#include <functional>
#include <mutex>
#include <string>

#include "loader/hook.hpp"

// Resolved addresses of the Lua C API. loadfile/loadbuffer/pcall are hooked
// (to observe the game); everything else is only ever called. Some fields
// are unused so far but resolved anyway -- cheap now, expensive to redo.
struct LuaApiAddresses {
    uintptr_t loadfile = 0;
    uintptr_t loadbuffer = 0;
    uintptr_t pcall = 0;
    uintptr_t call = 0;

    // Stack inspection.
    uintptr_t gettop = 0;
    uintptr_t settop = 0;
    uintptr_t pushvalue = 0;

    uintptr_t tolstring = 0;
    uintptr_t tonumber = 0;
    uintptr_t toboolean = 0;
    uintptr_t touserdata = 0;
    uintptr_t isnumber = 0;

    // Pushing values onto the stack.
    uintptr_t pushnil = 0;
    uintptr_t pushnumber = 0;
    uintptr_t pushlstring = 0;
    uintptr_t pushboolean = 0;
    uintptr_t pushcclosure = 0;

    uintptr_t getfield = 0;
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

        // Compiles and runs `patchSource` against whatever chunk hkPcall just
        // executed. Failure is logged and swallowed, never propagated.
        bool runPatch(void* L, const std::string& patchSource, const std::string& label) const;

        // Compiles and runs `code`, writing its first return value or the
        // error message into `out`. False if lua_tolstring never resolved.
        bool runSnippet(void* L, const std::string& code, std::string& out) const;
        bool callTick(void* L, double dt) const;

        typedef int(__cdecl* t_lua_cfunction)(void* L);

        // Registers `cFunction` as `<tableName>.<fieldName>`, e.g. Crabe.SetWindowMode.
        // `tableName` must already exist as a global table. False if missing
        // or a required address failed to resolve.
        bool registerNativeFunction(void* L, const char* tableName, const char* fieldName,
                                     t_lua_cfunction cFunction) const;

        // Reads argument `idx` as a string/number from inside a registered
        // t_lua_cfunction; does not touch the stack height.
        const char* argToString(void* L, int idx) const;

        // `fallback` is returned on unresolved address or non-number, since
        // lua_tonumber can't distinguish a real 0 from a failure.
        double argToNumber(void* L, int idx, double fallback = 0.0) const;

        // Pushes a return value; the caller must then return the pushed
        // count. Check hasReturnSupport() first or the count goes wrong.
        void pushString(void* L, const std::string& value) const;
        void pushNumber(void* L, double value) const;
        bool hasReturnSupport() const;

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
        typedef double(__cdecl* t_lua_tonumber)(void* L, int idx);
        typedef int(__cdecl* t_lua_toboolean)(void* L, int idx);
        typedef void(__cdecl* t_lua_pushlstring)(void* L, const char* s, size_t len);
        typedef void(__cdecl* t_lua_pushcclosure)(void* L, t_lua_cfunction fn, int n);
        typedef void(__cdecl* t_lua_rawset)(void* L, int idx);

        static int __cdecl hkLoadfile(void* L, const char* filename);
        static int __cdecl hkLoadbuffer(void* L, const char* buff, size_t size, const char* name);
        static int __cdecl hkPcall(void* L, int nargs, int nresults, int errfunc);

        t_luaL_loadfile originalLoadfile() const;
        // Outcome of loading and calling one chunk. `text` is the first
        // result on success, or the error message -- the same stack slot in
        // both cases.
        struct ChunkResult {
            enum class Stage { Ok, LoadFailed, CallFailed };

            Stage stage = Stage::Ok;
            int status = 0;
            std::string text;

            explicit operator bool() const { return stage == Stage::Ok; }
        };

        // The one place a chunk is loaded, called, and the stack put back.
        // `load` does the load and returns its Lua status; every caller
        // differs only in that and in what it logs. Stack balance lives here
        // alone -- it is what silently corrupts the game when it drifts.
        ChunkResult runChunk(void* L, int nresults, const std::function<int()>& load) const;

        t_luaL_loadbuffer originalLoadbuffer() const;
        t_lua_pcall originalPcall() const;
        std::string popString(void* L, int index) const;

        Hook _hookLoadfile;
        Hook _hookLoadbuffer;
        Hook _hookPcall;

        // Every resolved address, including the ones nothing calls yet.
        t_lua_getfield _getfield = nullptr;
        t_lua_tolstring _tolstring = nullptr;
        t_lua_settop _settop = nullptr;
        t_lua_gettop _gettop = nullptr;
        t_lua_pushnumber _pushnumber = nullptr;
        t_lua_tonumber _tonumber = nullptr;
        t_lua_toboolean _toboolean = nullptr;
        t_lua_pushlstring _pushlstring = nullptr;
        t_lua_pushcclosure _pushcclosure = nullptr;
        t_lua_rawset _rawset = nullptr;

};

#endif /* !LUACALL_HPP_ */
