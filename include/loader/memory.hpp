/*
** CrabeLoader
** File description:
** memory
*/

#ifndef MEMORY_HPP_
#define MEMORY_HPP_

#include <cstdint>
#include <windows.h>

namespace Memory {
    // Scans `module`'s mapped image for an IDA-style byte pattern
    // (e.g. "55 8B EC ?? 8B 45 10", "?"/"??" as a wildcard byte).
    // Returns the absolute address of the first match, or 0 if not found.
    uintptr_t PatternScan(const char* pattern, HMODULE module = nullptr);
}

#endif /* !MEMORY_HPP_ */
