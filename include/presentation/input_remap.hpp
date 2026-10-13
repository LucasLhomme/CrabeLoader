/*
** CrabeLoader
** File description:
** Declares the input remap table: swaps physical keys, mouse buttons, or pad buttons for the game.
** Lock-free: written from the script thread, read concurrently by polling input hook threads.
** Pure data and arithmetic: hooks nothing and knows no Win32; feeds the hooks and Lua natives.
**
** Authors: @LucasLhomme
*/

#ifndef INPUT_REMAP_HPP_
#define INPUT_REMAP_HPP_

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace crabe::presentation {

// A remap says "when this physical input is pressed, the game sees that one".
// Every entry is keyed by the physical input and holds the input the game gets;
// an entry whose target is 0 swallows the input instead. Inputs without an entry
// pass through untouched. Keyboard inputs are DirectInput scan codes (DIK_*),
// mouse buttons are 0-based button indices, pad buttons are XINPUT_GAMEPAD_* bits.
class InputRemap final {
    public:
        static constexpr size_t kKeys = 256;
        static constexpr size_t kMouseButtons = 8;
        static constexpr size_t kPadBits = 16;

        // Returned by the *Target lookups for an input that is swallowed.
        static constexpr int kDropped = -1;

        static InputRemap& get();

        // Each setter returns false, and changes nothing, when an index or mask is out of range.
        // A negative `to` swallows the input; so does a key `to` of 0 (not a scan code) and a pad `toMask` of 0.
        bool setKey(uint32_t from, int to);
        bool setMouseButton(uint32_t from, int to);
        bool setPadButton(uint32_t fromMask, uint32_t toMask);

        void clearKeys();
        void clearMouseButtons();
        void clearPadButtons();
        void clearAll();

        // True when at least one entry exists; the hooks skip all work while false.
        bool isActive() const;

        // What the game sees for a physical input: the input itself when unmapped, kDropped when swallowed.
        int keyTarget(uint32_t from) const;
        int mouseTarget(uint32_t from) const;

        // Rewrites a DirectInput keyboard state (256 bytes, high bit = down) in place.
        void remapKeyboardState(uint8_t* state, size_t size) const;

        // Rewrites DirectInput mouse button bytes (high bit = down) in place.
        void remapMouseButtons(uint8_t* buttons, size_t count) const;

        // Returns the XINPUT_GAMEPAD_* button mask the game should see.
        uint16_t remapPad(uint16_t buttons) const;

    private:
        InputRemap() = default;
        ~InputRemap() = default;
        InputRemap(const InputRemap&) = delete;
        InputRemap& operator=(const InputRemap&) = delete;

        // 0 = no entry, 1 = swallowed, otherwise target + 2.
        std::atomic<uint16_t> _keys[kKeys] = {};
        std::atomic<uint16_t> _mouse[kMouseButtons] = {};
        std::atomic<uint16_t> _pad[kPadBits] = {};

        // Entries set across the three tables; only read as a cheap "anything to do" test.
        std::atomic<uint32_t> _entries{ 0 };

        static bool store(std::atomic<uint16_t>& slot, uint16_t value, std::atomic<uint32_t>& entries);
        static void wipe(std::atomic<uint16_t>* slots, size_t count, std::atomic<uint32_t>& entries);
};

} // namespace crabe::presentation

// Lua C functions that edit the table. They live next to it rather than in
// lua_natives.cpp, which is at the line cap.
namespace crabe::input_natives {
    // Crabe._inputRemap(device, from, to) -> boolean. device: 0 keyboard, 1 mouse button, 2 pad button.
    int __cdecl remapSet(void* L);
    // Crabe._inputRemapClear([device]) clears one device, or everything when omitted.
    int __cdecl remapClear(void* L);
    // Crabe._keyScanCode(virtualKey) -> the DirectInput scan code of a Windows virtual key, 0 when it has none.
    int __cdecl keyScanCode(void* L);
    // Crabe._mouseWheel() -> whole wheel notches the game read since the last call (positive up), 0 when none.
    int __cdecl mouseWheel(void* L);
    // Crabe._keyName(scanCode) -> the key's name on the player's own keyboard layout ("Z" on AZERTY).
    int __cdecl keyName(void* L);
}

#endif /* !INPUT_REMAP_HPP_ */
