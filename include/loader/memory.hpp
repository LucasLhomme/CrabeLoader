/*
** CrabeLoader
** File description:
** memory
*/

#ifndef MEMORY_HPP_
#define MEMORY_HPP_

#include <cstddef>
#include <cstdint>
#include <windows.h>

// Read-only introspection of the host process' main module.
//
// The game exports no Lua symbol and ships no debug info, so every address the
// loader needs is resolved at runtime. Byte signatures proved unreliable on this
// build; the working method is findRegisteredFunction() + findNthCall(), which
// follows the game's own Lua standard-library registration table.
namespace Memory {
    // Scans the mapped image for an IDA-style byte pattern
    // (e.g. "55 8B EC ?? 8B 45 10", "?" or "??" being a wildcard byte).
    // Returns the absolute address of the first match, or 0 if not found.
    uintptr_t patternScan(const char* pattern, HMODULE module = nullptr);

    // True if [addr, addr + size) is committed and readable.
    bool isReadable(uintptr_t addr, size_t size);

    // Address of the first occurrence of `text` (its terminating NUL included)
    // in the module's readable memory, or 0 if absent.
    uintptr_t findString(const char* text);

    // Lua registers its standard library as an array of
    // { const char* name; lua_CFunction fn; } pairs. This locates the entry whose
    // name is `funcName` and returns its function pointer -- i.e. the *wrapper*
    // the interpreter calls (luaB_loadfile for "loadfile", ...), not the
    // underlying C API function. Returns 0 if not found.
    uintptr_t findRegisteredFunction(const char* funcName);

    // Target of the `call rel32` (opcode E8) at `addr`, or 0 if there is none.
    uintptr_t resolveCall(uintptr_t addr);

    // Target of the `n`-th `call rel32` found in the first `maxScan` bytes of the
    // function starting at `functionStart`. This is how a wrapper is turned into
    // the C API function it delegates to. `n` is 1-based. Returns 0 on failure.
    uintptr_t findNthCall(uintptr_t functionStart, int n, size_t maxScan = 256);

    // Total size of the whole instructions covering at least `minLen` bytes at
    // `addr`. Returns 0 if an instruction is unknown or position-dependent
    // (relative call/jump), meaning those bytes cannot be copied verbatim into a
    // trampoline. A hook must steal exactly this many bytes: stealing a fixed
    // count cuts an instruction in half and crashes the host process.
    size_t prologueLength(uintptr_t addr, size_t minLen);
}

#endif /* !MEMORY_HPP_ */
