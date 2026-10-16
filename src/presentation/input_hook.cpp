/*
** CrabeLoader
** File description:
** Hooks XInputGetState to count which controller slots the game polls and store what they return.
** The image imports XINPUT9_1_0.dll specifically, so that is the module hooked, not any other.
** Every call is forwarded; PadStateStore keeps the state and blanks it while a mod captures it.
**
** Authors: @LucasLhomme
*/

#include <format>
#include <string>
#include <windows.h>
#include <Xinput.h>

#include "presentation/dinput_hook.hpp"
#include "presentation/input_hook.hpp"
#include "presentation/input_remap.hpp"
#include "infrastructure/lua_call.hpp"
#include "shared/logger.hpp"

namespace crabe::presentation {

// Returns the singleton instance of InputHook.
InputHook& InputHook::get()
{
    static InputHook instance;
    return instance;
}

// Installs the XInputGetState hook on XINPUT9_1_0.dll.
bool InputHook::initialize()
{
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

    HMODULE module = GetModuleHandleW(L"XINPUT9_1_0.dll");
    if (!module) {
        logger.warning("InputHook: XINPUT9_1_0.dll is not loaded; controller polling will not be observed.");
        return false;
    }

    auto* target = reinterpret_cast<void*>(GetProcAddress(module, "XInputGetState"));
    if (!target) {
        logger.warning("InputHook: XInputGetState not found in XINPUT9_1_0.dll.");
        return false;
    }

    if (!_hookGetState.install(target, reinterpret_cast<void*>(&InputHook::hkXInputGetState),
                               "InputHook::XInputGetState")) {
        logger.error("InputHook: failed to hook XInputGetState.");
        return false;
    }

    logger.info("InputHook: watching XInputGetState @ 0x{:X}.", reinterpret_cast<uintptr_t>(target));
    return true;
}

// Removes the XInputGetState hook.
void InputHook::uninitialize()
{
    _hookGetState.remove();
}

// Detour for XInputGetState, recording pad polling, state capture, and button remapping.
uint32_t __stdcall InputHook::hkXInputGetState(uint32_t userIndex, void* state)
{
    InputHook& self = get();

    auto original = reinterpret_cast<t_XInputGetState>(self._hookGetState.getOriginal());
    if (!original) return 1;

    uint32_t result = original(userIndex, state);

    if (userIndex < kMaxSlots) {
        self._polled[userIndex].fetch_add(1, std::memory_order_relaxed);
        if (result == ERROR_SUCCESS) self._connected[userIndex].fetch_add(1, std::memory_order_relaxed);
    }
    self._pads.observe(userIndex, result, state);

    if (result == ERROR_SUCCESS && state && !self._pads.isCaptured()) {
        XINPUT_GAMEPAD& gamepad = static_cast<XINPUT_STATE*>(state)->Gamepad;
        gamepad.wButtons = InputRemap::get().remapPad(gamepad.wButtons);
    }
    return result;
}

// Returns the cached pad state for the given controller slot.
PadState InputHook::pad(uint32_t slot) const
{
    return _pads.pad(slot);
}

// Sets whether pad input is captured, blanking the state passed to the game.
void InputHook::setCaptured(bool captured)
{
    _pads.setCaptured(captured);
}

// Generates a diagnostic summary of polled and connected controller slots.
std::string InputHook::report() const
{
    std::string out;

    for (uint32_t slot = 0; slot < kMaxSlots; ++slot) {
        uint32_t polled = _polled[slot].load(std::memory_order_relaxed);
        uint32_t connected = _connected[slot].load(std::memory_order_relaxed);

        if (slot) out += " | ";
        out += std::format("slot{}=polled:{} connected:{}", slot, polled, connected);
    }
    return out;
}

} // namespace crabe::presentation

namespace {

// Reports whether a window of this process holds the foreground, so other applications' keystrokes are ignored.
bool gameHasFocus()
{
    const HWND foreground = GetForegroundWindow();
    if (!foreground)
        return false;

    DWORD processId = 0;
    GetWindowThreadProcessId(foreground, &processId);
    return processId == GetCurrentProcessId();
}

} // namespace

// Lua binding for Crabe._keyDown; checks if a virtual key is currently held while the game has focus.
int __cdecl crabe::input_natives::keyDown(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    auto key = static_cast<int>(lua.argToNumber(L, 1, 0.0));
    if (key <= 0 || key > 254) {
        lua.pushNumber(L, 0.0);
        return 1;
    }

    const bool held = gameHasFocus() && (GetAsyncKeyState(key) & 0x8000) != 0;
    lua.pushNumber(L, held ? 1.0 : 0.0);
    return 1;
}

// Lua binding for Crabe._padState; pushes gamepad connection, buttons, triggers, and axes.
int __cdecl crabe::input_natives::padState(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    auto slot = static_cast<uint32_t>(lua.argToNumber(L, 1, 0.0));
    crabe::presentation::PadState pad = crabe::presentation::InputHook::get().pad(slot);
    if (!gameHasFocus())
        pad = crabe::presentation::PadState{};

    lua.pushBoolean(L, pad.connected);
    lua.pushNumber(L, pad.buttons);
    lua.pushNumber(L, pad.leftTrigger);
    lua.pushNumber(L, pad.rightTrigger);
    lua.pushNumber(L, pad.leftX);
    lua.pushNumber(L, pad.leftY);
    lua.pushNumber(L, pad.rightX);
    lua.pushNumber(L, pad.rightY);
    return 8;
}

// Lua binding for Crabe._setPadCaptured; suppresses gamepad inputs passed to the game.
int __cdecl crabe::input_natives::setPadCaptured(void* L)
{
    bool captured = crabe::infrastructure::LuaCall::get().argToBoolean(L, 1, false);
    crabe::presentation::InputHook::get().setCaptured(captured);
    return 0;
}

// Lua binding for Crabe._setDirectInputCaptured; suppresses DirectInput keyboard and mouse passed to the game.
int __cdecl crabe::input_natives::setDirectInputCaptured(void* L)
{
    bool captured = crabe::infrastructure::LuaCall::get().argToBoolean(L, 1, false);
    crabe::presentation::DirectInputHook::get().setCaptured(captured);
    return 0;
}

