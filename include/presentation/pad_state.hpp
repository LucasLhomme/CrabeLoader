/*
** CrabeLoader
** File description:
** Declares the store of the last gamepad state each XInput slot returned to the game.
** Lock-free: written by whichever thread polls XInput, read by the Lua thread at tick time.
** Hooks nothing and knows no Lua; InputHook feeds it and lua natives read it.
**
** Authors: @LucasLhomme
*/

#ifndef PAD_STATE_HPP_
#define PAD_STATE_HPP_

#include <atomic>
#include <cstdint>

namespace crabe::presentation {

// Last gamepad state the game read for one slot, as XInput returned it.
struct PadState {
    bool connected = false;
    uint16_t buttons = 0;
    uint8_t leftTrigger = 0;
    uint8_t rightTrigger = 0;
    int16_t leftX = 0;
    int16_t leftY = 0;
    int16_t rightX = 0;
    int16_t rightY = 0;
};

// Keeps what XInputGetState answered per slot and, while captured, hands the
// caller an idle gamepad instead of the real one.
class PadStateStore final {
    public:
        static constexpr uint32_t kMaxSlots = 4;

        // Records the answer for `slot` (`state` is an XINPUT_STATE*). A failed
        // call marks the slot disconnected; while captured, the gamepad is blanked.
        void observe(uint32_t slot, uint32_t result, void* state);

        // Last state recorded for `slot`; disconnected until one is.
        PadState pad(uint32_t slot) const;

        void setCaptured(bool captured);
        bool isCaptured() const;

    private:
        // Packed so one slot is published by three independent stores:
        // buttons | lt << 16 | rt << 24, then x | y << 16 for each stick.
        std::atomic<uint32_t> _buttons[kMaxSlots] = {};
        std::atomic<uint32_t> _leftStick[kMaxSlots] = {};
        std::atomic<uint32_t> _rightStick[kMaxSlots] = {};
        std::atomic<bool> _live[kMaxSlots] = {};
        std::atomic<bool> _captured{ false };
};

} // namespace crabe::presentation

#endif /* !PAD_STATE_HPP_ */
