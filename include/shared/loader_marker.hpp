/*
** CrabeLoader
** File description:
** Names the text that identifies a CrabeLoader DLL: the file description of its version resource.
** Every release since 1.0.0 carries it, so the installer and a second copy recognise one.
** src/version.rc.in must keep the same description; the loader and the installer read this one.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_SHARED_LOADER_MARKER_HPP_
#define CRABELOADER_SHARED_LOADER_MARKER_HPP_

#include <cstddef>
#include <string_view>

namespace crabe::shared {

    constexpr std::wstring_view kLoaderDescription = L"CrabeLoader Proxy DLL for Disney Infinity 3.0";
    constexpr std::wstring_view kBootstrapDescription = L"CrabeLoader Bootstrap for Disney Infinity 3.0";

    /// Reports whether some bytes hold the loader description as UTF-16, as a version resource stores it.
    /// The bytes may be a whole DLL file or the version resource of a loaded module.
    /// A false positive would need another file to quote this exact sentence.
    [[nodiscard]] bool containsLoaderMarker(const void* bytes, std::size_t size) noexcept;

    /// Same, for the bootstrap dinput8.dll that loads CrabeLoader.asi. The two descriptions differ,
    /// so the bootstrap is never taken for a loader copy that would outrank CrabeLoader.asi.
    [[nodiscard]] bool containsBootstrapMarker(const void* bytes, std::size_t size) noexcept;

} // namespace crabe::shared

#endif /* !CRABELOADER_SHARED_LOADER_MARKER_HPP_ */
