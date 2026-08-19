/*
** CrabeLoader
** File description:
** memory
*/

#include "loader/memory.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

    struct ModuleRange {
        uintptr_t base = 0;
        size_t size = 0;
    };

    ModuleRange mainModule()
    {
        auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));
        if (!base) return {};

        auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
        auto* nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dos->e_lfanew);

        return { base, nt->OptionalHeader.SizeOfImage };
    }

    bool isReadableRegion(const MEMORY_BASIC_INFORMATION& mbi)
    {
        constexpr DWORD kReadable = PAGE_READONLY | PAGE_READWRITE
                                | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE;

        return mbi.State == MEM_COMMIT
            && !(mbi.Protect & PAGE_GUARD)
            && (mbi.Protect & kReadable) != 0;
    }

    // Calls `visit(regionStart, regionEnd)` on every committed readable region of
    // the module, stopping as soon as it returns true. Skipping the other regions
    // is what keeps the scans below from faulting.
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

    // Size of the ModRM byte plus its optional SIB and displacement.
} // namespace

uintptr_t Memory::patternScan(const char* pattern, HMODULE module, uintptr_t after)
{
    ModuleRange mod = mainModule();
    if (module) mod.base = reinterpret_cast<uintptr_t>(module);
    if (!mod.base) return 0;

    std::vector<PatternByte> needle = parsePattern(pattern);
    if (needle.empty()) return 0;

    uintptr_t found = 0;

    forEachReadableRegion(mod, [&](uintptr_t start, uintptr_t end) {
        // Same skip rule as findString: entirely behind the cursor means
        // nothing to do here, partly behind means resume just past it.
        if (after >= end) return false;
        if (after >= start) start = after + 1;
        if (end - start < needle.size()) return false;

        auto* bytes = reinterpret_cast<const uint8_t*>(start);
        size_t limit = (end - start) - needle.size();

        for (size_t i = 0; i <= limit; ++i) {
            bool matched = true;

            for (size_t j = 0; j < needle.size(); ++j) {
                if (!needle[j].wildcard && bytes[i + j] != needle[j].value) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                found = start + i;
                return true;
            }
        }
        return false;
    });

    return found;
}

bool Memory::isReadable(uintptr_t addr, size_t size)
{
    MEMORY_BASIC_INFORMATION mbi;

    if (!VirtualQuery(reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi))) return false;
    if (!isReadableRegion(mbi)) return false;

    auto end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    return addr + size <= end;
}

std::vector<uintptr_t> Memory::findPointers(uintptr_t value, size_t limit)
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

uintptr_t Memory::findString(const char* text, uintptr_t after)
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

// A short name can be registered by more than one library ("type" by both
// base and io); collects every binding, since which comes first is not
// stable between runs.
std::vector<uintptr_t> Memory::findRegisteredFunctions(const char* funcName)
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

uintptr_t Memory::findRegisteredFunction(const char* funcName)
{
    std::vector<uintptr_t> candidates = findRegisteredFunctions(funcName);
    return candidates.empty() ? 0 : candidates.front();
}

uintptr_t Memory::resolveCall(uintptr_t addr)
{
    if (!isReadable(addr, 5)) return 0;
    if (*reinterpret_cast<const uint8_t*>(addr) != 0xE8) return 0;

    int32_t rel = *reinterpret_cast<const int32_t*>(addr + 1);
    return addr + 5 + static_cast<uintptr_t>(rel);
}

std::vector<uintptr_t> Memory::findCallSites(uintptr_t target, size_t limit)
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

// Plain byte walk, not a full decoder: an E8 byte also occurs inside other
// instructions, so a candidate only counts when its target lands inside the
// image -- merely readable isn't enough, that shifts every later index.
std::vector<uintptr_t> Memory::findCalls(uintptr_t functionStart, size_t maxScan)
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
