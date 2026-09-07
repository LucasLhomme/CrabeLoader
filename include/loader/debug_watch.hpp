/*
** CrabeLoader
** File description:
** debug_watch -- which instruction touched this address
*/

#ifndef DEBUG_WATCH_HPP_
#define DEBUG_WATCH_HPP_

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

// Cheat Engine's "find out what writes to this address", in process.
//
// Every cheat in this loader so far was built by pattern-scanning byte
// signatures copied from a community table and hoping they meant what their
// labels claimed. The velocity sites are the proof that this does not work:
// all four execute, the multiply runs, and the player's speed is unchanged,
// because nobody ever verified those AOBs describe the store that matters.
//
// This flips the method around. Point it at a field whose meaning is already
// known -- because it was watched changing in game -- and the CPU reports the
// instruction that wrote it. No guessing, no inherited signatures.
//
// Mechanism: an x86 debug register (DR0) programmed to trap on write, plus a
// vectored exception handler to catch the trap. The trap costs nothing until
// it fires, unlike a PAGE_GUARD, which would fault on every access to the
// surrounding 4 KB and drag the frame rate down with it.
namespace DebugWatch {

    enum class Access {
        Write = 1,      // stores only
        ReadWrite = 3   // loads and stores; noisy, but finds readers too
    };

    // Arms DR0 on every thread in the process. `size` must be 1, 2 or 4 and
    // `address` must be aligned to it -- the hardware has no way to express
    // anything else, and a misaligned request would silently watch elsewhere.
    bool arm(uintptr_t address, size_t size, Access access);

    void disarm();
    bool armed();
    uintptr_t watching();

    // Threads created after arm() carry no debug registers.
    void refreshThreads();

    // One row per distinct instruction, most frequent first. `instruction` is
    // the address the trap reported: a data breakpoint fires *after* the store
    // completes, so the store itself is the instruction ending there.
    struct Hit {
        uintptr_t instruction{0};
        uint32_t count{0};
        std::array<uint32_t, 8> registers{}; // eax ecx edx ebx esp ebp esi edi at trap time
    };

    size_t hits(std::span<Hit> out);
    size_t hits(Hit* out, size_t max);
    size_t hitCount();
    void reset();

    // Human-readable form of the above, module-relative so the addresses can
    // be compared against a disassembler across runs.
    std::string report(size_t maxRows = 8);

    constexpr size_t kMaxHits = 32;

} // namespace DebugWatch

// Lua bindings. Registered from here rather than from the table in
// lua_natives.cpp, which is already within a dozen lines of the file cap.
namespace DebugWatchNatives {
    bool registerAll(void* L);
}

#endif /* !DEBUG_WATCH_HPP_ */
