/*
** CrabeLoader
** File description:
** memory
*/

#ifndef MEMORY_HPP_
#define MEMORY_HPP_

#include <cstddef>
#include <cstdint>
#include <vector>
#include <windows.h>

// Read-only introspection of the host process' main module. The game ships
// no debug info, so every address is resolved at runtime instead.
namespace Memory {
    // IDA-style byte pattern scan (e.g. "55 8B EC ?? 8B 45 10"). `after`
    // walks past a previous match to find the next one.
    uintptr_t patternScan(const char* pattern, HMODULE module = nullptr, uintptr_t after = 0);

    // True if [addr, addr + size) is committed and readable.
    bool isReadable(uintptr_t addr, size_t size);

    // Addresses holding a pointer equal to `value`, across the whole process
    // (not just the image) -- how heap object instances are found.
    std::vector<uintptr_t> findPointers(uintptr_t value, size_t limit = 32);

    // First occurrence of `text` strictly above `after`, or 0. `after` lets
    // a caller walk past occurrences no registration table points at.
    uintptr_t findString(const char* text, uintptr_t after = 0);

    // Address of the Lua stdlib wrapper registered under `funcName` (the
    // wrapper, not the underlying C API function). Ambiguous names collide;
    // prefer findRegisteredFunctions() when more than one may exist.
    uintptr_t findRegisteredFunction(const char* funcName);

    // Every distinct function pointer registered under `funcName`.
    std::vector<uintptr_t> findRegisteredFunctions(const char* funcName);

    // Target of the `call rel32` (opcode E8) at `addr`, or 0 if none.
    uintptr_t resolveCall(uintptr_t addr);

    // Every `call rel32` in the module targeting `target` -- the reverse of
    // resolveCall, found by walking the whole image (no other way).
    std::vector<uintptr_t> findCallSites(uintptr_t target, size_t limit = 32);

    // Targets of every `call rel32` in the first `maxScan` bytes at
    // `functionStart`, in order. Only counts calls landing in readable memory.
    std::vector<uintptr_t> findCalls(uintptr_t functionStart, size_t maxScan = 256);

    // Target of the `n`-th (1-based) call found by findCalls. Turns a
    // wrapper into the real function it delegates to.
    uintptr_t findNthCall(uintptr_t functionStart, int n, size_t maxScan = 256);

    // Size of the whole instructions covering >= minLen bytes at `addr`; 0 if
    // one is position-dependent. A hook must steal exactly this many bytes.
    size_t prologueLength(uintptr_t addr, size_t minLen);
}

#endif /* !MEMORY_HPP_ */
