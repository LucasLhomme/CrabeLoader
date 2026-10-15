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

class EngineResolution final {
public:
    static EngineResolution& get();

    // Resolves resolution check functions and installs detours.
    void initialize();

    // Removes resolution detours.
    void uninitialize();

    // Returns whether resolution detours are installed.
    [[nodiscard]] bool isHooked() const noexcept;

    // Validates whether a width and height pair constitutes a plausible display mode.
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
    void* (__cdecl* _defaultTarget)() {nullptr};
};

} // namespace crabe::infrastructure

#endif // CRABELOADER_INFRASTRUCTURE_ENGINE_RESOLUTION_HPP_
