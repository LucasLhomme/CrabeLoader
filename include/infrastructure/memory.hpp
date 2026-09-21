/*
** CrabeLoader
** File description:
** Declares read-only introspection of the host process: pattern scans, strings, call resolution.
** The game ships no debug info, so every address is measured at runtime over readable regions.
** Writes nothing. Patching bytes is infrastructure/multiplayer/memory_patcher.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef MEMORY_HPP_
#define MEMORY_HPP_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <windows.h>

// Read-only introspection of the host process' main module. The game ships
// no debug info, so every address is resolved at runtime instead.
namespace crabe::memory {
    // One token of a parsed IDA-style pattern. `??` (or a bare `?`) is a
    // wildcard and matches any byte.
    struct PatternByte {
        uint8_t value = 0;
        bool wildcard = false;
    };

    // Splits "55 8B EC ?? 8B 45 10" into one PatternByte per token.
    // Deliberately forgiving: a token strtoul cannot read yields 0, which is
    // what the scan has always done -- a malformed pattern silently matches
    // zero bytes rather than being rejected.
    std::vector<PatternByte> parsePattern(const char* pattern);

    // Returned by findPattern when the needle does not occur.
    inline constexpr size_t kNoMatch = static_cast<size_t>(-1);

    // Offset of the first occurrence of `needle` in `haystack`, or kNoMatch.
    // The pure core of patternScan: no process, no module, no page walk, so
    // it can be exercised against a synthetic buffer.
    size_t findPattern(std::span<const uint8_t> haystack, std::span<const PatternByte> needle);

    // Same, parsing `pattern` first. kNoMatch for an empty pattern.
    size_t findPattern(std::span<const uint8_t> haystack, const char* pattern);

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

    // Address of the Lua stdlib wrapper registered under `funcName` -- the
    // wrapper itself, not the underlying C API function. Names can be
    // ambiguous, so prefer findRegisteredFunctions when several may exist.
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

}

#endif /* !MEMORY_HPP_ */
