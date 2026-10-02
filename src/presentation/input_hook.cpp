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

#include "presentation/input_hook.hpp"
#include "infrastructure/lua_call.hpp"
#include "shared/logger.hpp"

namespace crabe::presentation {

InputHook& InputHook::get()
{
    static InputHook instance;
    return instance;
}

// The game imports from XINPUT9_1_0.dll specifically (confirmed: the other
// xinput DLL names do not appear in the image), so hook the module it
// actually uses rather than whichever one happens to be loaded.
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

void InputHook::uninitialize()
{
    _hookGetState.remove();
}

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
    return result;
}

PadState InputHook::pad(uint32_t slot) const
{
    return _pads.pad(slot);
}

void InputHook::setCaptured(bool captured)
{
    _pads.setCaptured(captured);
}

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

// Crabe._keyDown(virtualKey) -> 1 while the key is held, 0 otherwise.
//
// GetAsyncKeyState reports the physical state regardless of which window has
// focus, which is what a tick-driven mod needs: the overlay may own the
// keyboard while the mod still has to steer.
int __cdecl crabe::input_natives::keyDown(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    auto key = static_cast<int>(lua.argToNumber(L, 1, 0.0));
    if (key <= 0 || key > 254) {
        lua.pushNumber(L, 0.0);
        return 1;
    }

    const bool held = (GetAsyncKeyState(key) & 0x8000) != 0;
    lua.pushNumber(L, held ? 1.0 : 0.0);
    return 1;
}

// Crabe._padState([slot]) -> connected, buttons, leftTrigger, rightTrigger,
// leftX, leftY, rightX, rightY: what the game last read from that slot (0-3).
// buttons is the XINPUT_GAMEPAD_* bitmask; it reads the real pad while captured.
int __cdecl crabe::input_natives::padState(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    auto slot = static_cast<uint32_t>(lua.argToNumber(L, 1, 0.0));
    crabe::presentation::PadState pad = crabe::presentation::InputHook::get().pad(slot);

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

// Crabe._setPadCaptured(on): while on, the game reads an idle gamepad.
int __cdecl crabe::input_natives::setPadCaptured(void* L)
{
    bool captured = crabe::infrastructure::LuaCall::get().argToBoolean(L, 1, false);
    crabe::presentation::InputHook::get().setCaptured(captured);
    return 0;
}
