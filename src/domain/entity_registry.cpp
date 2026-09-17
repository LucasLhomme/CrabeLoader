/*
** CrabeLoader
** File description:
** entity_registry -- who is the player, answered by observation
*/

#include "domain/entity_registry.hpp"

#include <array>
#include <atomic>
#include <cstring>
#include <immintrin.h>

namespace {

    constexpr uintptr_t kHealthOffset = 0x08;
    constexpr uintptr_t kMaxHealthOffset = 0x0C;
    constexpr uint32_t kStaleAfter = 900;

    struct Slot {
        uintptr_t component{0};
        float maxHealth{0.0f};
        float health{0.0f};
        uint32_t seen{0};
        uint32_t damaged{0};
        float lastDelta{0.0f};
        uint32_t lastPass{0};

        void reset(uintptr_t newComp = 0) noexcept {
            component = newComp;
            maxHealth = 0.0f;
            health = 0.0f;
            seen = 0;
            damaged = 0;
            lastDelta = 0.0f;
            lastPass = 0;
        }
    };

    struct SpinLock {
        std::atomic_flag flag = ATOMIC_FLAG_INIT;

        void lock() noexcept {
            while (flag.test_and_set(std::memory_order_acquire)) {
                _mm_pause();
            }
        }

        void unlock() noexcept {
            flag.clear(std::memory_order_release);
        }
    };

    SpinLock g_lock;
    std::array<Slot, EntityRegistry::kMaxEntries> g_slots{};
    size_t g_used = 0;
    uint32_t g_pass = 0;

    alignas(4) uintptr_t g_target = 0;
    uintptr_t g_manual = 0;

    uintptr_t g_lastDamaged = 0;
    float g_lastDelta = 0.0f;

    bool plausible(uintptr_t component) noexcept
    {
        return component >= 0x10000 && (component & 3) == 0;
    }

    float readFloat(uintptr_t at) noexcept
    {
        float value = 0.0f;
        std::memcpy(&value, reinterpret_cast<const void*>(at), sizeof(float));
        return value;
    }

    Slot* existing(uintptr_t component) noexcept
    {
        for (size_t i = 0; i < g_used; ++i) {
            if (g_slots[i].component == component) return &g_slots[i];
        }
        return nullptr;
    }

    Slot* slotFor(uintptr_t component) noexcept
    {
        if (Slot* found = existing(component)) return found;

        if (g_used < EntityRegistry::kMaxEntries) {
            Slot* fresh = &g_slots[g_used++];
            fresh->reset(component);
            return fresh;
        }

        Slot* oldest = &g_slots[0];
        for (size_t i = 1; i < g_used; ++i) {
            if (g_slots[i].lastPass < oldest->lastPass) oldest = &g_slots[i];
        }
        oldest->reset(component);
        return oldest;
    }

    void refreshTarget() noexcept
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

extern "C" void __cdecl CrabeRecordSeen(uintptr_t component)
{
    if (!plausible(component)) return;

    g_lock.lock();
    ++g_pass;

    Slot* slot = slotFor(component);
    slot->health = readFloat(component + kHealthOffset);
    slot->maxHealth = readFloat(component + kMaxHealthOffset);
    slot->lastPass = g_pass;
    ++slot->seen;

    refreshTarget();
    g_lock.unlock();
}

extern "C" void __cdecl CrabeRecordDamage(uintptr_t component, float delta)
{
    if (!plausible(component)) return;

    g_lock.lock();
    Slot* slot = existing(component);
    if (!slot) {
        if (delta >= 0.0f) {
            g_lock.unlock();
            return;
        }
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
    g_lock.unlock();
}

uintptr_t* EntityRegistry::targetSlot() noexcept
{
    return &g_target;
}

uintptr_t EntityRegistry::target() noexcept
{
    return g_target;
}

void EntityRegistry::selectManual(uintptr_t component) noexcept
{
    g_lock.lock();
    g_manual = component;
    refreshTarget();
    g_lock.unlock();
}

uintptr_t EntityRegistry::manualSelection() noexcept
{
    return g_manual;
}

size_t EntityRegistry::count() noexcept
{
    return g_used;
}

uintptr_t EntityRegistry::lastDamagedComponent() noexcept
{
    return g_lastDamaged;
}

float EntityRegistry::lastDamagedDelta() noexcept
{
    return g_lastDelta;
}

void EntityRegistry::clear() noexcept
{
    g_lock.lock();
    g_used = 0;
    g_pass = 0;
    g_lastDamaged = 0;
    g_lastDelta = 0.0f;
    refreshTarget();
    g_lock.unlock();
}

size_t EntityRegistry::snapshot(std::span<Entry> out) noexcept
{
    if (out.empty()) return 0;

    g_lock.lock();
    size_t written = 0;
    std::array<bool, kMaxEntries> taken{};

    while (written < out.size() && written < g_used) {
        size_t bestIndex = kMaxEntries;

        for (size_t i = 0; i < g_used; ++i) {
            if (taken[i]) continue;
            if (bestIndex == kMaxEntries || g_slots[i].seen > g_slots[bestIndex].seen) {
                bestIndex = i;
            }
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
    g_lock.unlock();
    return written;
}

size_t EntityRegistry::snapshot(Entry* out, size_t max) noexcept
{
    if (!out || max == 0) return 0;
    return snapshot(std::span<Entry>(out, max));
}

std::vector<EntityRegistry::Entry> EntityRegistry::snapshot()
{
    std::vector<Entry> result(count());
    size_t written = snapshot(std::span<Entry>(result));
    result.resize(written);
    return result;
}

