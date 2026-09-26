/*
** CrabeLoader
** File description:
** Installs a code cave: measures the stolen bytes, relocates them, then writes the relative jmp.
** The length limits live in the header because the measurement is a free function the tests assert.
** Registers nothing in the hook inventory; a cave is not a detour and has no trampoline.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/code_cave.hpp"

#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <windows.h>

#include "infrastructure/memory.hpp"
#include "minhook/hde/hde32.h"
#include "shared/logger.hpp"

namespace crabe::infrastructure {

namespace {

    // kJmpLength and kMaxStolen live in the header: the measurement they
    // constrain is a free function now, and the tests assert against them.
    constexpr size_t kMaxBody = 256;

    // Writes `E9 rel32` at `at`, jumping to `target`. rel32 is measured from
    // the end of the instruction, hence the -5.
    void writeJump(uint8_t* at, uintptr_t from, uintptr_t target)
    {
        auto delta = static_cast<int32_t>(target - (from + kJmpLength));

        at[0] = 0xE9;
        std::memcpy(at + 1, &delta, sizeof(delta));
    }

} // namespace

size_t measureStolenLength(const uint8_t* code, size_t* failedAtOffset)
{
    if (failedAtOffset) *failedAtOffset = 0;
    if (!code) return 0;

    size_t accumulated = 0;

    while (accumulated < kJmpLength) {
        hde32s hs{};
        unsigned int len = hde32_disasm(code + accumulated, &hs);

        if (len == 0 || (hs.flags & F_ERROR)) {
            if (failedAtOffset) *failedAtOffset = accumulated;
            return 0;
        }
        accumulated += len;
    }
    return accumulated;
}

uintptr_t acquireSharedBlock(std::string_view name, size_t size)
{
    struct Block {
        std::unique_ptr<uint8_t[]> bytes;
        size_t size = 0;
    };
    static std::mutex mutex;
    static std::unordered_map<std::string, Block> blocks;

    if (size == 0 || name.empty())
        return 0;

    std::lock_guard<std::mutex> lock(mutex);
    auto it = blocks.find(std::string(name));
    if (it == blocks.end())
        it = blocks.emplace(std::string(name), Block{ std::make_unique<uint8_t[]>(size), size }).first;
    if (size > it->second.size)
        return 0;
    return reinterpret_cast<uintptr_t>(it->second.bytes.get());
}

CodeCave::~CodeCave()
{
    remove();
}

void CodeCave::putU32(std::vector<uint8_t>& body, size_t at, uint32_t value)
{
    if (at + sizeof(value) > body.size()) return;
    std::memcpy(body.data() + at, &value, sizeof(value));
}

bool CodeCave::patchBytes(uintptr_t address, const void* bytes, size_t count)
{
    DWORD previous = 0;
    auto* target = reinterpret_cast<void*>(address);

    if (!VirtualProtect(target, count, PAGE_EXECUTE_READWRITE, &previous)) return false;
    std::memcpy(target, bytes, count);
    VirtualProtect(target, count, previous, &previous);
    FlushInstructionCache(GetCurrentProcess(), target, count);
    return true;
}

bool CodeCave::install(uintptr_t site, const std::vector<uint8_t>& body, size_t stolenLength)
{
    if (_installed || !site) return false;
    if (body.empty() || body.size() > kMaxBody) return false;

    if (stolenLength == 0) {
        if (!crabe::memory::isReadable(site, kJmpLength)) return false;

        size_t failedAt = 0;
        stolenLength = measureStolenLength(reinterpret_cast<const uint8_t*>(site), &failedAt);

        if (stolenLength == 0) {
            crabe::shared::Logger::getInstance().error("CodeCave: instruction disassembly failed at site 0x{:X} (offset +{}).",
                                        site, failedAt);
            return false;
        }
    }

    if (!isStolenLengthAcceptable(stolenLength)) return false;
    if (!crabe::memory::isReadable(site, stolenLength)) return false;

    size_t caveSize = body.size() + stolenLength + kJmpLength;
    auto* cave = static_cast<uint8_t*>(VirtualAlloc(nullptr, caveSize, MEM_COMMIT | MEM_RESERVE,
                                                    PAGE_EXECUTE_READWRITE));
    if (!cave) return false;

    _stolen.assign(reinterpret_cast<const uint8_t*>(site),
                   reinterpret_cast<const uint8_t*>(site) + stolenLength);

    size_t offset = 0;
    std::memcpy(cave + offset, body.data(), body.size());
    offset += body.size();
    std::memcpy(cave + offset, _stolen.data(), stolenLength);
    offset += stolenLength;
    writeJump(cave + offset, reinterpret_cast<uintptr_t>(cave + offset), site + stolenLength);

    // The site keeps its original length: jmp first, NOP padding after, so the
    // instruction boundary the rest of the function branches to stays put.
    std::vector<uint8_t> patch(stolenLength, 0x90);
    writeJump(patch.data(), site, reinterpret_cast<uintptr_t>(cave));

    if (!patchBytes(site, patch.data(), patch.size())) {
        VirtualFree(cave, 0, MEM_RELEASE);
        _stolen.clear();
        return false;
    }

    _site = site;
    _cave = cave;
    _installed = true;
    return true;
}

bool CodeCave::installLogged(uintptr_t site, const std::vector<uint8_t>& body, size_t stolenLength,
                             const char* owner, const char* name)
{
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

    if (site == 0) {
        logger.warning("{}: {} skipped (address not resolved).", owner, name);
        return false;
    }
    if (!install(site, body, stolenLength)) {
        logger.error("{}: failed to install cave for {} at 0x{:X}.", owner, name, site);
        return false;
    }
    logger.debug("{}: {} caved at 0x{:X} (cave 0x{:X}).", owner, name, site,
                 reinterpret_cast<uintptr_t>(_cave));
    return true;
}

void CodeCave::remove()
{
    if (!_installed) return;

    patchBytes(_site, _stolen.data(), _stolen.size());

    _site = 0;
    _cave = nullptr;
    _stolen.clear();
    _installed = false;
}

bool CodeCave::isInstalled() const
{
    return _installed;
}

uintptr_t CodeCave::caveAddress() const
{
    return reinterpret_cast<uintptr_t>(_cave);
}

} // namespace crabe::infrastructure

