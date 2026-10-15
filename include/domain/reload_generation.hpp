/*
** CrabeLoader
** File description:
** Tracks which live Lua states still owe a hot reload: a counter, plus one generation per state.
** A state reloads itself from its own thread, so no state is ever touched from another's.
** Knows nothing of Lua or of mods; it answers "is this state behind?" and nothing else.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_DOMAIN_RELOAD_GENERATION_HPP_
#define CRABELOADER_DOMAIN_RELOAD_GENERATION_HPP_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace crabe::domain {

    class ReloadGenerations {
    public:
        // Records a hot reload request, advancing the current generation counter.
        void requestReload();

        // Checks whether the given Lua state is behind the current generation.
        [[nodiscard]] bool needsReload(void* L) const;

        // Marks the given Lua state as reloaded, returning true if first in generation.
        bool markReloaded(void* L);

        // Removes a tracked Lua state from the generation registry.
        void forget(void* L);

        [[nodiscard]] std::uint64_t generation() const;
        [[nodiscard]] std::size_t trackedStates() const;

        // Returns the number of tracked states still awaiting a reload.
        [[nodiscard]] std::size_t statesBehind() const;

    private:
        // Recomputes the lock-free count of states behind the current generation.
        void refreshBehindCount();

        mutable std::mutex _mutex;
        std::uint64_t _generation{0};
        std::uint64_t _servicedGeneration{0};
        std::unordered_map<void*, std::uint64_t> _states;
        std::atomic<std::size_t> _behind{0};
    };

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_RELOAD_GENERATION_HPP_ */
