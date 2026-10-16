/*
** CrabeLoader
** File description:
** Lua natives that edit the input remap table: set one entry, or clear a device or everything.
** Numbers only: a Lua boolean read as a number silently becomes false (see argToBoolean).
** Devices: 0 keyboard (DIK scan codes), 1 mouse buttons (0-based), 2 pad buttons (XINPUT masks).
**
** Authors: @LucasLhomme
*/

#include <string>
#include <windows.h>

#include "application/loader.hpp"
#include "infrastructure/lua_call.hpp"
#include "presentation/dinput_hook.hpp"
#include "presentation/input_remap.hpp"

namespace {

    enum class Device : int { Keyboard = 0, Mouse = 1, Pad = 2 };

    constexpr double kNoDevice = -1.0;

}

// Lua binding for Crabe._inputRemap; registers an input remap rule.
int __cdecl crabe::input_natives::remapSet(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    auto device = static_cast<int>(lua.argToNumber(L, 1, kNoDevice));
    auto from = static_cast<int>(lua.argToNumber(L, 2, -1.0));
    auto to = static_cast<int>(lua.argToNumber(L, 3, -1.0));

    bool stored = false;
    if (from >= 0) {
        crabe::presentation::InputRemap& remap = crabe::presentation::InputRemap::get();
        switch (static_cast<Device>(device)) {
            case Device::Keyboard: stored = remap.setKey(static_cast<uint32_t>(from), to); break;
            case Device::Mouse: stored = remap.setMouseButton(static_cast<uint32_t>(from), to); break;
            case Device::Pad: stored = remap.setPadButton(static_cast<uint32_t>(from), to < 0 ? 0u : static_cast<uint32_t>(to)); break;
        }
    }

    lua.pushBoolean(L, stored);
    return 1;
}

// Lua binding for Crabe._inputRemapClear; clears remap entries for a device or all devices.
int __cdecl crabe::input_natives::remapClear(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    crabe::presentation::InputRemap& remap = crabe::presentation::InputRemap::get();

    auto device = static_cast<int>(lua.argToNumber(L, 1, kNoDevice));
    switch (static_cast<Device>(device)) {
        case Device::Keyboard: remap.clearKeys(); break;
        case Device::Mouse: remap.clearMouseButtons(); break;
        case Device::Pad: remap.clearPadButtons(); break;
        default: remap.clearAll(); break;
    }
    return 0;
}

// Lua binding for Crabe._inputStatus; pushes whether DirectInput is captured, whether any remap is active
// and how many virtual keys are captured.
int __cdecl crabe::input_natives::inputStatus(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    constexpr int kLastVirtualKey = 255;
    int captured = 0;
    for (int key = 1; key <= kLastVirtualKey; ++key) {
        if (crabe::application::Loader::get().isKeyCaptured(key)) ++captured;
    }

    lua.pushBoolean(L, crabe::presentation::DirectInputHook::get().isCaptured());
    lua.pushBoolean(L, crabe::presentation::InputRemap::get().isActive());
    lua.pushNumber(L, static_cast<double>(captured));
    return 3;
}

// Lua binding for Crabe._keyScanCode; converts a virtual key to a DirectInput scan code.
int __cdecl crabe::input_natives::keyScanCode(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    auto key = static_cast<UINT>(lua.argToNumber(L, 1, 0.0));
    UINT mapped = (key == 0 || key > 254) ? 0 : MapVirtualKeyW(key, MAPVK_VK_TO_VSC_EX);

    UINT scan = mapped & 0xFF;
    bool extended = (mapped & 0xFF00) == 0xE000 || (mapped & 0xFF00) == 0xE100;
    lua.pushNumber(L, scan == 0 ? 0.0 : static_cast<double>(extended ? (scan | 0x80) : scan));
    return 1;
}

// Lua binding for Crabe._keyName; retrieves the localized name of a scan code.
int __cdecl crabe::input_natives::keyName(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    auto code = static_cast<LONG>(lua.argToNumber(L, 1, 0.0));
    std::string name;
    if (code > 0 && code < 256) {
        LONG lParam = ((code & 0x7F) << 16) | ((code & 0x80) ? (1 << 24) : 0);
        wchar_t wide[64] = {};
        int length = GetKeyNameTextW(lParam, wide, static_cast<int>(sizeof(wide) / sizeof(wide[0])));
        if (length > 0) {
            int bytes = WideCharToMultiByte(CP_UTF8, 0, wide, length, nullptr, 0, nullptr, nullptr);
            name.resize(static_cast<size_t>(bytes));
            WideCharToMultiByte(CP_UTF8, 0, wide, length, name.data(), bytes, nullptr, nullptr);
        }
    }

    lua.pushString(L, name);
    return 1;
}

// Lua binding for Crabe._mouseWheel; consumes and returns accumulated wheel delta.
int __cdecl crabe::input_natives::mouseWheel(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    lua.pushNumber(L, static_cast<double>(crabe::presentation::DirectInputHook::get().takeWheelNotches()));
    return 1;
}

