/*
** CrabeLoader
** File description:
** Implements native key and controller binding options for InGameSettingsMenu.
** Populates keyboard and gamepad binding rows directly below the Controls option.
** Manages input remapping state transitions and delegates to the controls interface.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/patch/patch_keybind.hpp"

#include <array>
#include <string_view>

#include "infrastructure/lua_call.hpp"
#include "presentation/input_remap.hpp"
#include "shared/logger.hpp"

namespace {

struct KeybindRowDef {
    const char* id;
    const char* label;
    const char* page;
};

constexpr std::array<KeybindRowDef, 2> kKeybindRows = {{
    { "Crabe_KeyboardBindings", "Keyboard and mouse bindings", "keyboard" },
    { "Crabe_PadBindings", "Controller bindings", "pad" },
}};

// Lua C callback querying whether active input remappings are present.
int __cdecl nativeIsRemapActive(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport())
        return 0;

    lua.pushBoolean(L, crabe::presentation::InputRemap::get().isActive());
    return 1;
}

// Lua C callback clearing all configured input remappings.
int __cdecl nativeClearRemaps(void* L)
{
    (void)L;
    crabe::presentation::InputRemap::get().clearAll();
    return 0;
}

constexpr std::string_view kInitScript =
    "if Crabe and Crabe.Settings and not _G.__crabeKeybindsInitialized then\n"
    "    _G.__crabeKeybindsInitialized = true\n"
    "    local ok, mod = pcall(require, 'mods.keybinds.main')\n"
    "    if not ok and Crabe.write then\n"
    "        Crabe.write('[PatchKeybind] Note: ' .. tostring(mod))\n"
    "    end\n"
    "end\n";

} // namespace

namespace crabe::infrastructure::patch {

// Applies native keybind options and hooks to InGameSettingsMenu.
bool PatchKeybind::apply(void* L)
{
    if (!L)
        return false;

    auto& lua = LuaCall::get();
    bool ok = true;

    ok = lua.registerNativeFunction(L, "Crabe", "_patchIsRemapActive", &nativeIsRemapActive) && ok;
    ok = lua.registerNativeFunction(L, "Crabe", "_patchClearRemaps", &nativeClearRemaps) && ok;

    if (!ok) {
        shared::Logger::getInstance().error("PatchKeybind: failed to bind native remap callbacks.");
        return false;
    }

    ok = lua.runSnippet(L, std::string(kInitScript));
    if (ok) {
        shared::Logger::getInstance().info("PatchKeybind: applied in-game keybinds patch.");
    } else {
        shared::Logger::getInstance().error("PatchKeybind: failed to initialize in-game keybinds.");
    }
    return ok;
}

} // namespace crabe::infrastructure::patch

