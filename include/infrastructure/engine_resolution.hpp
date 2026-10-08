/*
** CrabeLoader
** File description:
** Declares the two detours that let the engine accept display resolutions beyond its own
** hardcoded list of nine (960x540 up to 2560x1440): 4K UHD, ultrawide, 16:10, Steam Deck.
** Knows no gameplay: it only widens a display-mode validity check, nothing is written to disk.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_ENGINE_RESOLUTION_HPP_
#define CRABELOADER_INFRASTRUCTURE_ENGINE_RESOLUTION_HPP_

#include <cstddef>
#include <cstdint>

#include "infrastructure/hook.hpp"

namespace crabe::infrastructure {

/**
 * @brief Widens the engine's notion of a supported display resolution.
 *
 * Two engine functions gate every resolution, and both must be widened:
 *
 *  - IsResolutionSupported(w, h) (rva 0x459B0, __cdecl) is the filter the display-mode
 *    enumerator applies to each EnumDisplaySettings result. It only knows a static table
 *    of nine entries, so any other mode never reaches the engine's resolution list.
 *  - ResolutionList::Contains (rva 0x65A010, __thiscall) is asked at boot whether the
 *    resolution saved in the registry is in that list. When it is not, the engine clamps
 *    to the nearest listed one and overwrites the registry with it, which is why a 4K
 *    choice came back as 1440p after every restart.
 *
 * The static table cannot simply be extended in place: only one slot follows it before the
 * storage of an unrelated global object. Hooking the two checks leaves the table untouched.
 *
 * A third detour keeps the Scaleform UI usable on a screen that is not 16:9. The 3D scene
 * fills any back buffer, but Flash::Movie::SetViewport (rva 0x3F3C20) hands Scaleform the
 * whole buffer and the UI, authored for 16:9, then lands against the right edge (21:9) with
 * the left part empty. Full-screen movies are given a centred 16:9 rectangle instead.
 *
 * Installed from DllMain, before the engine's first boot read of the registry.
 */
class EngineResolution final {
public:
    static EngineResolution& get();

    /// Resolves both functions, checks their bytes, and installs the detours. Each one is
    /// all or nothing: an unrecognised build keeps the engine's own behaviour.
    void initialize();

    /// Removes the detours.
    void uninitialize();

    /// Whether both detours are installed.
    [[nodiscard]] bool isHooked() const noexcept;

    /// Pure range rule shared by both detours: true when a (width, height) pair is a
    /// plausible display mode. Symmetric in its arguments on purpose.
    [[nodiscard]] static bool isPlausibleMode(std::uint32_t first, std::uint32_t second) noexcept;

private:
    EngineResolution() = default;
    ~EngineResolution() = default;

    EngineResolution(const EngineResolution&) = delete;
    EngineResolution& operator=(const EngineResolution&) = delete;
    EngineResolution(EngineResolution&&) = delete;
    EngineResolution& operator=(EngineResolution&&) = delete;

    static std::uintptr_t resolve(const char* symbol, std::uintptr_t base, const char* pattern,
                                  std::size_t length);

    static bool __cdecl hkIsResolutionSupported(std::uint32_t width, std::uint32_t height);
    static void __fastcall hkMovieSetViewport(void* self, void* edx, float x, float y,
                                              float width, float height);
    static bool __fastcall hkResolutionListContains(void* self, void* edx,
                                                    const std::uint32_t* first,
                                                    const std::uint32_t* second);

    bool _attempted{false};
    Hook _supportedHook;
    Hook _containsHook;
    Hook _movieViewportHook;
    // Renderer::r_defaultTarget(), read off the SetViewport call it makes; null until resolved.
    void* (__cdecl* _defaultTarget)() {nullptr};
};

} // namespace crabe::infrastructure

#endif // CRABELOADER_INFRASTRUCTURE_ENGINE_RESOLUTION_HPP_
