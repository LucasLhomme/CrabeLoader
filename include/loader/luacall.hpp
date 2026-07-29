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

#include "loader/hook.hpp"

class LuaCall {
    public:
        static LuaCall& get();

        bool initialize(uintptr_t addrLoadfile, uintptr_t addrLoadbuffer, uintptr_t addrPcall);
        void uninitialize();

        // Loads `path` with the real (unhooked) luaL_loadfile and runs it with
        // the real lua_pcall, in the given state. This is how mod scripts get
        // executed outside of the game's own loading path.
        bool runFile(void* L, const char* path) const;

    protected:
    private:
        LuaCall() = default;
        ~LuaCall() = default;
        LuaCall(const LuaCall&) = delete;
        LuaCall& operator=(const LuaCall&) = delete;

        typedef int(__cdecl* t_luaL_loadfile)(void* L, const char* filename);
        typedef int(__cdecl* t_luaL_loadbuffer)(void* L, const char* buff, size_t size, const char* name);
        typedef int(__cdecl* t_lua_pcall)(void* L, int nargs, int nresults, int errfunc);

        static int __cdecl hkLoadfile(void* L, const char* filename);
        static int __cdecl hkLoadbuffer(void* L, const char* buff, size_t size, const char* name);
        static int __cdecl hkPcall(void* L, int nargs, int nresults, int errfunc);

        t_luaL_loadfile originalLoadfile() const;
        t_luaL_loadbuffer originalLoadbuffer() const;
        t_lua_pcall originalPcall() const;

        Hook _hookLoadfile;
        Hook _hookLoadbuffer;
        Hook _hookPcall;
        std::mutex _StateMutex;

};

#endif /* !LUACALL_HPP_ */
