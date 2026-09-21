/*
** CrabeLoader
** File description:
** Implements the hot-reload generation counter and what each Lua state has caught up to.
** needsReload runs from the pcall detour thousands of times a second: the quiet case takes no lock.
** Decides nothing about mods; ModManager asks it when to reload, and reloads.
**
** Authors: @LucasLhomme
*/

#include "domain/reload_generation.hpp"

namespace crabe::domain {

// Recomputes the lock-free hint. Caller holds _mutex.
//
// O(states), and states number about four -- but it only runs on a reload
// request or a catch-up, never on the pcall path that reads the hint.
void ReloadGenerations::refreshBehindCount()
{
    std::size_t behind = 0;
    for (const auto& [state, generation] : _states) {
        (void)state;
        if (generation < _generation)
            ++behind;
    }
    _behind.store(behind, std::memory_order_release);
}

void ReloadGenerations::requestReload()
{
    std::lock_guard<std::mutex> lock(_mutex);
    ++_generation;
    refreshBehindCount();
}

bool ReloadGenerations::needsReload(void* L) const
{
    if (!L)
        return false;

    // The quiet path, and the one that matters: this is reached from
    // LuaCall::hkPcall, which the game calls thousands of times a second on
    // several threads. Once every state has caught up -- which is the steady
    // state, since a reload is a keypress -- the answer is one relaxed atomic
    // load and no lock at all.
    //
    // The hint can lag a request by the width of requestReload's critical
    // section, and that is harmless: a reader that misses it asks again on the
    // next pcall, microseconds later. It can never lag the other way, because
    // the count is only lowered under the same lock that records the catch-up.
    if (_behind.load(std::memory_order_acquire) == 0)
        return false;

    std::lock_guard<std::mutex> lock(_mutex);
    const auto it = _states.find(L);
    // Unknown state: it has not loaded its mods yet, and doing that is
    // discoverAndLoadMods's job on creation, not a reload's.
    if (it == _states.end())
        return false;
    return it->second < _generation;
}

bool ReloadGenerations::markReloaded(void* L)
{
    if (!L)
        return false;

    std::lock_guard<std::mutex> lock(_mutex);
    _states[L] = _generation;
    refreshBehindCount();

    // Generation 0 is the initial load, which no state is ever "first" to:
    // there is nothing installed yet to tear down.
    if (_generation == 0 || _servicedGeneration >= _generation)
        return false;

    _servicedGeneration = _generation;
    return true;
}

void ReloadGenerations::forget(void* L)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _states.erase(L);
    refreshBehindCount();
}

std::uint64_t ReloadGenerations::generation() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _generation;
}

std::size_t ReloadGenerations::statesBehind() const
{
    return _behind.load(std::memory_order_acquire);
}

std::size_t ReloadGenerations::trackedStates() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _states.size();
}

} // namespace crabe::domain
