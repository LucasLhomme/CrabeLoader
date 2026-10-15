/*
** CrabeLoader
** File description:
** Implements the read-only scans: byte patterns, strings, pointer hunts and call resolution.
** Every scan walks only committed readable regions, which is what stops it faulting mid-sweep.
** Writes nothing; changing bytes is src/infrastructure/multiplayer/memory_patcher.cpp.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/memory.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

    struct ModuleRange {
        uintptr_t base = 0;
        size_t size = 0;
    };

    // Returns base address and image size of the main executable module.
    ModuleRange mainModule()
    {
        auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));
        if (!base) return {};

        auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
        auto* nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dos->e_lfanew);

        return { base, nt->OptionalHeader.SizeOfImage };
    }

    // Checks whether the memory region described by mbi is committed and readable.
    bool isReadableRegion(const MEMORY_BASIC_INFORMATION& mbi)
    {
        constexpr DWORD kReadable = PAGE_READONLY | PAGE_READWRITE
                                | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE;

        return mbi.State == MEM_COMMIT
            && !(mbi.Protect & PAGE_GUARD)
            && (mbi.Protect & kReadable) != 0;
    }

    // Iterates over all committed readable regions in the module, calling visit.
    template <typename Visit>
    void forEachReadableRegion(const ModuleRange& mod, Visit&& visit)
    {
        uintptr_t addr = mod.base;

        while (addr < mod.base + mod.size) {
            MEMORY_BASIC_INFORMATION mbi;
            if (!VirtualQuery(reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi))) return;

            auto start = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            uintptr_t end = start + mbi.RegionSize;

            if (isReadableRegion(mbi) && visit(start, end)) return;
            addr = end;
        }
    }

} // namespace

// Parses an IDA-style pattern string into a vector of PatternByte tokens.
std::vector<crabe::memory::PatternByte> crabe::memory::parsePattern(const char* pattern)
{
    std::vector<PatternByte> bytes;
    if (!pattern) return bytes;

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

// Finds the first occurrence of needle in haystack, returning offset or kNoMatch.
size_t crabe::memory::findPattern(std::span<const uint8_t> haystack, std::span<const PatternByte> needle)
{
    if (needle.empty() || haystack.size() < needle.size()) return kNoMatch;

    size_t limit = haystack.size() - needle.size();

    for (size_t i = 0; i <= limit; ++i) {
        bool matched = true;

        for (size_t j = 0; j < needle.size(); ++j) {
            if (!needle[j].wildcard && haystack[i + j] != needle[j].value) {
                matched = false;
                break;
            }
        }
        if (matched) return i;
    }
    return kNoMatch;
}

// Parses pattern and finds its first occurrence in haystack.
size_t crabe::memory::findPattern(std::span<const uint8_t> haystack, const char* pattern)
{
    return findPattern(haystack, parsePattern(pattern));
}

// Scans committed readable module memory for the given IDA pattern.
uintptr_t crabe::memory::patternScan(const char* pattern, HMODULE module, uintptr_t after)
{
    ModuleRange mod = mainModule();
    if (module) mod.base = reinterpret_cast<uintptr_t>(module);
    if (!mod.base) return 0;

    std::vector<PatternByte> needle = parsePattern(pattern);
    if (needle.empty()) return 0;

    uintptr_t found = 0;

    forEachReadableRegion(mod, [&](uintptr_t start, uintptr_t end) {
        if (after >= end) return false;
        if (after >= start) start = after + 1;
        if (end - start < needle.size()) return false;

        size_t at = findPattern({reinterpret_cast<const uint8_t*>(start), end - start}, needle);
        if (at == kNoMatch) return false;

        found = start + at;
        return true;
    });

    return found;
}

// Checks whether a memory range is readable.
bool crabe::memory::isReadable(uintptr_t addr, size_t size)
{
    MEMORY_BASIC_INFORMATION mbi;

    if (!VirtualQuery(reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi))) return false;
    if (!isReadableRegion(mbi)) return false;

    auto end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    return addr + size <= end;
}

// Searches readable private memory regions for pointers holding the specified value.
std::vector<uintptr_t> crabe::memory::findPointers(uintptr_t value, size_t limit)
{
    std::vector<uintptr_t> found;
    if (!value) return found;

    SYSTEM_INFO info{};
    GetSystemInfo(&info);

    auto addr = reinterpret_cast<uintptr_t>(info.lpMinimumApplicationAddress);
    auto maxAddr = reinterpret_cast<uintptr_t>(info.lpMaximumApplicationAddress);

    while (addr < maxAddr && found.size() < limit) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi))) break;

        auto start = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        uintptr_t end = start + mbi.RegionSize;

        if (isReadableRegion(mbi) && mbi.Type == MEM_PRIVATE) {
            for (uintptr_t p = start; p + sizeof(uintptr_t) <= end; p += sizeof(uintptr_t)) {
                if (*reinterpret_cast<const uintptr_t*>(p) != value) continue;

                found.push_back(p);
                if (found.size() >= limit) break;
            }
        }

        addr = end;
    }

    return found;
}

// Searches readable module memory for a null-terminated string.
uintptr_t crabe::memory::findString(const char* text, uintptr_t after)
{
    ModuleRange mod = mainModule();
    if (!mod.base) return 0;

    size_t len = std::strlen(text) + 1;
    uintptr_t found = 0;

    forEachReadableRegion(mod, [&](uintptr_t start, uintptr_t end) {
        if (end - start < len) return false;
        if (after >= end) return false;
        if (after >= start) start = after + 1;

        for (uintptr_t p = start; p + len <= end; ++p) {
            if (std::memcmp(reinterpret_cast<const void*>(p), text, len) == 0) {
                found = p;
                return true;
            }
        }
        return false;
    });

    return found;
}

// Finds all registered function addresses matching the given name in readable memory.
std::vector<uintptr_t> crabe::memory::findRegisteredFunctions(const char* funcName)
{
    std::vector<uintptr_t> found;

    ModuleRange mod = mainModule();
    if (!mod.base) return found;

    for (uintptr_t nameAddr = findString(funcName); nameAddr;
        nameAddr = findString(funcName, nameAddr)) {

        forEachReadableRegion(mod, [&](uintptr_t start, uintptr_t end) {
            for (uintptr_t p = start; p + 2 * sizeof(uintptr_t) <= end; p += sizeof(uintptr_t)) {
                if (*reinterpret_cast<const uintptr_t*>(p) != nameAddr) continue;

                uintptr_t fn = *reinterpret_cast<const uintptr_t*>(p + sizeof(uintptr_t));
                if (fn <= mod.base || fn >= mod.base + mod.size || !isReadable(fn, 16)) continue;

                if (std::find(found.begin(), found.end(), fn) == found.end())
                    found.push_back(fn);
            }
            return false;
        });
    }

    return found;
}

// Finds the first registered function address matching the given name.
uintptr_t crabe::memory::findRegisteredFunction(const char* funcName)
{
    std::vector<uintptr_t> candidates = findRegisteredFunctions(funcName);
    return candidates.empty() ? 0 : candidates.front();
}

// Decodes a direct relative near call (0xE8) at the given address to find the target.
uintptr_t crabe::memory::resolveCall(uintptr_t addr)
{
    if (!isReadable(addr, 5)) return 0;
    if (*reinterpret_cast<const uint8_t*>(addr) != 0xE8) return 0;

    int32_t rel = *reinterpret_cast<const int32_t*>(addr + 1);
    return addr + 5 + static_cast<uintptr_t>(rel);
}

// Finds all call instructions in the module that target the specified function address.
std::vector<uintptr_t> crabe::memory::findCallSites(uintptr_t target, size_t limit)
{
    std::vector<uintptr_t> sites;

    ModuleRange mod = mainModule();
    if (!mod.base || !target) return sites;

    forEachReadableRegion(mod, [&](uintptr_t start, uintptr_t end) {
        if (end - start < 5) return false;

        const auto* bytes = reinterpret_cast<const uint8_t*>(start);
        size_t limitIndex = (end - start) - 5;

        for (size_t i = 0; i <= limitIndex; ++i) {
            if (bytes[i] != 0xE8) continue;

            auto rel = *reinterpret_cast<const int32_t*>(start + i + 1);
            uintptr_t callee = start + i + 5 + static_cast<uintptr_t>(rel);

            if (callee != target) continue;

            sites.push_back(start + i);
            if (sites.size() >= limit) return true;
        }
        return false;
    });

    return sites;
}

// Scans a function memory range for direct relative call instructions (0xE8).
std::vector<uintptr_t> crabe::memory::findCalls(uintptr_t functionStart, size_t maxScan)
{
    std::vector<uintptr_t> targets;

    ModuleRange mod = mainModule();
    if (!mod.base || !isReadable(functionStart, maxScan)) return targets;

    auto* code = reinterpret_cast<const uint8_t*>(functionStart);

    for (size_t i = 0; i + 5 <= maxScan; ) {
        if (code[i] != 0xE8) {
            ++i;
            continue;
        }

        uintptr_t target = resolveCall(functionStart + i);
        bool inImage = target >= mod.base && target < mod.base + mod.size;

        if (!inImage || !isReadable(target, 16)) {
            ++i;
            continue;
        }

        targets.push_back(target);
        i += 5;
    }
    return targets;
}

