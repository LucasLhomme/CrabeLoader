/*
** CrabeLoader
** File description:
** input_hook
*/

#include <format>
#include <string>
#include <windows.h>

#include "loader/input_hook.hpp"
#include "logger/logger.hpp"

InputHook& InputHook::get()
{
    static InputHook instance;
    return instance;
}

bool InputHook::initialize()
{
    Logger& logger = Logger::getInstance();

    // The game imports from XINPUT9_1_0.dll specifically (confirmed: the other
    // xinput DLL names do not appear in the image), so hook the module it
    // actually uses rather than whichever one happens to be loaded.
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
    if (!original) return 1; // ERROR_DEVICE_NOT_CONNECTED

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
