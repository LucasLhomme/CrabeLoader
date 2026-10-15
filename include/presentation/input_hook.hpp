/*
** CrabeLoader
** File description:
** Declares the XInput hook: which slots the game polls, and the last pad state each returned.
** Forwards every call; while a mod captures the pad, the game gets a neutral (idle) gamepad.
** Hooks XINPUT9_1_0.dll specifically, the module this image actually imports.
**
** Authors: @LucasLhomme
*/

#ifndef INPUT_HOOK_HPP_
#define INPUT_HOOK_HPP_

#include <atomic>
#include <cstdint>
#include <string>

#include "infrastructure/hook.hpp"
#include "presentation/pad_state.hpp"

namespace crabe::presentation {

class InputHook {
    public:
        // Returns the singleton instance of InputHook.
        static InputHook& get();

        // Initializes the XInput detour hook.
        bool initialize();

        // Uninstalls the XInput detour hook.
        void uninitialize();

        // Generates a diagnostic summary of polled and connected input slots.
        std::string report() const;

        // Returns the last state recorded for the given gamepad slot.
        PadState pad(uint32_t slot) const;

        // While captured, the game receives an idle gamepad state while mods read real inputs.
        void setCaptured(bool captured);

    private:
        InputHook() = default;
        ~InputHook() = default;
        InputHook(const InputHook&) = delete;
        InputHook& operator=(const InputHook&) = delete;

        typedef uint32_t(__stdcall* t_XInputGetState)(uint32_t userIndex, void* state);
        static uint32_t __stdcall hkXInputGetState(uint32_t userIndex, void* state);

        crabe::infrastructure::Hook _hookGetState;

        static constexpr uint32_t kMaxSlots = 4;
        std::atomic<uint32_t> _polled[kMaxSlots] = {};
        std::atomic<uint32_t> _connected[kMaxSlots] = {};

        PadStateStore _pads;
};

} // namespace crabe::presentation

namespace crabe::input_natives {
    // Lua binding to query asynchronous keyboard key state.
    int __cdecl keyDown(void* L);

    // Lua binding to query gamepad state for a slot.
    int __cdecl padState(void* L);

    // Lua binding to capture or release gamepad input from the game.
    int __cdecl setPadCaptured(void* L);

    // Lua binding to capture or release DirectInput keyboard and mouse from the game.
    int __cdecl setDirectInputCaptured(void* L);
}

#endif /* !INPUT_HOOK_HPP_ */
