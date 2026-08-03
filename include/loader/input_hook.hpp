/*
** CrabeLoader
** File description:
** input_hook
*/

#ifndef INPUT_HOOK_HPP_
#define INPUT_HOOK_HPP_

#include <atomic>
#include <cstdint>
#include <string>

#include "loader/hook.hpp"

// Counts which XInput slots the game polls, and which answer "connected".
// Observer only: always forwards to the real function, never fakes a result.
class InputHook {
    public:
        static InputHook& get();

        // Returns false if XINPUT9_1_0.dll is absent; the loader works without it.
        bool initialize();
        void uninitialize();

        // "slot0=polled:N connected:N | slot1=..." since the hook went in.
        std::string report() const;

    private:
        InputHook() = default;
        ~InputHook() = default;
        InputHook(const InputHook&) = delete;
        InputHook& operator=(const InputHook&) = delete;

        typedef uint32_t(__stdcall* t_XInputGetState)(uint32_t userIndex, void* state);
        static uint32_t __stdcall hkXInputGetState(uint32_t userIndex, void* state);

        Hook _hookGetState;

        // Relaxed: written from whichever thread polls input, read from the
        // Lua thread. Only the counts matter, never their ordering.
        static constexpr uint32_t kMaxSlots = 4;
        std::atomic<uint32_t> _polled[kMaxSlots] = {};
        std::atomic<uint32_t> _connected[kMaxSlots] = {};
};

#endif /* !INPUT_HOOK_HPP_ */
