/*
** CrabeLoader
** File description:
** Finds the CrabeLoader description in a block of bytes, the way a version resource stores it.
** Used by the loader to spot another copy of itself and by the installer to recognise its files.
** Reads nothing from disk; callers hand it the bytes.
**
** Authors: @LucasLhomme
*/

#include <cstddef>
#include <cstring>

#include "shared/loader_marker.hpp"

namespace crabe::shared {

    bool containsLoaderMarker(const void* bytes, std::size_t size) noexcept
    {
        const std::size_t needle = kLoaderDescription.size() * sizeof(wchar_t);
        if (!bytes || size < needle)
            return false;
        const auto* data = static_cast<const unsigned char*>(bytes);
        const auto* text = reinterpret_cast<const unsigned char*>(kLoaderDescription.data());
        for (std::size_t offset = 0; offset + needle <= size; ++offset) {
            if (data[offset] == text[0] && std::memcmp(data + offset, text, needle) == 0)
                return true;
        }
        return false;
    }

} // namespace crabe::shared
