/*
** CrabeLoader
** File description:
** Packs each XInput answer into three atomics per slot and unpacks it for the Lua natives.
** Sticks are signed 16-bit and go through uint16_t so a negative value survives the packing.
** Capturing only blanks the caller's XINPUT_GAMEPAD; the result code and packet number stay.
**
** Authors: @LucasLhomme
*/

#include "presentation/pad_state.hpp"

#include <cstring>
#include <windows.h>
#include <Xinput.h>

namespace crabe::presentation {

namespace {

    uint32_t packStick(int16_t x, int16_t y)
    {
        return static_cast<uint16_t>(x) | (static_cast<uint32_t>(static_cast<uint16_t>(y)) << 16);
    }

}

void PadStateStore::observe(uint32_t slot, uint32_t result, void* state)
{
    if (slot >= kMaxSlots) return;
    if (result != ERROR_SUCCESS || !state) {
        _live[slot].store(false, std::memory_order_relaxed);
        return;
    }

    auto* pad = &static_cast<XINPUT_STATE*>(state)->Gamepad;
    _buttons[slot].store(pad->wButtons | (static_cast<uint32_t>(pad->bLeftTrigger) << 16)
                         | (static_cast<uint32_t>(pad->bRightTrigger) << 24), std::memory_order_relaxed);
    _leftStick[slot].store(packStick(pad->sThumbLX, pad->sThumbLY), std::memory_order_relaxed);
    _rightStick[slot].store(packStick(pad->sThumbRX, pad->sThumbRY), std::memory_order_relaxed);
    _live[slot].store(true, std::memory_order_relaxed);

    if (_captured.load(std::memory_order_relaxed))
        std::memset(pad, 0, sizeof(*pad));
}

PadState PadStateStore::pad(uint32_t slot) const
{
    PadState out;
    if (slot >= kMaxSlots || !_live[slot].load(std::memory_order_relaxed)) return out;

    uint32_t buttons = _buttons[slot].load(std::memory_order_relaxed);
    uint32_t left = _leftStick[slot].load(std::memory_order_relaxed);
    uint32_t right = _rightStick[slot].load(std::memory_order_relaxed);

    out.connected = true;
    out.buttons = static_cast<uint16_t>(buttons & 0xFFFF);
    out.leftTrigger = static_cast<uint8_t>((buttons >> 16) & 0xFF);
    out.rightTrigger = static_cast<uint8_t>(buttons >> 24);
    out.leftX = static_cast<int16_t>(left & 0xFFFF);
    out.leftY = static_cast<int16_t>(left >> 16);
    out.rightX = static_cast<int16_t>(right & 0xFFFF);
    out.rightY = static_cast<int16_t>(right >> 16);
    return out;
}

void PadStateStore::setCaptured(bool captured)
{
    _captured.store(captured, std::memory_order_relaxed);
}

bool PadStateStore::isCaptured() const
{
    return _captured.load(std::memory_order_relaxed);
}

} // namespace crabe::presentation
