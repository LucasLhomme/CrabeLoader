/*
** CrabeLoader
** File description:
** Declares the DirectInput hook: remaps keyboard and mouse input data read by the game.
** Hooks IDirectInput8A devices created by the game image to intercept device state buffers.
** Forwards every call; only buffers returned to the game are rewritten, never hardware devices.
**
** Authors: @LucasLhomme
*/

#ifndef DINPUT_HOOK_HPP_
#define DINPUT_HOOK_HPP_

#include <atomic>
#include <cstdint>
#include <string>

#include "infrastructure/hook.hpp"
#include "presentation/wheel_counter.hpp"

struct IDirectInputDevice8A;

namespace crabe::presentation {

class DirectInputHook {
    public:
        // Returns the singleton instance of DirectInputHook.
        static DirectInputHook& get();

        // Initializes DirectInput hooks on available devices.
        bool initialize();

        // Removes all installed DirectInput hooks.
        void uninitialize();

        // Returns accumulated mouse wheel notches since the previous call.
        int takeWheelNotches();

        // Generates a diagnostic summary of intercepted DirectInput calls.
        std::string report() const;

    private:
        DirectInputHook() = default;
        ~DirectInputHook() = default;
        DirectInputHook(const DirectInputHook&) = delete;
        DirectInputHook& operator=(const DirectInputHook&) = delete;

        typedef long(__stdcall* t_GetDeviceState)(IDirectInputDevice8A* self, unsigned long size, void* data);
        typedef long(__stdcall* t_GetDeviceData)(IDirectInputDevice8A* self, unsigned long objectSize, void* events,
                                                 unsigned long* count, unsigned long flags);

        static long __stdcall hkGetDeviceState(IDirectInputDevice8A* self, unsigned long size, void* data);
        static long __stdcall hkGetDeviceData(IDirectInputDevice8A* self, unsigned long objectSize, void* events,
                                              unsigned long* count, unsigned long flags);

        // Intercepts mouse device state queries when distinct from keyboard.
        static long __stdcall hkMouseGetDeviceState(IDirectInputDevice8A* self, unsigned long size, void* data);
        static long __stdcall hkMouseGetDeviceData(IDirectInputDevice8A* self, unsigned long objectSize, void* events,
                                                   unsigned long* count, unsigned long flags);

        // Processes device state buffer after the real call completes.
        long afterState(unsigned long size, void* data, long result);
        long afterData(IDirectInputDevice8A* self, unsigned long objectSize, void* events, unsigned long* count, long result);

        crabe::infrastructure::Hook _hookState;
        crabe::infrastructure::Hook _hookData;
        crabe::infrastructure::Hook _hookMouseState;
        crabe::infrastructure::Hook _hookMouseData;

        std::atomic<uint32_t> _keyboardStates{ 0 };
        std::atomic<uint32_t> _keyboardEvents{ 0 };
        std::atomic<uint32_t> _mouseStates{ 0 };
        std::atomic<uint32_t> _mouseEvents{ 0 };
        std::atomic<uint32_t> _otherStates{ 0 };
        WheelCounter _wheel;
};

} // namespace crabe::presentation

#endif /* !DINPUT_HOOK_HPP_ */
