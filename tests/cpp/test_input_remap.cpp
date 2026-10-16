/*
** CrabeLoader
** File description:
** Unit tests for the input remap table: keyboard, mouse and pad remaps, swallowing, and pass-through.
** Runs offline as an independent C++23 test binary: no game process, no DirectInput, no Lua VM.
**
** Authors: @LucasLhomme
*/

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include "presentation/input_remap.hpp"
#include "presentation/wheel_counter.hpp"

namespace {

using crabe::presentation::InputRemap;
using crabe::presentation::WheelCounter;

constexpr uint32_t kW = 0x11;
constexpr uint32_t kA = 0x1E;
constexpr uint32_t kS = 0x1F;
constexpr uint32_t kD = 0x20;
constexpr uint32_t kZ = 0x2C;
constexpr uint32_t kSpace = 0x39;

constexpr uint16_t kPadUp = 0x0001;
constexpr uint16_t kPadA = 0x1000;
constexpr uint16_t kPadB = 0x2000;
constexpr uint16_t kPadX = 0x4000;

int g_checks = 0;

void require(bool condition, const std::string& message)
{
    ++g_checks;
    if (!condition) {
        std::cerr << "[FAILED] " << message << std::endl;
        std::exit(1);
    }
}

bool isDown(const uint8_t* state, uint32_t key)
{
    return (state[key] & 0x80) != 0;
}

void testEmptyTableChangesNothing()
{
    InputRemap& remap = InputRemap::get();
    remap.clearAll();

    uint8_t state[256] = {};
    state[kW] = 0x80;
    remap.remapKeyboardState(state, sizeof(state));
    require(isDown(state, kW), "an empty table leaves a held key held");
    require(!remap.isActive(), "an empty table is inactive");
    require(remap.remapPad(kPadA) == kPadA, "an empty table leaves pad buttons alone");
}

void testKeyIsRemapped()
{
    InputRemap& remap = InputRemap::get();
    remap.clearAll();
    require(remap.setKey(kZ, kW), "Z can be remapped to W");
    require(remap.isActive(), "a table with one entry is active");

    uint8_t state[256] = {};
    state[kZ] = 0x80;
    state[kA] = 0x80;
    remap.remapKeyboardState(state, sizeof(state));
    require(isDown(state, kW), "Z held becomes W held");
    require(!isDown(state, kZ), "the remapped key itself is no longer seen");
    require(isDown(state, kA), "an unmapped key passes through");
}

void testSwapNeedsNoTemporary()
{
    InputRemap& remap = InputRemap::get();
    remap.clearAll();
    require(remap.setKey(kW, kS) && remap.setKey(kS, kW), "W and S can be swapped");

    uint8_t state[256] = {};
    state[kW] = 0x80;
    remap.remapKeyboardState(state, sizeof(state));
    require(isDown(state, kS) && !isDown(state, kW), "W held becomes S, not W again");

    uint8_t both[256] = {};
    both[kW] = 0x80;
    both[kS] = 0x80;
    remap.remapKeyboardState(both, sizeof(both));
    require(isDown(both, kS) && isDown(both, kW), "both held stay both held after a swap");
}

void testSwallowedKey()
{
    InputRemap& remap = InputRemap::get();
    remap.clearAll();
    require(remap.setKey(kSpace, 0), "a key can be swallowed with target 0");
    require(remap.keyTarget(kSpace) == InputRemap::kDropped, "a swallowed key has no target");

    uint8_t state[256] = {};
    state[kSpace] = 0x80;
    state[kD] = 0x80;
    remap.remapKeyboardState(state, sizeof(state));
    require(!isDown(state, kSpace), "a swallowed key is gone");
    require(isDown(state, kD), "its neighbours stay");
}

void testTwoKeysOneTargetMerge()
{
    InputRemap& remap = InputRemap::get();
    remap.clearAll();
    remap.setKey(kZ, kW);

    uint8_t state[256] = {};
    state[kW] = 0x80;
    remap.remapKeyboardState(state, sizeof(state));
    require(isDown(state, kW), "an untouched W still reads as W while Z is also mapped to it");

    uint8_t onlyZ[256] = {};
    onlyZ[kZ] = 0x80;
    remap.remapKeyboardState(onlyZ, sizeof(onlyZ));
    require(isDown(onlyZ, kW), "Z reads as W");
}

void testIdentityEntryIsNoEntry()
{
    InputRemap& remap = InputRemap::get();
    remap.clearAll();
    require(remap.setKey(kA, kA), "mapping a key to itself is accepted");
    require(!remap.isActive(), "and leaves the table inactive");
    require(remap.setKey(kA, kD), "a real remap goes in");
    require(remap.isActive(), "and makes the table active");
    require(remap.setKey(kA, kA), "mapping it back to itself works");
    require(!remap.isActive(), "which removes the entry");
}

void testBadArgumentsChangeNothing()
{
    InputRemap& remap = InputRemap::get();
    remap.clearAll();
    require(!remap.setKey(0, kW), "scan code 0 is not a key");
    require(!remap.setKey(256, kW), "scan code 256 is out of range");
    require(!remap.setKey(kW, 256), "a target of 256 is out of range");
    require(!remap.setMouseButton(8, 0), "mouse button 8 is out of range");
    require(!remap.setMouseButton(0, 8), "mouse target 8 is out of range");
    require(!remap.setPadButton(0x0003, kPadA), "a pad source with two bits is refused");
    require(!remap.setPadButton(kPadA, 0x3000), "a pad target with two bits is refused");
    require(!remap.setPadButton(0, kPadA), "a pad source of zero is refused");
    require(!remap.isActive(), "nothing was stored");
}

void testMouseButtons()
{
    InputRemap& remap = InputRemap::get();
    remap.clearAll();
    require(remap.setMouseButton(0, 1), "left can be remapped to right");
    require(remap.setMouseButton(1, -1), "right can be swallowed");

    uint8_t onlyRight[8] = { 0, 0x80, 0, 0, 0, 0, 0, 0 };
    remap.remapMouseButtons(onlyRight, 8);
    require((onlyRight[1] & 0x80) == 0, "right held alone is swallowed");

    uint8_t both[8] = { 0x80, 0x80, 0, 0, 0, 0, 0, 0 };
    remap.remapMouseButtons(both, 8);
    require((both[1] & 0x80) != 0, "left held still lands on right while the physical right is swallowed");
    require((both[0] & 0x80) == 0, "and left itself is no longer seen");
}

void testMouseRemapOrderIsPhysical()
{
    InputRemap& remap = InputRemap::get();
    remap.clearAll();
    remap.setMouseButton(0, 1);

    uint8_t buttons[8] = { 0x80, 0, 0x80, 0, 0, 0, 0, 0 };
    remap.remapMouseButtons(buttons, 8);
    require((buttons[1] & 0x80) != 0, "left held becomes right held");
    require((buttons[0] & 0x80) == 0, "left is no longer seen");
    require((buttons[2] & 0x80) != 0, "middle passes through");

    uint8_t shortState[4] = { 0x80, 0, 0, 0 };
    remap.remapMouseButtons(shortState, 4);
    require((shortState[1] & 0x80) != 0, "a four-button state is remapped within its size");
}

void testMouseTargetOutsideTheStateIsLost()
{
    InputRemap& remap = InputRemap::get();
    remap.clearAll();
    remap.setMouseButton(0, 6);

    uint8_t shortState[4] = { 0x80, 0, 0, 0 };
    remap.remapMouseButtons(shortState, 4);
    require(shortState[0] == 0 && shortState[1] == 0 && shortState[2] == 0 && shortState[3] == 0,
            "a target the state cannot hold is dropped instead of written out of bounds");
}

void testPadButtons()
{
    InputRemap& remap = InputRemap::get();
    remap.clearAll();
    require(remap.setPadButton(kPadA, kPadB), "A can be remapped to B");
    require(remap.setPadButton(kPadB, kPadA), "B can be remapped to A");
    require(remap.setPadButton(kPadX, 0), "X can be swallowed");

    require(remap.remapPad(kPadA) == kPadB, "A held becomes B held");
    require(remap.remapPad(kPadB) == kPadA, "B held becomes A held");
    require(remap.remapPad(kPadA | kPadB) == (kPadA | kPadB), "both held stay both held");
    require(remap.remapPad(kPadX) == 0, "a swallowed button vanishes");
    require(remap.remapPad(kPadUp | kPadX) == kPadUp, "other buttons pass through a swallow");
    require(remap.remapPad(0) == 0, "no buttons stay no buttons");
}

void testClearByDevice()
{
    InputRemap& remap = InputRemap::get();
    remap.clearAll();
    remap.setKey(kZ, kW);
    remap.setMouseButton(0, 1);
    remap.setPadButton(kPadA, kPadB);

    remap.clearKeys();
    require(remap.keyTarget(kZ) == static_cast<int>(kZ), "clearing keys frees the key");
    require(remap.mouseTarget(0) == 1 && remap.remapPad(kPadA) == kPadB, "and leaves the others");
    require(remap.isActive(), "still active while another device has entries");

    remap.clearMouseButtons();
    remap.clearPadButtons();
    require(!remap.isActive(), "inactive once every device is cleared");

    remap.setKey(kZ, kW);
    remap.clearAll();
    require(!remap.isActive(), "clearAll empties everything");
}


void testWheelCountsWholeNotches()
{
    WheelCounter wheel;
    require(wheel.takeNotches() == 0, "a new counter has no notches");

    wheel.add(120);
    require(wheel.takeNotches() == 1, "one notch up is one notch");
    require(wheel.takeNotches() == 0, "and it is handed out once");

    wheel.add(-120);
    wheel.add(-120);
    wheel.add(-120);
    require(wheel.takeNotches() == -3, "three notches down add up, with their sign");

    wheel.add(240);
    wheel.add(-120);
    require(wheel.takeNotches() == 1, "up and down cancel");
}

void testWheelKeepsFractions()
{
    WheelCounter wheel;
    wheel.add(40);
    wheel.add(40);
    require(wheel.takeNotches() == 0, "a high-resolution wheel's fractions are not a notch yet");
    wheel.add(40);
    require(wheel.takeNotches() == 1, "until they add up to one");

    wheel.add(130);
    require(wheel.takeNotches() == 1, "an overshoot gives its notch");
    wheel.add(110);
    require(wheel.takeNotches() == 1, "and the remainder waits for the rest of the next one");
    require(wheel.takeNotches() == 0, "nothing is invented");

    wheel.add(-130);
    require(wheel.takeNotches() == -1, "a fraction below zero counts the same way");
}

void testWheelReadings()
{
    WheelCounter wheel;
    wheel.add(0);
    require(wheel.readings() == 0, "a still wheel is not a reading");
    wheel.add(120);
    wheel.add(-30);
    require(wheel.readings() == 2, "every movement is a reading, whole notch or not");
    wheel.takeNotches();
    require(wheel.readings() == 2, "taking notches does not reset the count");
}

}

int main()
{
    testEmptyTableChangesNothing();
    testKeyIsRemapped();
    testSwapNeedsNoTemporary();
    testSwallowedKey();
    testTwoKeysOneTargetMerge();
    testIdentityEntryIsNoEntry();
    testBadArgumentsChangeNothing();
    testMouseButtons();
    testMouseRemapOrderIsPhysical();
    testMouseTargetOutsideTheStateIsLost();
    testPadButtons();
    testClearByDevice();
    testWheelCountsWholeNotches();
    testWheelKeepsFractions();
    testWheelReadings();

    std::cout << "[OK] test_input_remap: " << g_checks << " checks passed." << std::endl;
    return 0;
}
