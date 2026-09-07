/*
** CrabeLoader
** File description:
** debug_watch -- which instruction touched this address
*/

#include "loader/debug_watch.hpp"
#include "logger/logger.hpp"

#include <array>
#include <mutex>
#include <windows.h>
#include <tlhelp32.h>

namespace {

    std::mutex g_mutex;

    PVOID g_handler = nullptr;
    uintptr_t g_address = 0;
    size_t g_size = 0;
    DebugWatch::Access g_access = DebugWatch::Access::Write;
    bool g_armed = false;

    std::array<DebugWatch::Hit, DebugWatch::kMaxHits> g_hits{};
    size_t g_used = 0;


    // DR7 bit layout for breakpoint 0: L0 enables it, LE and GE make the
    // report precise on the processors that still honour them, R/W0 picks the
    // access kind and LEN0 the width. The odd length encoding -- four bytes is
    // 0b11, not 0b10 -- is the hardware's, not a typo.
    uint32_t buildControl(size_t size, DebugWatch::Access access)
    {
        uint32_t length = 0;
        if (size == 2) length = 1;
        else if (size == 4) length = 3;

        uint32_t control = 1u;
        control |= (1u << 8) | (1u << 9);
        control |= (static_cast<uint32_t>(access) & 3u) << 16;
        control |= (length & 3u) << 18;
        return control;
    }

    // A hit is keyed by the reporting address, so a loop storing through the
    // same instruction a thousand times stays one row with a count.
    void record(const CONTEXT* context)
    {
        for (size_t i = 0; i < g_used; ++i) {
            if (g_hits[i].instruction != context->Eip) continue;
            ++g_hits[i].count;
            return;
        }
        if (g_used >= DebugWatch::kMaxHits) return;

        DebugWatch::Hit& fresh = g_hits[g_used++];
        fresh.instruction = context->Eip;
        fresh.count = 1;
        fresh.registers[0] = context->Eax;
        fresh.registers[1] = context->Ecx;
        fresh.registers[2] = context->Edx;
        fresh.registers[3] = context->Ebx;
        fresh.registers[4] = context->Esp;
        fresh.registers[5] = context->Ebp;
        fresh.registers[6] = context->Esi;
        fresh.registers[7] = context->Edi;
    }

    // Runs on whatever game thread did the store. Everything here has to be
    // allocation free and non blocking: the handler can fire between any two
    // instructions, including inside the allocator itself.
    LONG CALLBACK onException(EXCEPTION_POINTERS* info)
    {
        if (info->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP) {
            return EXCEPTION_CONTINUE_SEARCH;
        }

        CONTEXT* context = info->ContextRecord;

        // DR6 bit 0 is the processor saying "breakpoint 0", not "single step".
        // Any other single step belongs to somebody else and must be passed
        // on, or this handler would swallow the game's own debugging.
        if (!(context->Dr6 & 1u)) return EXCEPTION_CONTINUE_SEARCH;

        if (g_mutex.try_lock()) {
            if (g_armed) record(context);
            g_mutex.unlock();
        }

        context->Dr6 = 0;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    // Writes the debug registers into one thread. Suspending first is what
    // makes the context coherent; the calling thread is never suspended,
    // because it would never wake up to resume itself.
    bool applyTo(HANDLE thread, bool isSelf, uintptr_t address, uint32_t control)
    {
        if (!isSelf && SuspendThread(thread) == static_cast<DWORD>(-1)) return false;

        CONTEXT context = {};
        context.ContextFlags = CONTEXT_DEBUG_REGISTERS;

        bool ok = false;
        if (GetThreadContext(thread, &context)) {
            context.Dr0 = address;
            context.Dr6 = 0;
            context.Dr7 = control;
            context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
            ok = SetThreadContext(thread, &context) != 0;
        }

        if (!isSelf) ResumeThread(thread);
        return ok;
    }

    // The debug registers are per-thread state, so a process wide watch means
    // walking every thread. There is no API that does this in one call.
    size_t applyToAll(uintptr_t address, uint32_t control)
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return 0;

        const DWORD self = GetCurrentThreadId();
        const DWORD owner = GetCurrentProcessId();

        THREADENTRY32 entry = {};
        entry.dwSize = sizeof(entry);
        size_t applied = 0;

        if (Thread32First(snapshot, &entry)) {
            do {
                if (entry.th32OwnerProcessID != owner) continue;

                HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT
                                           | THREAD_SUSPEND_RESUME, FALSE, entry.th32ThreadID);
                if (!thread) continue;

                if (applyTo(thread, entry.th32ThreadID == self, address, control)) ++applied;
                CloseHandle(thread);
            } while (Thread32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return applied;
    }

} // namespace

bool DebugWatch::arm(uintptr_t address, size_t size, Access access)
{
    if (size != 1 && size != 2 && size != 4) return false;
    if (!address || (address & (size - 1)) != 0) return false;

    std::lock_guard<std::mutex> lock(g_mutex);

    // Registered first in the chain so the trap is seen before any handler the
    // game installs, and kept for the process lifetime: unregistering it while
    // another thread is inside it is a race with no way to detect it.
    if (!g_handler) {
        g_handler = AddVectoredExceptionHandler(1, &onException);
        if (!g_handler) {
            Logger::getInstance().error("DebugWatch: AddVectoredExceptionHandler failed.");
            return false;
        }
    }

    g_used = 0;
    g_address = address;
    g_size = size;
    g_access = access;

    const size_t applied = applyToAll(address, buildControl(size, access));
    g_armed = applied > 0;

    if (!g_armed) {
        Logger::getInstance().error("DebugWatch: no thread accepted a breakpoint on 0x{:X}.", address);
        return false;
    }

    Logger::getInstance().info("DebugWatch: watching {} bytes at 0x{:X} across {} threads.",
                               size, address, applied);
    return true;
}

void DebugWatch::disarm()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_armed) return;

    g_armed = false;
    applyToAll(0, 0);
    Logger::getInstance().info("DebugWatch: disarmed 0x{:X}, {} distinct instructions seen.",
                               g_address, g_used);
    g_address = 0;
}

bool DebugWatch::armed()
{
    return g_armed;
}

uintptr_t DebugWatch::watching()
{
    return g_address;
}

void DebugWatch::refreshThreads()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_armed) return;
    applyToAll(g_address, buildControl(g_size, g_access));
}

size_t DebugWatch::hits(std::span<Hit> out)
{
    if (out.empty()) return 0;

    std::lock_guard<std::mutex> lock(g_mutex);
    size_t written = 0;
    std::array<bool, kMaxHits> taken{};

    while (written < out.size() && written < g_used) {
        size_t best = kMaxHits;
        for (size_t i = 0; i < g_used; ++i) {
            if (taken[i]) continue;
            if (best == kMaxHits || g_hits[i].count > g_hits[best].count) best = i;
        }
        if (best == kMaxHits) break;

        taken[best] = true;
        out[written++] = g_hits[best];
    }
    return written;
}

size_t DebugWatch::hits(Hit* out, size_t max)
{
    if (!out || max == 0) return 0;
    return hits(std::span<Hit>(out, max));
}

size_t DebugWatch::hitCount()
{
    return g_used;
}

void DebugWatch::reset()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_used = 0;
}

