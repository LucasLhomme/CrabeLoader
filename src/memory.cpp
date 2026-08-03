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

    // Minimal x86 instruction sizer, whitelist-based: anything not covered
    // (notably relative call/jump) returns 0, so prologueLength() refuses
    // rather than building a trampoline that jumps to the wrong place.

    // Size of the ModRM byte plus its optional SIB and displacement.
    size_t modrmLength(const uint8_t* p)
    {
        uint8_t mod = static_cast<uint8_t>(p[0] >> 6);
        uint8_t rm = static_cast<uint8_t>(p[0] & 7);
        size_t len = 1;

        if (mod != 3 && rm == 4) len += 1;              // SIB byte
        if (mod == 1) len += 1;                         // disp8
        else if (mod == 2) len += 4;                    // disp32
        else if (mod == 0 && rm == 5) len += 4;         // absolute disp32

        return len;
    }

    size_t instructionLength(const uint8_t* p)
    {
        switch (p[0]) {
            case 0x64:                                  // FS segment prefix
                return 1 + instructionLength(p + 1);

            case 0x50: case 0x51: case 0x52: case 0x53: // push r32
            case 0x54: case 0x55: case 0x56: case 0x57:
            case 0x58: case 0x59: case 0x5A: case 0x5B: // pop r32
            case 0x5C: case 0x5D: case 0x5E: case 0x5F:
            case 0x90:                                  // nop
                return 1;

            case 0x6A:                                  // push imm8
                return 2;

            case 0x68:                                  // push imm32
            case 0xA1: case 0xA3:                       // mov eax, moffs32
            case 0xB8: case 0xB9: case 0xBA: case 0xBB: // mov r32, imm32
            case 0xBC: case 0xBD: case 0xBE: case 0xBF:
                return 5;

            case 0x03: case 0x2B: case 0x31: case 0x33: // add/sub/xor r32
            case 0x39: case 0x3B: case 0x85:            // cmp / test
            case 0x88: case 0x8A: case 0x89: case 0x8B: // mov
            case 0x8D:                                  // lea
                return 1 + modrmLength(p + 1);

            case 0x83:                                  // group1 r/m32, imm8
            case 0xC6:                                  // mov r/m8, imm8
                return 1 + modrmLength(p + 1) + 1;

            case 0x81:                                  // group1 r/m32, imm32
            case 0xC7:                                  // mov r/m32, imm32
                return 1 + modrmLength(p + 1) + 4;

            default:
                return 0;                               // unknown: refuse
        }
    }

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

        // MEM_PRIVATE only: the heap, where a live object's vtable pointer
        // sits. Skipping MEM_IMAGE/MEM_MAPPED keeps this scan in milliseconds.
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

    size_t len = std::strlen(text) + 1;   // the NUL is part of the match
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

std::vector<uintptr_t> Memory::findRegisteredFunctions(const char* funcName)
{
    std::vector<uintptr_t> found;

    ModuleRange mod = mainModule();
    if (!mod.base) return found;

    // A short name can be registered by more than one library ("type" by both
    // base and io); collect every binding, since which comes first is not
    // stable between runs.
    for (uintptr_t nameAddr = findString(funcName); nameAddr;
        nameAddr = findString(funcName, nameAddr)) {

        // Look for a pointer to that string: the pointer right after it in the
        // registration table is the C function bound to the name.
        forEachReadableRegion(mod, [&](uintptr_t start, uintptr_t end) {
            for (uintptr_t p = start; p + 2 * sizeof(uintptr_t) <= end; p += sizeof(uintptr_t)) {
                if (*reinterpret_cast<const uintptr_t*>(p) != nameAddr) continue;

                uintptr_t fn = *reinterpret_cast<const uintptr_t*>(p + sizeof(uintptr_t));
                if (fn <= mod.base || fn >= mod.base + mod.size || !isReadable(fn, 16)) continue;

                if (std::find(found.begin(), found.end(), fn) == found.end())
                    found.push_back(fn);
            }
            return false; // every region, not just the first with a hit
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

            // Resolve in place rather than via resolveCall(): that would
            // re-run VirtualQuery for every one of the millions of stray E8
            // bytes in the image, which turns this scan into minutes.
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

std::vector<uintptr_t> Memory::findCalls(uintptr_t functionStart, size_t maxScan)
{
    std::vector<uintptr_t> targets;

    ModuleRange mod = mainModule();
    if (!mod.base || !isReadable(functionStart, maxScan)) return targets;

    auto* code = reinterpret_cast<const uint8_t*>(functionStart);

    // Plain byte walk, not a full decoder: an E8 byte also occurs inside other
    // instructions, so a candidate only counts when its target lands inside
    // the image -- merely readable isn't enough, that shifts every later index.
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

uintptr_t Memory::findNthCall(uintptr_t functionStart, int n, size_t maxScan)
{
    if (n <= 0) return 0;

    std::vector<uintptr_t> targets = findCalls(functionStart, maxScan);
    if (static_cast<size_t>(n) > targets.size()) return 0;

    return targets[static_cast<size_t>(n) - 1];
}

size_t Memory::prologueLength(uintptr_t addr, size_t minLen)
{
    if (!isReadable(addr, minLen + 16)) return 0;

    auto* code = reinterpret_cast<const uint8_t*>(addr);
    size_t total = 0;

    while (total < minLen) {
        size_t len = instructionLength(code + total);
        if (len == 0) return 0;
        total += len;
    }
    return total;
}
