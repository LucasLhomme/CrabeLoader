/*
** CrabeLoader
** File description:
** Finds Script VM native handlers by the name they register under, and checks code shapes.
** Shared by every bridge that reads engine entry points off those handlers.
** Calls nothing in the engine; it only reads the image.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/script_natives.hpp"

#include <cstdio>
#include <cstring>

#include "domain/game_profile.hpp"
#include "infrastructure/memory.hpp"

namespace crabe::infrastructure::script_natives {

std::uintptr_t find(const char* name)
{
    const std::size_t length = std::strlen(name);
    for (std::uintptr_t text = crabe::memory::findString(name); text; text = crabe::memory::findString(name, text)) {
        if (!crabe::memory::isReadable(text - 1, length + 2) || readAt<char>(text - 1) != '\0')
            continue;

        char pattern[32];
        std::snprintf(pattern, sizeof(pattern), "68 %02X %02X %02X %02X",
                      static_cast<unsigned>(text & 0xFF), static_cast<unsigned>((text >> 8) & 0xFF),
                      static_cast<unsigned>((text >> 16) & 0xFF), static_cast<unsigned>((text >> 24) & 0xFF));
        for (std::uintptr_t push = crabe::memory::patternScan(pattern); push;
             push = crabe::memory::patternScan(pattern, nullptr, push)) {
            if (readAt<std::uint8_t>(push - 5) == 0x68)
                return readAt<std::uint32_t>(push - 4);
        }
    }
    return 0;
}

bool bytesAt(std::uintptr_t address, std::initializer_list<std::uint8_t> expected)
{
    if (!crabe::memory::isReadable(address, expected.size()))
        return false;
    return std::memcmp(reinterpret_cast<const void*>(address), expected.begin(), expected.size()) == 0;
}

bool matchesProfile(std::uintptr_t base, const char* symbol, std::uintptr_t address)
{
    const crabe::domain::GameProfile* profile = crabe::domain::activeProfile();
    if (!profile)
        return true;
    const std::uint32_t rva = profile->engineSymbolRva(symbol);
    return rva != crabe::domain::kUnmeasured && base + rva == address;
}

} // namespace crabe::infrastructure::script_natives
