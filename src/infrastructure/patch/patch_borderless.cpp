/*
** CrabeLoader
** File description:
** Implements native borderless window mode callbacks for SettingsVideo.
** Exposes native window mode getter and setter directly through LuaCall.
** Interacts directly with the display subsystem through RenderHook.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/patch/patch_borderless.hpp"

#include "infrastructure/lua_call.hpp"
#include "presentation/render_hook.hpp"
#include "shared/logger.hpp"

namespace {

// Lua C callback querying active borderless window state choice index.
int __cdecl nativeGetBorderless(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport())
        return 0;

    const bool borderless = (crabe::presentation::RenderHook::get().getCurrentWindowMode() ==
                             crabe::presentation::WindowMode::BorderlessWindowed);
    lua.pushNumber(L, borderless ? 2.0 : 1.0);
    return 1;
}

// Lua C callback updating active window display mode from selected choice index.
int __cdecl nativeSetBorderless(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    const int index = static_cast<int>(lua.argToNumber(L, 3, 1.0));
    const bool borderless = (index == 2);
    const auto mode = borderless ? crabe::presentation::WindowMode::BorderlessWindowed
                                 : crabe::presentation::WindowMode::Windowed;
    crabe::presentation::RenderHook::get().requestWindowMode(mode);
    return 0;
}

} // namespace

namespace crabe::infrastructure::patch {

// Binds native borderless window mode callbacks to the Lua runtime.
bool PatchBorderless::apply(void* L)
{
    if (!L)
        return false;

    auto& lua = LuaCall::get();
    bool ok = true;

    ok = lua.registerNativeFunction(L, "Crabe", "_patchGetBorderless", &nativeGetBorderless) && ok;
    ok = lua.registerNativeFunction(L, "Crabe", "_patchSetBorderless", &nativeSetBorderless) && ok;

    if (!ok) {
        shared::Logger::getInstance().error("PatchBorderless: failed to bind native window mode callbacks.");
        return false;
    }

    shared::Logger::getInstance().info("PatchBorderless: registered native window mode callbacks.");
    return true;
}

} // namespace crabe::infrastructure::patch
