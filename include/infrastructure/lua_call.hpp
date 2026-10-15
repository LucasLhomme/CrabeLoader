/*
** CrabeLoader
** File description:
** Declares the resolved Lua C API addresses and every call the loader makes through them.
** loadfile, loadbuffer and pcall are detoured to observe the game; the rest are only called.
** Finds none of those addresses -- that is infrastructure/lua_symbols.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef LUACALL_HPP_
#define LUACALL_HPP_

#include <cstdint>
#include <cstddef>
#include <functional>
#include <mutex>
#include <span>
#include <string>
#include <string_view>

#include "infrastructure/hook.hpp"

namespace crabe::infrastructure {

struct LuaApiAddresses {
    uintptr_t loadfile = 0;
    uintptr_t loadbuffer = 0;
    uintptr_t pcall = 0;
    uintptr_t call = 0;

    uintptr_t gettop = 0;
    uintptr_t settop = 0;
    uintptr_t pushvalue = 0;

    uintptr_t tolstring = 0;
    uintptr_t tonumber = 0;
    uintptr_t toboolean = 0;
    uintptr_t touserdata = 0;
    uintptr_t isnumber = 0;

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

    uintptr_t checkany = 0;
    uintptr_t checktype = 0;
    uintptr_t checklstring = 0;
};

class LuaCall {
    public:
        // Returns the singleton instance of LuaCall.
        static LuaCall& get();

        // Initializes LuaCall hooks and resolved API addresses.
        bool initialize(const LuaApiAddresses& addresses);

        // Uninitializes LuaCall and removes installed hooks.
        void uninitialize();

        // Loads and executes a Lua file from disk in the given Lua state.
        bool runFile(void* L, const char* path) const;

        // Compiles and runs a Lua code buffer in the given Lua state.
        bool runBuffer(void* L, const char* buff, size_t size, const char* name = nullptr) const;

        // Calls a global Lua function by name if it exists.
        bool runGlobalIfExists(void* L, const std::string& functionName) const;

        // Compiles and executes a patch string against the active Lua state.
        bool runPatch(void* L, const std::string& patchSource, const std::string& label) const;

        // Compiles and executes Lua code, writing the result or error to out.
        bool runSnippet(void* L, const std::string& code, std::string& out) const;

        // Compiles and executes a Lua code snippet.
        bool runSnippet(void* L, const std::string& code) const;

        // Loads chunk as a function and calls it passing args as string parameters.
        // Returns true on success or populates outError on failure.
        bool runChunkWithArgs(void* L,
                              std::string_view chunk,
                              std::span<const std::string> args,
                              std::string& outError) const;

        // Invokes the registered tick callback with delta time dt.
        bool callTick(void* L, double dt) const;

        // Checks whether global name is defined and non-nil/non-false in L.
        [[nodiscard]] bool hasGlobal(void* L, const char* name) const;

        // Calls Crabe.Mod.dispatchDraw on the script thread.
        bool dispatchModDraw(void* L) const;

        typedef int(__cdecl* t_lua_cfunction)(void* L);

        // Registers a C function into a global table as tableName.fieldName.
        bool registerNativeFunction(void* L, const char* tableName, const char* fieldName,
                                     t_lua_cfunction cFunction) const;

        // Reads an argument at index as a string without modifying the stack.
        const char* argToString(void* L, int idx) const;

        // Reads an argument at index as a number, returning fallback on failure.
        double argToNumber(void* L, int idx, double fallback = 0.0) const;

        // Reads an argument at index as a boolean, returning fallback on failure.
        bool argToBoolean(void* L, int idx, bool fallback = false) const;

        // Pushes a string onto the Lua stack.
        void pushString(void* L, const std::string& value) const;

        // Pushes a number onto the Lua stack.
        void pushNumber(void* L, double value) const;

        // Pushes a boolean onto the Lua stack.
        void pushBoolean(void* L, bool value) const;

        // Pushes nil onto the Lua stack.
        void pushNil(void* L) const;

        // Returns the index of the top element on the stack.
        int getTop(void* L) const;

        // Checks whether the value at index is a number.
        bool isNumber(void* L, int idx) const;

        // Checks whether native return value helpers are supported.
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
        typedef void(__cdecl* t_lua_pushboolean)(void* L, int b);
        typedef void(__cdecl* t_lua_pushnil)(void* L);
        typedef int(__cdecl* t_lua_isnumber)(void* L, int idx);
        typedef void(__cdecl* t_lua_pushcclosure)(void* L, t_lua_cfunction fn, int n);
        typedef void(__cdecl* t_lua_rawset)(void* L, int idx);

        // Detour for luaL_loadfile.
        static int __cdecl hkLoadfile(void* L, const char* filename);

        // Detour for luaL_loadbuffer.
        static int __cdecl hkLoadbuffer(void* L, const char* buff, size_t size, const char* name);

        // Detour for lua_pcall.
        static int __cdecl hkPcall(void* L, int nargs, int nresults, int errfunc);

        // Returns original luaL_loadfile function pointer.
        t_luaL_loadfile originalLoadfile() const;

        struct ChunkResult {
            enum class Stage { Ok, LoadFailed, CallFailed };

            Stage stage = Stage::Ok;
            int status = 0;
            std::string text;

            explicit operator bool() const { return stage == Stage::Ok; }
        };

        // Loads and executes a chunk using the load callback, balancing the stack.
        ChunkResult runChunk(void* L, int nresults, const std::function<int()>& load) const;

        // Returns original luaL_loadbuffer function pointer.
        t_luaL_loadbuffer originalLoadbuffer() const;

        // Returns original lua_pcall function pointer.
        t_lua_pcall originalPcall() const;

        // Pops a string from the Lua stack at index.
        std::string popString(void* L, int index) const;

        Hook _hookLoadfile;
        Hook _hookLoadbuffer;
        Hook _hookPcall;

        t_lua_getfield _getfield = nullptr;
        t_lua_tolstring _tolstring = nullptr;
        t_lua_settop _settop = nullptr;
        t_lua_gettop _gettop = nullptr;
        t_lua_pushnumber _pushnumber = nullptr;
        t_lua_tonumber _tonumber = nullptr;
        t_lua_toboolean _toboolean = nullptr;
        t_lua_pushlstring _pushlstring = nullptr;
        t_lua_pushboolean _pushboolean = nullptr;
        t_lua_pushnil _pushnil = nullptr;
        t_lua_isnumber _isnumber = nullptr;
        t_lua_pushcclosure _pushcclosure = nullptr;
        t_lua_rawset _rawset = nullptr;
};

} // namespace crabe::infrastructure

#endif /* !LUACALL_HPP_ */
