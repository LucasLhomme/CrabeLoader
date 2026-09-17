/*
** CrabeLoader
** File description:
** input_hook
*/

#include <format>
#include <string>
#include <windows.h>

#include "presentation/input_hook.hpp"
#include "infrastructure/luacall.hpp"
#include "shared/logger.hpp"

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
    Logger& logger = Logger::getInstance();

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

    if (!_hookGetState.install(target, reinterpret_cast<void*>(&InputHook::hkXInputGetState))) {
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
        if (result == 0) self._connected[userIndex].fetch_add(1, std::memory_order_relaxed);
    }

    return result;
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

// Crabe._keyDown(virtualKey) -> 1 while the key is held, 0 otherwise.
//
// GetAsyncKeyState reports the physical state regardless of which window has
// focus, which is what a tick-driven mod needs: the overlay may own the
// keyboard while the mod still has to steer.
int __cdecl InputNatives::keyDown(void* L)
{
    LuaCall& lua = LuaCall::get();
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
