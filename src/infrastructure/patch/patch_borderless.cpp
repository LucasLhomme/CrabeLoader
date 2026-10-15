/*
** CrabeLoader
** File description:
** Implements the in-game borderless window mode toggle for SettingsVideo.
** Uses Crabe.Settings to add a Toggle widget to the native video menu.
** Interacts directly with the display subsystem through RenderHook.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/patch/patch_borderless.hpp"

#include <string_view>

#include "infrastructure/lua_call.hpp"
#include "presentation/render_hook.hpp"
#include "shared/logger.hpp"

namespace {

// Lua C callback querying active borderless window state.
int __cdecl nativeGetBorderless(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport())
        return 0;

    const bool borderless = (crabe::presentation::RenderHook::get().getCurrentWindowMode() ==
                             crabe::presentation::WindowMode::BorderlessWindowed);
    lua.pushBoolean(L, borderless);
    return 1;
}

// Lua C callback updating active window display mode.
int __cdecl nativeSetBorderless(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    bool borderless = false;

    if (lua.isNumber(L, 3)) {
        borderless = lua.argToNumber(L, 3) != 0.0;
    } else {
        borderless = lua.argToBoolean(L, 3);
    }

    const auto mode = borderless ? crabe::presentation::WindowMode::BorderlessWindowed
                                 : crabe::presentation::WindowMode::Windowed;
    crabe::presentation::RenderHook::get().requestWindowMode(mode);
    return 0;
}

constexpr std::string_view kRegisterScript =
    "if Crabe and Crabe.Settings and Crabe.Settings.addOption then\n"
    "    Crabe.Settings.addOption(\"SettingsVideo\", {\n"
    "        id = \"crabeWindowMode\",\n"
    "        text = \"Borderless Window\",\n"
    "        widgetType = \"Toggle\",\n"
    "        get = Crabe._patchGetBorderless,\n"
    "        set = Crabe._patchSetBorderless,\n"
    "    })\n"
    "end\n";

} // namespace

namespace crabe::infrastructure::patch {

// Injects the borderless window toggle option into SettingsVideo.
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

    ok = lua.runSnippet(L, std::string(kRegisterScript));
    if (ok) {
        shared::Logger::getInstance().info("PatchBorderless: injected borderless window toggle into SettingsVideo.");
    } else {
        shared::Logger::getInstance().error("PatchBorderless: failed to register SettingsVideo option.");
    }
    return ok;
}

} // namespace crabe::infrastructure::patch
