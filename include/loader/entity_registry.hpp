/*
** CrabeLoader
** File description:
** entity_registry
*/

#ifndef ENTITY_REGISTRY_HPP_
#define ENTITY_REGISTRY_HPP_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

// Every health component the game touches, catalogued as it goes past.
//
// Why this exists: the avatar health meter and the NPC health meter are the
// same C++ function -- in the game's own Lua, hud_AvatarHealthMeter derives
// from hud_NPCHealthMeter -- so the player and every enemy stream through one
// site. Telling them apart by how much health they have cannot work: a low bar
// matches every NPC and the last one drawn wins, a high bar matches nothing.
//
// So nothing is guessed. Each component is recorded with how often it has been
// seen and how often it has been damaged, and the menu can list them. The
// automatic pick is the most-seen component, which is the player's: the avatar
// meter is drawn every single frame of a session, while an enemy's meter only
// appears while that enemy is engaged. The gap grows without bound, so the
// choice settles within seconds and stays settled.
namespace EntityRegistry {

    // Room for the player plus every enemy that can plausibly be on screen.
    // Full is not an error; late arrivals are simply not catalogued.
    constexpr size_t kMaxEntries = 32;

    struct Entry {
        uintptr_t component{0};
        float maxHealth{0.0f};
        float health{0.0f};
        uint32_t seen{0};
        uint32_t damaged{0};
        float lastDelta{0.0f};
    };

    // Address the clamp cave reads to know who it protects. Stable for the
    // lifetime of the process, which is what lets the cave bake it in.
    uintptr_t* targetSlot() noexcept;
    uintptr_t target() noexcept;

    // Pins the clamp to one component. Zero hands control back to the
    // automatic pick described above.
    void selectManual(uintptr_t component) noexcept;
    uintptr_t manualSelection() noexcept;

    // Copies out entries ordered most-seen first.
    size_t snapshot(std::span<Entry> out) noexcept;
    size_t snapshot(Entry* out, size_t max) noexcept;
    std::vector<Entry> snapshot();
    size_t count() noexcept;

    // The component that last lost health, and by how much. This is the
    // ground truth for identifying the player: get hit, read it back.
    uintptr_t lastDamagedComponent() noexcept;
    float lastDamagedDelta() noexcept;

    void clear() noexcept;

} // namespace EntityRegistry

// Called from cave code with the game's own registers, so these have to stay
// cheap and total: no allocation, no locks, no logging, and no exception can
// be allowed to escape into the middle of an engine function.
extern "C" {
    void __cdecl CrabeRecordSeen(uintptr_t component);
    void __cdecl CrabeRecordDamage(uintptr_t component, float delta);
}

#endif /* !ENTITY_REGISTRY_HPP_ */
