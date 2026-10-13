/*
** CrabeLoader
** File description:
** Implements the input remap table: three small atomic arrays and the arithmetic that applies them.
** An entry holds target + 1 so that "swallow" (target 0) and "no entry" stay distinct.
** A remap never invents input: every output bit comes from a physical bit that was actually held.
**
** Authors: @LucasLhomme
*/

#include "presentation/input_remap.hpp"

namespace crabe::presentation {

namespace {

    constexpr uint8_t kDown = 0x80;

    // Index of the single set bit of `mask`, or -1 when `mask` is zero or has several bits.
    int singleBit(uint32_t mask)
    {
        if (mask == 0 || (mask & (mask - 1)) != 0) return -1;
        int index = 0;
        while ((mask & 1u) == 0) {
            mask >>= 1;
            ++index;
        }
        return index;
    }

}

InputRemap& InputRemap::get()
{
    static InputRemap instance;
    return instance;
}

bool InputRemap::store(std::atomic<uint16_t>& slot, uint16_t value, std::atomic<uint32_t>& entries)
{
    uint16_t previous = slot.exchange(value, std::memory_order_relaxed);
    if (previous == 0 && value != 0) entries.fetch_add(1, std::memory_order_relaxed);
    if (previous != 0 && value == 0) entries.fetch_sub(1, std::memory_order_relaxed);
    return true;
}

void InputRemap::wipe(std::atomic<uint16_t>* slots, size_t count, std::atomic<uint32_t>& entries)
{
    for (size_t i = 0; i < count; ++i) store(slots[i], 0, entries);
}

bool InputRemap::setKey(uint32_t from, int to)
{
    if (from == 0 || from >= kKeys || to >= static_cast<int>(kKeys)) return false;
    // Scan code 0 is not a key, so mapping to it, like a negative target, swallows the input.
    if (to <= 0) return store(_keys[from], 1, _entries);
    // A key mapped to itself is the same as no entry, and keeps the table minimal.
    return store(_keys[from], static_cast<uint32_t>(to) == from ? 0 : static_cast<uint16_t>(to + 2), _entries);
}

bool InputRemap::setMouseButton(uint32_t from, int to)
{
    if (from >= kMouseButtons || to >= static_cast<int>(kMouseButtons)) return false;
    if (to < 0) return store(_mouse[from], 1, _entries);
    return store(_mouse[from], static_cast<uint32_t>(to) == from ? 0 : static_cast<uint16_t>(to + 2), _entries);
}

bool InputRemap::setPadButton(uint32_t fromMask, uint32_t toMask)
{
    int from = singleBit(fromMask);
    if (from < 0 || from >= static_cast<int>(kPadBits)) return false;

    uint16_t value = 0;
    if (toMask == 0) {
        value = 1;
    } else {
        int to = singleBit(toMask);
        if (to < 0 || to >= static_cast<int>(kPadBits)) return false;
        value = (to == from) ? 0 : static_cast<uint16_t>(to + 2);
    }
    return store(_pad[from], value, _entries);
}

void InputRemap::clearKeys() { wipe(_keys, kKeys, _entries); }
void InputRemap::clearMouseButtons() { wipe(_mouse, kMouseButtons, _entries); }
void InputRemap::clearPadButtons() { wipe(_pad, kPadBits, _entries); }

void InputRemap::clearAll()
{
    clearKeys();
    clearMouseButtons();
    clearPadButtons();
}

bool InputRemap::isActive() const
{
    return _entries.load(std::memory_order_relaxed) != 0;
}

int InputRemap::keyTarget(uint32_t from) const
{
    if (from >= kKeys) return static_cast<int>(from);
    uint16_t entry = _keys[from].load(std::memory_order_relaxed);
    if (entry == 0) return static_cast<int>(from);
    return entry == 1 ? kDropped : static_cast<int>(entry - 2);
}

int InputRemap::mouseTarget(uint32_t from) const
{
    if (from >= kMouseButtons) return static_cast<int>(from);
    uint16_t entry = _mouse[from].load(std::memory_order_relaxed);
    if (entry == 0) return static_cast<int>(from);
    return entry == 1 ? kDropped : static_cast<int>(entry - 2);
}

void InputRemap::remapKeyboardState(uint8_t* state, size_t size) const
{
    if (!state || size < kKeys || !isActive()) return;

    uint8_t held[kKeys];
    for (size_t i = 0; i < kKeys; ++i) held[i] = state[i];
    for (size_t i = 0; i < kKeys; ++i) state[i] = 0;

    for (size_t from = 0; from < kKeys; ++from) {
        if ((held[from] & kDown) == 0) continue;
        int target = keyTarget(static_cast<uint32_t>(from));
        if (target != kDropped) state[target] = kDown;
    }
}

void InputRemap::remapMouseButtons(uint8_t* buttons, size_t count) const
{
    if (!buttons || count == 0 || !isActive()) return;
    if (count > kMouseButtons) count = kMouseButtons;

    uint8_t held[kMouseButtons];
    for (size_t i = 0; i < count; ++i) {
        held[i] = buttons[i];
        buttons[i] = 0;
    }

    for (size_t from = 0; from < count; ++from) {
        if ((held[from] & kDown) == 0) continue;
        int target = mouseTarget(static_cast<uint32_t>(from));
        if (target != kDropped && static_cast<size_t>(target) < count) buttons[target] = kDown;
    }
}

uint16_t InputRemap::remapPad(uint16_t buttons) const
{
    if (buttons == 0 || !isActive()) return buttons;

    uint16_t out = 0;
    for (size_t bit = 0; bit < kPadBits; ++bit) {
        if ((buttons & (1u << bit)) == 0) continue;
        uint16_t entry = _pad[bit].load(std::memory_order_relaxed);
        if (entry == 0) out = static_cast<uint16_t>(out | (1u << bit));
        else if (entry >= 2) out = static_cast<uint16_t>(out | (1u << (entry - 2)));
    }
    return out;
}

} // namespace crabe::presentation
