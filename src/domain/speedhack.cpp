/*
** CrabeLoader
** File description:
** speedhack -- game speed by lying about the clock
*/

#include "domain/speedhack.hpp"

#include <atomic>
#include <cstdio>
#include <mutex>
#include <windows.h>

#include "shared/logger.hpp"
#include "infrastructure/hook.hpp"
#include "infrastructure/luacall.hpp"

namespace {

    // One of these per clock. Each keeps its own pair of bases because the game
    // does not start reading them all at the same moment, and a base captured
    // before the first real call would put that clock's origin in the past.
    struct Clock {
        std::atomic<long long> realBase{0};
        std::atomic<long long> fakeBase{0};
        std::atomic<long long> last{0};
        std::atomic<unsigned int> calls{0};
        std::atomic<bool> started{false};
        std::atomic<bool> rebase{false};
    };

    Clock g_performance;
    Clock g_multimedia;
    Clock g_ticks;

    std::atomic<float> g_speed{1.0f};
    std::atomic<bool> g_installed{false};
    std::mutex g_installMutex;

    Hook g_performanceHook;
    Hook g_multimediaHook;
    Hook g_ticksHook;

    using QueryPerformanceCounterFn = BOOL(WINAPI*)(LARGE_INTEGER*);
    using TimeGetTimeFn = DWORD(WINAPI*)();
    using GetTickCountFn = DWORD(WINAPI*)();

    // Shared by all three clocks. `raw` is whatever Windows just returned,
    // widened to 64 bits; the caller narrows the result back if it needs to.
    //
    // The monotonic clamp at the end is not decoration. Reads deliberately take
    // no lock -- at 36000 calls a second on forty threads a mutex here would
    // cost more than the cheat is worth -- so a speed change can be observed
    // half-applied. Refusing to ever return less than the previous answer turns
    // that race into a harmless flat moment instead of time running backwards,
    // which would hand the game a negative delta.
    long long transform(Clock& clock, long long raw)
    {
        clock.calls.fetch_add(1, std::memory_order_relaxed);

        bool expected = false;
        if (clock.started.compare_exchange_strong(expected, true)) {
            clock.realBase.store(raw, std::memory_order_relaxed);
            clock.fakeBase.store(raw, std::memory_order_relaxed);
            clock.last.store(raw, std::memory_order_relaxed);
            return raw;
        }

        // A speed change does not rebase from the setter, because doing so
        // would mean calling the original clock from an arbitrary thread. The
        // next call through this clock does it instead, which is the first
        // moment a fresh raw value is available anyway.
        expected = true;
        if (clock.rebase.compare_exchange_strong(expected, false)) {
            clock.fakeBase.store(clock.last.load(std::memory_order_relaxed),
                                 std::memory_order_relaxed);
            clock.realBase.store(raw, std::memory_order_relaxed);
            return clock.last.load(std::memory_order_relaxed);
        }

        const long long elapsed = raw - clock.realBase.load(std::memory_order_relaxed);
        const double scaled = static_cast<double>(elapsed) * g_speed.load(std::memory_order_relaxed);

        long long fake = clock.fakeBase.load(std::memory_order_relaxed)
                       + static_cast<long long>(scaled);

        const long long previous = clock.last.load(std::memory_order_relaxed);
        if (fake < previous) fake = previous;

        clock.last.store(fake, std::memory_order_relaxed);
        return fake;
    }

    BOOL WINAPI hookedQueryPerformanceCounter(LARGE_INTEGER* out)
    {
        auto original = reinterpret_cast<QueryPerformanceCounterFn>(g_performanceHook.getOriginal());
        if (!original) return FALSE;

        const BOOL ok = original(out);
        if (!ok || !out) return ok;

        out->QuadPart = transform(g_performance, out->QuadPart);
        return ok;
    }

    // timeGetTime and GetTickCount both return milliseconds in a DWORD, which
    // wraps every 49 days. Unsigned arithmetic on the difference handles the
    // wrap on its own, so nothing here has to special-case it.
    DWORD WINAPI hookedTimeGetTime()
    {
        auto original = reinterpret_cast<TimeGetTimeFn>(g_multimediaHook.getOriginal());
        if (!original) return 0;

        return static_cast<DWORD>(transform(g_multimedia, static_cast<long long>(original())));
    }

    DWORD WINAPI hookedGetTickCount()
    {
        auto original = reinterpret_cast<GetTickCountFn>(g_ticksHook.getOriginal());
        if (!original) return 0;

        return static_cast<DWORD>(transform(g_ticks, static_cast<long long>(original())));
    }

    // winmm is loaded by the game itself; asking for it rather than loading it
    // keeps this from pulling in a module the process did not already want.
    uintptr_t exportAddress(const wchar_t* moduleName, const char* functionName)
    {
        HMODULE module = GetModuleHandleW(moduleName);
        if (!module) return 0;

        return reinterpret_cast<uintptr_t>(GetProcAddress(module, functionName));
    }

} // namespace

bool SpeedHack::install()
{
    std::lock_guard<std::mutex> lock(g_installMutex);
    if (g_installed.load()) return true;

    const uintptr_t performance = exportAddress(L"kernel32.dll", "QueryPerformanceCounter");
    const uintptr_t ticks = exportAddress(L"kernel32.dll", "GetTickCount");
    const uintptr_t multimedia = exportAddress(L"winmm.dll", "timeGetTime");

    // QueryPerformanceCounter is the one the game actually leans on, so it is
    // the only clock whose absence is fatal. The other two are hooked when
    // present and shrugged off when not.
    if (!performance) {
        Logger::getInstance().error("SpeedHack: QueryPerformanceCounter not found.");
        return false;
    }

    if (!g_performanceHook.installLogged(performance, &hookedQueryPerformanceCounter,
                                         "SpeedHack", "QueryPerformanceCounter")) {
        return false;
    }

    if (multimedia) {
        g_multimediaHook.installLogged(multimedia, &hookedTimeGetTime, "SpeedHack", "timeGetTime");
    }
    if (ticks) {
        g_ticksHook.installLogged(ticks, &hookedGetTickCount, "SpeedHack", "GetTickCount");
    }

    g_installed.store(true);
    Logger::getInstance().info("SpeedHack: clocks hooked.");
    return true;
}

bool SpeedHack::installed()
{
    return g_installed.load();
}

bool SpeedHack::setSpeed(float value)
{
    if (value < kMinSpeed) value = kMinSpeed;
    if (value > kMaxSpeed) value = kMaxSpeed;

    // Nothing to hook while the speed is real time: an untouched cheat leaves
    // the game's imports exactly as they were.
    if (value == 1.0f && !g_installed.load()) {
        g_speed.store(1.0f);
        return true;
    }
    if (!install()) return false;

    g_performance.rebase.store(true);
    g_multimedia.rebase.store(true);
    g_ticks.rebase.store(true);
    g_speed.store(value);

    Logger::getInstance().info("SpeedHack: game speed x{:.2f}.", value);
    return true;
}

float SpeedHack::speed()
{
    return g_speed.load();
}

std::string SpeedHack::report()
{
    if (!g_installed.load()) return "not installed";

    char line[160];
    std::snprintf(line, sizeof(line), "x%.2f  qpc=%u  timeGetTime=%u  GetTickCount=%u",
                  g_speed.load(),
                  g_performance.calls.load(),
                  g_multimedia.calls.load(),
                  g_ticks.calls.load());
    return line;
}

namespace {

    int __cdecl nativeSetGameSpeed(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        const auto value = static_cast<float>(lua.argToNumber(L, 1, 1.0));
        if (!SpeedHack::setSpeed(value)) return 0;

        lua.pushNumber(L, 1.0);
        return 1;
    }

    int __cdecl nativeGetGameSpeed(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        lua.pushNumber(L, static_cast<double>(SpeedHack::speed()));
        lua.pushNumber(L, SpeedHack::installed() ? 1.0 : 0.0);
        return 2;
    }

    int __cdecl nativeGameSpeedReport(void* L)
    {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        lua.pushString(L, SpeedHack::report());
        return 1;
    }

} // namespace

bool SpeedHackNatives::registerAll(void* L)
{
    struct Entry {
        const char* name;
        int (__cdecl* fn)(void*);
    };

    static const Entry kEntries[] = {
        { "_setGameSpeed",    &nativeSetGameSpeed },
        { "_getGameSpeed",    &nativeGetGameSpeed },
        { "_gameSpeedReport", &nativeGameSpeedReport },
    };

    bool allOk = true;
    for (const auto& entry : kEntries) {
        if (LuaCall::get().registerNativeFunction(L, "Crabe", entry.name, entry.fn)) continue;

        Logger::getInstance().error("SpeedHack: failed to register Crabe.{}.", entry.name);
        allOk = false;
    }
    return allOk;
}
