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

class InputRemap final {
    public:
        static constexpr size_t kKeys = 256;
        static constexpr size_t kMouseButtons = 8;
        static constexpr size_t kPadBits = 16;

        static constexpr int kDropped = -1;

        // Returns the singleton instance of InputRemap.
        static InputRemap& get();

        // Configures a keyboard key remap target, or swallows the key if target is non-positive.
        bool setKey(uint32_t from, int to);

        // Configures a mouse button remap target, or swallows the button if target is negative.
        bool setMouseButton(uint32_t from, int to);

        // Configures a gamepad button mask remap target.
        bool setPadButton(uint32_t fromMask, uint32_t toMask);

        // Clears all configured keyboard remaps.
        void clearKeys();

        // Clears all configured mouse button remaps.
        void clearMouseButtons();

        // Clears all configured gamepad button remaps.
        void clearPadButtons();

        // Clears all active input remap entries.
        void clearAll();

        // Returns true if at least one remap entry is currently configured.
        bool isActive() const;

        // Returns the target scan code for a physical key, or kDropped if swallowed.
        int keyTarget(uint32_t from) const;

        // Returns the target button index for a physical mouse button, or kDropped if swallowed.
        int mouseTarget(uint32_t from) const;

        // Rewrites a DirectInput keyboard state in place according to active remaps.
        void remapKeyboardState(uint8_t* state, size_t size) const;

        // Rewrites DirectInput mouse button states in place according to active remaps.
        void remapMouseButtons(uint8_t* buttons, size_t count) const;

        // Returns the remapped XInput button bitmask for the game.
        uint16_t remapPad(uint16_t buttons) const;

    private:
        InputRemap() = default;
        ~InputRemap() = default;
        InputRemap(const InputRemap&) = delete;
        InputRemap& operator=(const InputRemap&) = delete;

        std::atomic<uint16_t> _keys[kKeys] = {};
        std::atomic<uint16_t> _mouse[kMouseButtons] = {};
        std::atomic<uint16_t> _pad[kPadBits] = {};

        std::atomic<uint32_t> _entries{ 0 };

        static bool store(std::atomic<uint16_t>& slot, uint16_t value, std::atomic<uint32_t>& entries);
        static void wipe(std::atomic<uint16_t>* slots, size_t count, std::atomic<uint32_t>& entries);
};

} // namespace crabe::presentation

namespace crabe::input_natives {
    // Lua binding to configure an input remap mapping.
    int __cdecl remapSet(void* L);

    // Lua binding to clear input remaps for a device or all devices.
    int __cdecl remapClear(void* L);

    // Lua binding to report what is currently taken from the game: DirectInput, remaps and captured keys.
    int __cdecl inputStatus(void* L);

    // Lua binding to convert a Windows virtual-key code into a DirectInput scan code.
    int __cdecl keyScanCode(void* L);

    // Lua binding to read accumulated mouse wheel notches since previous query.
    int __cdecl mouseWheel(void* L);

    // Lua binding to query the localized display name for a scan code.
    int __cdecl keyName(void* L);
}

#endif /* !INPUT_REMAP_HPP_ */
