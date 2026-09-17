/*
** CrabeLoader
** File description:
** speedhack -- game speed by lying about the clock
*/

#ifndef SPEEDHACK_HPP_
#define SPEEDHACK_HPP_

#include <cstdint>
#include <string>

// Movement speed, reached from the other side.
//
// Every attempt to make the player faster by writing memory failed, and the
// reason turned out to be structural: the character object carries four copies
// of the same transform and the engine rewrites all of them from upstream every
// frame. A cave on the position integrator ran 4202 times, matched the player
// object on all 4202, and changed nothing at all -- it was writing into a copy
// that was overwritten microseconds later.
//
// So this stops arguing with the engine about position. The engine asks Windows
// how much time has passed and multiplies velocity by the answer; changing the
// answer moves everything downstream, including all four copies.
//
// Each clock reports a fake origin plus the real elapsed time since its own
// origin, multiplied by the chosen rate. Cheat Engine calls this its newer
// approach: no counting thread and no sleep tuning, only an affine transform
// on the clock. Measured on this build, the game reads
// QueryPerformanceCounter about 36000 times a second, timeGetTime about 11000,
// and GetTickCount about 650, so all three are hooked.
//
// It scales the whole simulation -- enemies, animations and physics with the
// player. That is what this cheat is, not a defect: a player-only version does
// not exist at this layer. The menu presents it as Game speed for that reason.
namespace SpeedHack {

    // Hooks the three clocks. Idempotent, and installed lazily on the first
    // speed change so an unused cheat never touches the game's imports.
    bool install();
    bool installed();

    // 1.0 restores real time. Clamped to a sane range; values far from 1 make
    // the game unplayable long before they become interesting.
    bool setSpeed(float speed);
    float speed();

    // How many times each clock has been called, which is what tells a clock
    // the game ignores apart from one it depends on.
    std::string report();

    constexpr float kMinSpeed = 0.1f;
    constexpr float kMaxSpeed = 10.0f;

} // namespace SpeedHack

namespace SpeedHackNatives {
    bool registerAll(void* L);
} // namespace SpeedHackNatives

#endif

