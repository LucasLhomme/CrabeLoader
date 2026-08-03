/*
** CrabeLoader
** File description:
** avatar_relay_hook
*/

#ifndef AVATAR_RELAY_HOOK_HPP_
#define AVATAR_RELAY_HOOK_HPP_

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

#include "loader/hook.hpp"

// Hooks sub_34B200 (avatar handle resolver). When armed, replays the handle
// it just resolved into GamePlayers::vtable[7] for another player index,
// same call stack. Single-shot, disarmed by default.
class AvatarRelayHook {
    public:
        static AvatarRelayHook& get();

        bool initialize();
        void uninitialize();

        // Arms a one-shot relay for the next resolved handle. False if the
        // hook isn't installed or gamePlayersThis is 0.
        bool arm(uintptr_t gamePlayersThis, int targetPlayerIndex);
        void disarm();

        // "armed=0 fired=1 lastHandle=0x... lastRelayResult=..."
        std::string report() const;

    private:
        AvatarRelayHook() = default;
        ~AvatarRelayHook() = default;
        AvatarRelayHook(const AvatarRelayHook&) = delete;
        AvatarRelayHook& operator=(const AvatarRelayHook&) = delete;

        // __cdecl, one stack arg, doesn't touch ecx.
        typedef int(__cdecl* t_Resolve)(int arg);
        static int __cdecl hkResolve(int arg);

        // __thiscall, 3 stack args (ret 0xC) -- GamePlayers::vtable[7].
        typedef int(__thiscall* t_ThisCall3)(void* self, int a1, int a2, int a3);

        static bool relayGuarded(uintptr_t relayFn, uintptr_t gamePlayersThis,
                                int playerIndex, int handle, int& outResult);

        Hook _hook;
        uintptr_t _relayFn = 0;

        mutable std::mutex _mutex;
        bool _armed = false;
        uintptr_t _gamePlayersThis = 0;
        int _targetPlayerIndex = 0;

        std::atomic<bool> _fired{false};
        std::atomic<int> _lastHandle{0};
        std::atomic<int> _lastRelayResult{0};
};

#endif /* !AVATAR_RELAY_HOOK_HPP_ */
