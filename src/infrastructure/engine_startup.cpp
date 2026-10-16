/*
** CrabeLoader
** File description:
** Implements the startup logo skip. PlayStartupFMVS loops over seven logo slots.
** Replaces the jump table dispatch with a direct jump past movie playback.
** The patch leaves the rest of the function, including input setup, untouched.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/engine_startup.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <windows.h>

#include "domain/game_profile.hpp"
#include "infrastructure/code_cave.hpp"
#include "infrastructure/memory.hpp"
#include "shared/logger.hpp"

namespace {

    constexpr const char* kDispatchSymbol = "StartupLogos::Dispatch";
    constexpr std::size_t kJumpOffset = 9;
    constexpr std::size_t kTargetOffset = 0xB3;

    constexpr std::array<std::uint8_t, 12> kDispatchShape = {
        0x83, 0xFF, 0x06, 0x0F, 0x87, 0x8C, 0x00, 0x00, 0x00, 0xFF, 0x24, 0xBD,
    };
    constexpr std::array<std::uint8_t, 11> kTargetShape = {
        0x8D, 0x4C, 0x24, 0x10, 0x88, 0x9C, 0x24, 0xAC, 0x00, 0x00, 0x00,
    };
    constexpr std::array<std::uint8_t, 7> kPatched = {
        0xE9, static_cast<std::uint8_t>(kTargetOffset - 5), 0x00, 0x00, 0x00, 0x90, 0x90,
    };

    bool matches(std::uintptr_t address, const std::uint8_t* expected, std::size_t length)
    {
        return crabe::memory::isReadable(address, length)
            && std::memcmp(reinterpret_cast<const void*>(address), expected, length) == 0;
    }

} // namespace

namespace crabe::infrastructure {

EngineStartup& EngineStartup::get()
{
    static EngineStartup instance;
    return instance;
}

bool EngineStartup::skipLogos()
{
    if (_applied)
        return true;

    auto& logger = crabe::shared::Logger::getInstance();
    const auto* profile = crabe::domain::activeProfile();
    const std::uint32_t rva = profile ? profile->engineSymbolRva(kDispatchSymbol) : crabe::domain::kUnmeasured;
    if (rva == crabe::domain::kUnmeasured) {
        logger.warning("EngineStartup: no measured profile for the logo loop; the startup logos stay.");
        return false;
    }

    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    const std::uintptr_t site = base + rva;
    const std::uintptr_t jump = site + kJumpOffset;
    if (matches(jump, kPatched.data(), kPatched.size())) {
        _applied = true;
        return true;
    }

    if (!matches(site, kDispatchShape.data(), kDispatchShape.size())
        || !matches(jump + kTargetOffset, kTargetShape.data(), kTargetShape.size())) {
        logger.error("EngineStartup: the logo loop at rva 0x{:X} does not have the measured shape; refusing.", rva);
        return false;
    }

    if (!CodeCave::patchBytes(jump, kPatched.data(), kPatched.size())) {
        logger.error("EngineStartup: could not write the logo skip at rva 0x{:X}.", rva + kJumpOffset);
        return false;
    }

    _applied = true;
    logger.info("EngineStartup: startup logo movies skipped (rva 0x{:X}).", rva + kJumpOffset);
    return true;
}

} // namespace crabe::infrastructure
