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

// Counts which XInput slots the game polls, and which answer "connected", and
// keeps the last state each returned. Always forwards to the real function;
// while captured, the game is handed an idle gamepad instead of the real one.
class InputHook {
    public:
        static InputHook& get();

        // Returns false if XINPUT9_1_0.dll is absent; the loader works without it.
        bool initialize();
        void uninitialize();

        // "slot0=polled:N connected:N | slot1=..." since the hook went in.
        std::string report() const;

        // Last state the game read from `slot`; disconnected until it polls it.
        PadState pad(uint32_t slot) const;

        // While true, the game sees an idle gamepad; mods still read the real one.
        void setCaptured(bool captured);

    private:
        InputHook() = default;
        ~InputHook() = default;
        InputHook(const InputHook&) = delete;
        InputHook& operator=(const InputHook&) = delete;

        typedef uint32_t(__stdcall* t_XInputGetState)(uint32_t userIndex, void* state);
        static uint32_t __stdcall hkXInputGetState(uint32_t userIndex, void* state);

        crabe::infrastructure::Hook _hookGetState;

        // Relaxed: written from whichever thread polls input, read from the
        // Lua thread. Only the counts matter, never their ordering.
        static constexpr uint32_t kMaxSlots = 4;
        std::atomic<uint32_t> _polled[kMaxSlots] = {};
        std::atomic<uint32_t> _connected[kMaxSlots] = {};

        PadStateStore _pads;
};

} // namespace crabe::presentation

// Lua C function for reading live key state. It lives here rather than in
// lua_natives.cpp only because that file is at the line cap.
//
// This reads the keyboard directly instead of going through the game's input
// path. A mod that has to steer something every frame -- a free camera -- needs
// key state at tick time, and the engine never hands that to Lua.
namespace crabe::input_natives {
    int __cdecl keyDown(void* L);
    int __cdecl padState(void* L);
    int __cdecl setPadCaptured(void* L);
}

#endif /* !INPUT_HOOK_HPP_ */
