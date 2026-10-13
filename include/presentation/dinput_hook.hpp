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

// Hooks IDirectInputDevice8::GetDeviceState and GetDeviceData. The keyboard and the
// mouse do not share that code on every Windows build (a session logged the mouse
// using its own), so each distinct address gets its own hook; the device kind is
// still read per call, which makes every detour correct whichever device reaches it.
class DirectInputHook {
    public:
        static DirectInputHook& get();

        // Returns false if DirectInput is unavailable; the loader works without it.
        bool initialize();
        void uninitialize();

        // Whole wheel notches the game has read since the last call (positive up), for mods that need the wheel
        // at tick time: the engine hands it to the menus as an input action and a mod cannot read that.
        int takeWheelNotches();

        // "dinput kbdState:N kbdData:N mouseState:N mouseData:N otherState:N wheel:N" calls seen since the hook went in.
        // This is how a session shows which path the game actually reads its keyboard through.
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

        // The mouse's own code, hooked only when it is not the keyboard's.
        static long __stdcall hkMouseGetDeviceState(IDirectInputDevice8A* self, unsigned long size, void* data);
        static long __stdcall hkMouseGetDeviceData(IDirectInputDevice8A* self, unsigned long objectSize, void* events,
                                                   unsigned long* count, unsigned long flags);

        // What every detour does once the real call has returned.
        long afterState(unsigned long size, void* data, long result);
        long afterData(IDirectInputDevice8A* self, unsigned long objectSize, void* events, unsigned long* count, long result);

        crabe::infrastructure::Hook _hookState;
        crabe::infrastructure::Hook _hookData;
        crabe::infrastructure::Hook _hookMouseState;
        crabe::infrastructure::Hook _hookMouseData;

        // Relaxed: only the counts matter, never their ordering.
        std::atomic<uint32_t> _keyboardStates{ 0 };
        std::atomic<uint32_t> _keyboardEvents{ 0 };
        std::atomic<uint32_t> _mouseStates{ 0 };
        std::atomic<uint32_t> _mouseEvents{ 0 };
        std::atomic<uint32_t> _otherStates{ 0 };
        WheelCounter _wheel;
};

} // namespace crabe::presentation

#endif /* !DINPUT_HOOK_HPP_ */
