/*
** CrabeLoader
** File description:
** Supplies missing pairs and ipairs immediately before patches run in restricted Lua states.
** Uses profile-verified zero-upvalue next/rawget wrappers and restores the incoming stack top.
** Reuses no Lua objects across states and changes no existing iterator definitions.
**
** Authors: @Weresthebeef
*/
#pragma once

#include <cstddef>

namespace crabe::infrastructure::patch_iterators {

using Native = int(__cdecl*)(void*);
struct Api {
    int(__cdecl* gettop)(void*);
    void(__cdecl* settop)(void*, int);
    int(__cdecl* loadbuffer)(void*, const char*, size_t, const char*);
    int(__cdecl* pcall)(void*, int, int, int);
    void(__cdecl* pushcclosure)(void*, Native, int);
    double(__cdecl* tonumber)(void*, int);
};

// These helpers capture new C closures in this state, never Lua objects from
// another VM. rawget/next are the game's own validated, zero-upvalue wrappers.
inline constexpr char source[] = R"lua(
local nativeNext, nativeRawget = ...
local installed = 0
if pairs == nil then
    pairs = function(t)
        nativeRawget(t, 0) -- Lua 5.1 table argument validation, no metamethods.
        return nativeNext, t, nil
    end
    installed = installed + 1
end
if ipairs == nil then
    local function advance(t, i)
        i = i + 1
        local v = nativeRawget(t, i)
        if v ~= nil then return i, v end
    end
    ipairs = function(t)
        nativeRawget(t, 0)
        return advance, t, 0
    end
    installed = installed + 2
end
return installed
)lua";

// Returns installed-bitmask (pairs=1, ipairs=2), or a negative failure code.
// Balanced even with arguments/results already on the target state's stack.
inline int ensure(void* state, const Api& api, Native next, Native rawget)
{
    if (!state || !next || !rawget || !api.gettop || !api.settop ||
        !api.loadbuffer || !api.pcall || !api.pushcclosure || !api.tonumber)
        return -1;
    const int top = api.gettop(state);
    int result = -2;
    if (api.loadbuffer(state, source, sizeof(source) - 1, "=crabe_patch_iterators") == 0) {
        api.pushcclosure(state, next, 0);
        api.pushcclosure(state, rawget, 0);
        if (api.pcall(state, 2, 1, 0) == 0)
            result = static_cast<int>(api.tonumber(state, -1));
        else
            result = -3;
    }
    api.settop(state, top);
    return result;
}

} // namespace crabe::infrastructure::patch_iterators
