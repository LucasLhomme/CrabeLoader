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

namespace crabe::memory {
    struct PatternByte {
        uint8_t value = 0;
        bool wildcard = false;
    };

    // Parses an IDA-style pattern string into a vector of PatternByte tokens.
    std::vector<PatternByte> parsePattern(const char* pattern);

    inline constexpr size_t kNoMatch = static_cast<size_t>(-1);

    // Finds the first occurrence of needle in haystack.
    // Returns byte offset or kNoMatch if not found.
    size_t findPattern(std::span<const uint8_t> haystack, std::span<const PatternByte> needle);

    // Parses pattern and finds its first occurrence in haystack.
    // Returns byte offset or kNoMatch if not found.
    size_t findPattern(std::span<const uint8_t> haystack, const char* pattern);

    // Scans module memory for an IDA-style byte pattern.
    // Returns matched address or 0 if not found.
    uintptr_t patternScan(const char* pattern, HMODULE module = nullptr, uintptr_t after = 0);

    // Checks if the memory range [addr, addr + size) is committed and readable.
    bool isReadable(uintptr_t addr, size_t size);

    // Searches process memory for pointers holding value up to limit occurrences.
    std::vector<uintptr_t> findPointers(uintptr_t value, size_t limit = 32);

    // Finds the first occurrence of text at an address strictly above after.
    uintptr_t findString(const char* text, uintptr_t after = 0);

    // Returns the address of the Lua stdlib wrapper registered under funcName.
    uintptr_t findRegisteredFunction(const char* funcName);

    // Returns all distinct function pointers registered under funcName.
    std::vector<uintptr_t> findRegisteredFunctions(const char* funcName);

    // Resolves the destination address of a call rel32 instruction at addr.
    uintptr_t resolveCall(uintptr_t addr);

    // Finds call sites within the module targeting the given address.
    std::vector<uintptr_t> findCallSites(uintptr_t target, size_t limit = 32);

    // Finds call destinations within the first maxScan bytes of functionStart.
    std::vector<uintptr_t> findCalls(uintptr_t functionStart, size_t maxScan = 256);

} // namespace crabe::memory

#endif /* !MEMORY_HPP_ */
