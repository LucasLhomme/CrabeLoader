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

// Recomputes the lock-free hint of states behind generation. Caller holds _mutex.
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

// Increments the target reload generation.
void ReloadGenerations::requestReload()
{
    std::lock_guard<std::mutex> lock(_mutex);
    ++_generation;
    refreshBehindCount();
}

// Checks whether the specified Lua state is behind the current generation.
bool ReloadGenerations::needsReload(void* L) const
{
    if (!L)
        return false;

    if (_behind.load(std::memory_order_acquire) == 0)
        return false;

    std::lock_guard<std::mutex> lock(_mutex);
    const auto it = _states.find(L);
    if (it == _states.end())
        return false;
    return it->second < _generation;
}

// Marks a Lua state as caught up to the current generation.
bool ReloadGenerations::markReloaded(void* L)
{
    if (!L)
        return false;

    std::lock_guard<std::mutex> lock(_mutex);
    _states[L] = _generation;
    refreshBehindCount();

    if (_generation == 0 || _servicedGeneration >= _generation)
        return false;

    _servicedGeneration = _generation;
    return true;
}

// Stops tracking the specified Lua state.
void ReloadGenerations::forget(void* L)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _states.erase(L);
    refreshBehindCount();
}

// Returns the current reload generation counter.
std::uint64_t ReloadGenerations::generation() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _generation;
}

// Returns the number of tracked states that have not caught up to current generation.
std::size_t ReloadGenerations::statesBehind() const
{
    return _behind.load(std::memory_order_acquire);
}

// Returns the count of actively tracked Lua states.
std::size_t ReloadGenerations::trackedStates() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _states.size();
}

} // namespace crabe::domain
