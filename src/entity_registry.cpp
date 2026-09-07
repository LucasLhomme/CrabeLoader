/*
** CrabeLoader
** File description:
** entity_registry -- who is the player, answered by observation
*/

#include "loader/entity_registry.hpp"

#include <cstring>

namespace {

    // Health component layout, identical at both cave sites: a float health at
    // offset 8, a float maximum health right behind it.
    constexpr uintptr_t kHealthOffset = 0x08;
    constexpr uintptr_t kMaxHealthOffset = 0x0C;

    // A component not seen for this many passes is treated as gone. Without
    // it a level reload would leave the previous avatar holding an unbeatable
    // count, and the clamp would guard a pointer the engine had freed.
    constexpr uint32_t kStaleAfter = 900;

    struct Slot {
        uintptr_t component;
        float maxHealth;
        float health;
        uint32_t seen;
        uint32_t damaged;
        float lastDelta;
        uint32_t lastPass;
    };

    Slot g_slots[EntityRegistry::kMaxEntries];
    size_t g_used = 0;
    uint32_t g_pass = 0;

    uintptr_t g_target = 0;
    uintptr_t g_manual = 0;

    uintptr_t g_lastDamaged = 0;
    float g_lastDelta = 0.0f;

    // The engine hands us pointers it has already tested, so a full page query
    // would only cost time. Reject the shapes that cannot be a live object.
    bool plausible(uintptr_t component)
    {
        return component >= 0x10000 && (component & 3) == 0;
    }

    float readFloat(uintptr_t at)
    {
        float value = 0.0f;
        std::memcpy(&value, reinterpret_cast<const void*>(at), sizeof(float));
        return value;
    }

    // Finds the slot for `component`, adding one if there is room. When the
    // table is full the stalest slot is recycled, which is what keeps a level
    // change from permanently filling it with corpses.
    Slot* existing(uintptr_t component)
    {
        for (size_t i = 0; i < g_used; ++i) {
            if (g_slots[i].component == component) return &g_slots[i];
        }
        return nullptr;
    }

    Slot* slotFor(uintptr_t component)
    {
        if (Slot* found = existing(component)) return found;

        if (g_used < EntityRegistry::kMaxEntries) {
            Slot* fresh = &g_slots[g_used++];
            std::memset(fresh, 0, sizeof(*fresh));
            fresh->component = component;
            return fresh;
        }

        Slot* oldest = &g_slots[0];
        for (size_t i = 1; i < g_used; ++i) {
            if (g_slots[i].lastPass < oldest->lastPass) oldest = &g_slots[i];
        }
        std::memset(oldest, 0, sizeof(*oldest));
        oldest->component = component;
        return oldest;
    }

    // Most-seen wins, but only among components still being drawn. The avatar
    // meter is on screen every frame; an enemy meter is not, so the player
    // pulls ahead and stays ahead.
    void refreshTarget()
    {
        if (g_manual) {
            g_target = g_manual;
            return;
        }

        Slot* best = nullptr;
        for (size_t i = 0; i < g_used; ++i) {
            Slot* candidate = &g_slots[i];
            if (g_pass - candidate->lastPass > kStaleAfter) continue;
            if (!best || candidate->seen > best->seen) best = candidate;
        }
        g_target = best ? best->component : 0;
    }

} // namespace

// Called from the capture cave with the health component the HUD is about to
// draw a bar for. Counts the sighting and re-decides who the player is.
extern "C" void __cdecl CrabeRecordSeen(uintptr_t component)
{
    if (!plausible(component)) return;

    ++g_pass;

    Slot* slot = slotFor(component);
    slot->health = readFloat(component + kHealthOffset);
    slot->maxHealth = readFloat(component + kMaxHealthOffset);
    slot->lastPass = g_pass;
    ++slot->seen;

    refreshTarget();
}

// Called from the damage cave before the clamp, with the delta the engine is
// about to apply. Negative means a hit landed.
extern "C" void __cdecl CrabeRecordDamage(uintptr_t component, float delta)
{
    if (!plausible(component)) return;

    // Only a loss earns a slot. Spawning and healing push non-negative deltas
    // through here for every entity in the level, which filled the table with
    // rows nobody could act on and buried the one that mattered.
    Slot* slot = existing(component);
    if (!slot) {
        if (delta >= 0.0f) return;
        slot = slotFor(component);
    }

    slot->health = readFloat(component + kHealthOffset);
    slot->maxHealth = readFloat(component + kMaxHealthOffset);
    slot->lastPass = g_pass;
    slot->lastDelta = delta;

    if (delta < 0.0f) {
        ++slot->damaged;
        g_lastDamaged = component;
        g_lastDelta = delta;
    }
}

uintptr_t* EntityRegistry::targetSlot()
{
    return &g_target;
}

uintptr_t EntityRegistry::target()
{
    return g_target;
}

void EntityRegistry::selectManual(uintptr_t component)
{
    g_manual = component;
    refreshTarget();
}

uintptr_t EntityRegistry::manualSelection()
{
    return g_manual;
}

size_t EntityRegistry::count()
{
    return g_used;
}

uintptr_t EntityRegistry::lastDamagedComponent()
{
    return g_lastDamaged;
}

float EntityRegistry::lastDamagedDelta()
{
    return g_lastDelta;
}

void EntityRegistry::clear()
{
    g_used = 0;
    g_pass = 0;
    g_lastDamaged = 0;
    g_lastDelta = 0.0f;
    refreshTarget();
}

// Ordered most-seen first so the menu lists the likely player at the top. The
// cave may update a slot mid-copy; a row can be one frame stale, which costs
// nothing because every field here is only ever displayed.
size_t EntityRegistry::snapshot(Entry* out, size_t max)
{
    if (!out || max == 0) return 0;

    size_t written = 0;
    bool taken[kMaxEntries] = {};

    while (written < max && written < g_used) {
        size_t bestIndex = kMaxEntries;

        for (size_t i = 0; i < g_used; ++i) {
            if (taken[i]) continue;
            if (bestIndex == kMaxEntries || g_slots[i].seen > g_slots[bestIndex].seen) bestIndex = i;
        }
        if (bestIndex == kMaxEntries) break;

        taken[bestIndex] = true;
        const Slot& slot = g_slots[bestIndex];
        out[written].component = slot.component;
        out[written].maxHealth = slot.maxHealth;
        out[written].health = slot.health;
        out[written].seen = slot.seen;
        out[written].damaged = slot.damaged;
        out[written].lastDelta = slot.lastDelta;
        ++written;
    }
    return written;
}
