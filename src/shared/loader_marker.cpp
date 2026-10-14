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

    namespace {

        /// Reports whether some bytes hold a text as UTF-16.
        bool containsWideText(const void* bytes, std::size_t size, std::wstring_view wanted) noexcept
        {
            const std::size_t needle = wanted.size() * sizeof(wchar_t);
            if (!bytes || size < needle)
                return false;
            const auto* data = static_cast<const unsigned char*>(bytes);
            const auto* text = reinterpret_cast<const unsigned char*>(wanted.data());
            for (std::size_t offset = 0; offset + needle <= size; ++offset) {
                if (data[offset] == text[0] && std::memcmp(data + offset, text, needle) == 0)
                    return true;
            }
            return false;
        }

    } // namespace

    bool containsLoaderMarker(const void* bytes, std::size_t size) noexcept
    {
        return containsWideText(bytes, size, kLoaderDescription);
    }

    bool containsBootstrapMarker(const void* bytes, std::size_t size) noexcept
    {
        return containsWideText(bytes, size, kBootstrapDescription);
    }

} // namespace crabe::shared
