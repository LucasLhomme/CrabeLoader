/*
** CrabeLoader
** File description:
** memory
*/

#include "loader/memory.hpp"

#include <cstdlib>
#include <vector>

namespace {
    struct PatternByte {
        uint8_t value;
        bool wildcard;
    };

    std::vector<PatternByte> parsePattern(const char* pattern)
    {
        std::vector<PatternByte> bytes;
        const char* p = pattern;

        while (*p) {
            while (*p == ' ') ++p;
            if (!*p) break;

            if (p[0] == '?') {
                bytes.push_back({0, true});
                p += (p[1] == '?') ? 2 : 1;
            } else {
                bytes.push_back({static_cast<uint8_t>(std::strtoul(p, nullptr, 16)), false});
                p += 2;
            }
        }
        return bytes;
    }
}

uintptr_t Memory::PatternScan(const char* pattern, HMODULE module)
{
    auto* base = reinterpret_cast<uint8_t*>(module ? module : GetModuleHandle(nullptr));
    if (!base) return 0;

    auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
    auto* nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dos->e_lfanew);
    size_t imageSize = nt->OptionalHeader.SizeOfImage;

    std::vector<PatternByte> needle = parsePattern(pattern);
    if (needle.empty() || imageSize < needle.size()) return 0;

    for (size_t i = 0; i <= imageSize - needle.size(); ++i) {
        bool matched = true;
        for (size_t j = 0; j < needle.size(); ++j) {
            if (!needle[j].wildcard && base[i + j] != needle[j].value) {
                matched = false;
                break;
            }
        }
        if (matched) {
            return reinterpret_cast<uintptr_t>(base + i);
        }
    }
    return 0;
}
