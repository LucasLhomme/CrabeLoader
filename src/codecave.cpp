/*
** CrabeLoader
** File description:
** codecave
*/

#include "loader/codecave.hpp"

#include <cstring>
#include <windows.h>

#include "loader/memory.hpp"
#include "logger/logger.hpp"

namespace {

    // A relative jmp is 5 bytes, so a site has to give up at least that much.
    constexpr size_t kJmpLength = 5;

    // Nothing here needs a big cave; refuse anything that looks like a mistake
    // rather than allocating it.
    constexpr size_t kMaxStolen = 32;
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
    if (stolenLength < kJmpLength || stolenLength > kMaxStolen) return false;
    if (body.empty() || body.size() > kMaxBody) return false;
    if (!Memory::isReadable(site, stolenLength)) return false;

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
    Logger& logger = Logger::getInstance();

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
