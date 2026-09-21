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

    // The game runs one Lua state per screen context -- front-end, world, and
    // more as they are entered -- and the loader injects its API into each. A
    // hot reload has to reach all of them.
    //
    // It used to be one bool. F4 set it, and the first state to reach
    // Loader::runTicks cleared it and reloaded itself; every other live state
    // kept running yesterday's mod code. Three states, five F4 presses, five
    // reloads instead of fifteen.
    //
    // The obvious repair -- iterate the set of states the loader has seen --
    // is the one thing that must not be done, for two reasons that both come
    // from where reloads actually run. Loader::runTicks is called from
    // LuaCall::hkPcall, the detour on the game's own lua_pcall, so it runs on
    // whichever thread made that call, holding that thread's state:
    //
    //   * thread affinity (Invariant I3) -- a state may only be touched from
    //     the thread that owns it. Reloading state B from inside state A's
    //     pcall calls into B from A's thread;
    //   * liveness -- nothing observes a state's destruction. lua_close is not
    //     hooked, and is not even among the symbols the profile resolves, so a
    //     torn-down screen context leaves a freed pointer in that set forever.
    //     Calling into it is a use-after-free.
    //
    // A generation counter removes both rather than guarding against them.
    // Nothing iterates: each state asks, on its own thread, whether it is
    // behind, and reloads itself if so. A state that has died never asks again,
    // so a dead pointer is never dereferenced -- the only state ever touched is
    // one currently executing a pcall, which is the strongest liveness proof
    // available and needs no hook at all.
    class ReloadGenerations {
    public:
        // A hot reload was requested. Every state that has not reloaded since
        // is now behind, and will catch up the next time it runs.
        void requestReload();

        // Is `L` behind? False for a state never recorded: a state loads its
        // mods when it is created, so it starts out current.
        [[nodiscard]] bool needsReload(void* L) const;

        // `L` has just loaded its mods. Returns true when it is the first state
        // to do so for this generation -- which is what work that must happen
        // once per reload rather than once per state keys on, such as revoking
        // mod-owned native hooks, which are global and would otherwise be torn
        // down again after the second state had reinstalled them.
        bool markReloaded(void* L);

        // Drops a state's entry. Nothing calls this yet: destruction is not
        // observable, and the cost of not knowing is one pointer-sized map
        // entry per dead state, never a wrong answer. It exists so that a
        // future lua_close hook has somewhere obvious to report to.
        void forget(void* L);

        [[nodiscard]] std::uint64_t generation() const;
        [[nodiscard]] std::size_t trackedStates() const;

        // Recorded states still behind the current generation. Logged after
        // each reload, because "every live state caught up" is otherwise an
        // invisible property: a state that has gone dormant catches up only
        // when it next runs, and nothing would say how many are waiting.
        [[nodiscard]] std::size_t statesBehind() const;

    private:
        // Caller holds _mutex.
        void refreshBehindCount();

        mutable std::mutex _mutex;
        std::uint64_t _generation{0};
        std::uint64_t _servicedGeneration{0};
        std::unordered_map<void*, std::uint64_t> _states;

        // How many recorded states are behind, readable without the lock.
        // needsReload is called from the game's lua_pcall detour thousands of
        // times a second on several threads; taking a mutex there to answer
        // "no" -- which is the answer almost always, a reload being a keypress
        // -- would put a contention point on the hottest path in the loader.
        std::atomic<std::size_t> _behind{0};
    };

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_RELOAD_GENERATION_HPP_ */
