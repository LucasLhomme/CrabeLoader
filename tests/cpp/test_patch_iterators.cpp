/*
** CrabeLoader
** File description:
** Exercises the patch iterator bootstrap against real independent Lua 5.1 states.
** Covers missing globals, sparse arrays, false values and stack cleanup after failures.
** Runs offline; native Toy Box loading and S9 behavior require separate game testing.
**
** Authors: @Weresthebeef
*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "infrastructure/lua_patch_iterators.hpp"
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

namespace pi = crabe::infrastructure::patch_iterators;
static int checks = 0;
static void check(bool ok, const char* message) {
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
static int top(void* s) { return lua_gettop(static_cast<lua_State*>(s)); }
static void settop(void* s, int n) { lua_settop(static_cast<lua_State*>(s), n); }
static int load(void* s, const char* code, size_t n, const char* name) {
    return luaL_loadbuffer(static_cast<lua_State*>(s), code, n, name);
}
static int call(void* s, int a, int r, int e) { return lua_pcall(static_cast<lua_State*>(s), a, r, e); }
static void closure(void* s, pi::Native fn, int n) {
    lua_pushcclosure(static_cast<lua_State*>(s), reinterpret_cast<lua_CFunction>(fn), n);
}
static double number(void* s, int n) { return lua_tonumber(static_cast<lua_State*>(s), n); }
static const pi::Api api{top, settop, load, call, closure, number};
static int run(lua_State* s, const char* code) {
    const int base = lua_gettop(s);
    int status = luaL_loadstring(s, code);
    if (!status) status = lua_pcall(s, 0, 0, 0);
    if (status) std::printf("  expected/diagnostic Lua error: %s\n", lua_tostring(s, -1));
    lua_settop(s, base);
    return status;
}
int main() {
    lua_State* reference = luaL_newstate(); luaL_openlibs(reference);
    lua_getglobal(reference, "next"); auto next = reinterpret_cast<pi::Native>(lua_tocfunction(reference, -1)); lua_pop(reference, 1);
    lua_getglobal(reference, "rawget"); auto rawget = reinterpret_cast<pi::Native>(lua_tocfunction(reference, -1)); lua_pop(reference, 1);
    check(next && rawget, "zero-upvalue native wrappers available");
    lua_State* s = luaL_newstate();
    check(run(s, "for k,v in pairs({a=1}) do end") != 0, "reproduce missing pairs");
    check(run(s, "for k,v in ipairs({1}) do end") != 0, "reproduce missing ipairs");
    lua_pushnumber(s, 12345);
    check(pi::ensure(s, api, next, rawget) == 3, "install both in bare state");
    check(lua_gettop(s) == 1 && lua_tonumber(s, 1) == 12345, "existing stack value preserved");
    check(run(s, R"lua(
        local n, sawFalse = 0, false
        for k,v in pairs({a=false,b=2}) do n=n+1; if k=='a' and v==false then sawFalse=true end end
        if n~=2 or not sawFalse then broken_pairs() end
        local t={false,7,[4]=9}; local n=0
        for i,v in ipairs(t) do n=n+1; if i==1 and v~=false then lost_false() end end
        if n~=2 then crossed_hole() end
        if next~=nil or rawget~=nil or io~=nil or os~=nil or package~=nil or debug~=nil then opened_library() end
    )lua") == 0, "table/array iteration, false values, holes, no unrelated libraries");
    check(pi::ensure(s, api, next, rawget) == 0, "repeated installation is a no-op");
    check(run(s, "originalPairs=pairs; originalIpairs=ipairs") == 0, "capture iterator identity");
    check(pi::ensure(s, api, next, rawget) == 0, "preserve existing iterators");
    check(run(s, "if originalPairs~=pairs or originalIpairs~=ipairs then replaced_iterator() end") == 0, "identities unchanged");
    check(run(s, "pairs(2)") != 0, "pairs validates table argument");
    check(run(s, "ipairs(2)") != 0, "ipairs validates table argument");
    // The original CrabeMenu shape, including both iterator calls and duplicates.
    const char* fixture = R"lua(
        DataMap=DataMap or {Playset={toybox={Files={'existing'}}}}
        local extra={'munitions','surfaceImpacts'}
        for _,ps in pairs(DataMap.Playset) do
            local seen={}
            for _,f in ipairs(ps.Files) do seen[f]=true end
            for _,f in ipairs(extra) do if not seen[f] then ps.Files[#ps.Files+1]=f end end
        end
        if #DataMap.Playset.toybox.Files~=3 or DataMap.Playset.toybox.Files[1]~='existing' then bad_preload() end
    )lua";
    check(run(s, fixture) == 0, "original preload pattern succeeds");
    check(run(s, fixture) == 0, "original preload pattern remains idempotent");
    lua_State* second=luaL_newstate();
    check(run(second, "pairs=false; ipairs=nil") == 0, "second state configured independently");
    check(pi::ensure(second, api, next, rawget)==2, "install only missing iterator; preserve false override");
    check(run(second,"if pairs~=false then overwritten_override() end; local n=0; for i,v in ipairs({3}) do n=n+1 end; if n~=1 then bad_second_state() end")==0,"second-state semantics");
    check(pi::ensure(second,api,nullptr,rawget)<0,"unresolved address refused");
    check(lua_gettop(second)==0,"failure preserves stack");
    // Compile/runtime failure recovery uses the exact helper stack guard.
    pi::Api failing=api;
    failing.loadbuffer=[](void* state,const char*,size_t,const char*) -> int {
        lua_pushstring(static_cast<lua_State*>(state),"synthetic compiler error");return LUA_ERRSYNTAX;
    };
    check(pi::ensure(s,failing,next,rawget)==-2,"compilation failure reported");
    check(lua_gettop(s)==1 && lua_tonumber(s,1)==12345,"compilation failure stack restored");
    failing=api;failing.pcall=[](void* state,int,int,int)->int {
        lua_pushstring(static_cast<lua_State*>(state),"synthetic runtime error");return LUA_ERRRUN;
    };
    check(pi::ensure(s,failing,next,rawget)==-3,"runtime failure reported");
    check(lua_gettop(s)==1 && lua_tonumber(s,1)==12345,"runtime failure stack restored");
    lua_close(second);lua_close(s);lua_close(reference);
    std::printf("PASS: %d checks (real Lua 5.1.5, bare independent states)\n",checks);
}
