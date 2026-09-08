/*
** CrabeLoader
** File description:
** avatar_relay_hook
*/

#include <format>
#include <windows.h>

#include "infrastructure/avatar_relay_hook.hpp"
#include "infrastructure/memory.hpp"
#include "shared/logger.hpp"

namespace {
    // Code RVAs, usable directly as base + RVA (Disney Infinity 3.0, confirmed live).
    constexpr uintptr_t kResolveRva = 0x34B200; // sub_34B200: sku/handle -> handle
    constexpr uintptr_t kRelayRva   = 0x68D680; // GamePlayers::vtable[7]
}

AvatarRelayHook& AvatarRelayHook::get()
{
    static AvatarRelayHook instance;
    return instance;
}

bool AvatarRelayHook::initialize()
{
    Logger& logger = Logger::getInstance();

    uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    if (!base) {
        logger.warning("AvatarRelayHook: could not resolve the module base.");
        return false;
    }

    uintptr_t resolveFn = base + kResolveRva;
    _relayFn = base + kRelayRva;

    if (!Memory::isReadable(resolveFn, 16) || !Memory::isReadable(_relayFn, 16)) {
        logger.warning("AvatarRelayHook: resolver (0x{:X}) or relay target (0x{:X}) not mapped; disabled.",
                        resolveFn, _relayFn);
        return false;
    }

    if (!_hook.install(reinterpret_cast<void*>(resolveFn), reinterpret_cast<void*>(&AvatarRelayHook::hkResolve))) {
        logger.error("AvatarRelayHook: failed to hook the avatar resolver at 0x{:X}.", resolveFn);
        return false;
    }

    logger.info("AvatarRelayHook: watching the avatar resolver @ 0x{:X}, disarmed. Relay target @ 0x{:X}.",
                resolveFn, _relayFn);
    return true;
}

void AvatarRelayHook::uninitialize()
{
    disarm();
    _hook.remove();
}

bool AvatarRelayHook::arm(uintptr_t gamePlayersThis, int targetPlayerIndex)
{
    if (!_hook.isInstalled() || !gamePlayersThis) return false;

    std::lock_guard<std::mutex> lock(_mutex);
    _gamePlayersThis = gamePlayersThis;
    _targetPlayerIndex = targetPlayerIndex;
    _armed = true;
    _fired.store(false, std::memory_order_relaxed);

    Logger::getInstance().info("AvatarRelayHook: armed for player {} (gamePlayers=0x{:X}).",
                                targetPlayerIndex, gamePlayersThis);
    return true;
}

void AvatarRelayHook::disarm()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _armed = false;
}

std::string AvatarRelayHook::report() const
{
    bool armed;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        armed = _armed;
    }

    return std::format("armed={} fired={} lastHandle=0x{:X} lastRelayResult={}",
                        armed ? 1 : 0,
                        _fired.load(std::memory_order_relaxed) ? 1 : 0,
                        static_cast<unsigned>(_lastHandle.load(std::memory_order_relaxed)),
                        _lastRelayResult.load(std::memory_order_relaxed));
}

// Isolated from hkResolve so the SEH __try below doesn't have to coexist
// with an object needing unwinding in the same function.
bool AvatarRelayHook::relayGuarded(uintptr_t relayFn, uintptr_t gamePlayersThis,
                                    int playerIndex, int handle, int& outResult)
{
    auto fn = reinterpret_cast<t_ThisCall3>(relayFn);

    __try {
        outResult = fn(reinterpret_cast<void*>(gamePlayersThis), playerIndex, handle, 0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    return true;
}

// The real resolve always runs first, unconditionally -- the relay is a side
// effect bolted on after, never a replacement for it.
int __cdecl AvatarRelayHook::hkResolve(int arg)
{
    AvatarRelayHook& self = get();
    auto original = reinterpret_cast<t_Resolve>(self._hook.getOriginal());

    int result = original ? original(arg) : 0;
    if (result == 0) return result;

    uintptr_t gamePlayersThis = 0;
    int targetPlayerIndex = 0;
    bool shouldRelay = false;

    {
        std::lock_guard<std::mutex> lock(self._mutex);
        if (self._armed) {
            gamePlayersThis = self._gamePlayersThis;
            targetPlayerIndex = self._targetPlayerIndex;
            shouldRelay = true;
            self._armed = false;
        }
    }

    if (shouldRelay) {
        int relayResult = 0;
        bool ok = relayGuarded(self._relayFn, gamePlayersThis, targetPlayerIndex, result, relayResult);

        self._lastHandle.store(result, std::memory_order_relaxed);
        self._lastRelayResult.store(ok ? relayResult : -1, std::memory_order_relaxed);
        self._fired.store(true, std::memory_order_relaxed);

        Logger::getInstance().info(
            "AvatarRelayHook: fired for player {} with handle 0x{:X} -> relay {} (result {}).",
            targetPlayerIndex, static_cast<unsigned>(result),
            ok ? "ran" : "faulted", relayResult);
    }

    return result;
}
