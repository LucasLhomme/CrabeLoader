/*
** CrabeLoader
** File description:
** Declares the bridge to the engine's own free camera, compiled into the shipped game.
** Knows no gameplay: it forwards one call, and Lua decides when to make it.
** The camera's flight and controls belong to the engine, not to this file.
**
** Authors: @LucasLhomme
*/

#ifndef ENGINE_FREE_CAMERA_HPP_
#define ENGINE_FREE_CAMERA_HPP_

#include <atomic>
#include <cstdint>
#include <optional>

#include "infrastructure/hook.hpp"

namespace crabe::infrastructure {

class EngineFreeCamera final {
public:
    static EngineFreeCamera& get();

    // Resolves entry points and hooks CreateFreeCameras before first world load.
    void initialize();
    void uninitialize();

    // Advances the engine free-camera state for the specified player and returns new state.
    [[nodiscard]] std::optional<bool> toggle(int playerId, bool skipNoControl);

    // Returns whether the player's active camera is currently the engine free camera.
    [[nodiscard]] std::optional<bool> isActive(int playerId) const;

    // Returns the camera scene generation counter incremented per world load.
    [[nodiscard]] std::uint32_t sceneGeneration() const;

private:
    EngineFreeCamera() = default;

    bool resolve();

    // Hook on CreateFreeCameras to clear stale camera property set IDs.
    static void __fastcall hkCreateFreeCameras(void* scenes, void* edx);

    // Returns the name of the specified player's current camera.
    [[nodiscard]] const char* currentCameraName(int playerId) const;

    bool _attempted{false};
    std::uintptr_t _activate{0};
    std::uintptr_t _createFreeCameras{0};
    std::uintptr_t _scenes{0};
    std::uintptr_t _sceneHookSlot{0};
    std::atomic<std::uint32_t> _generation{0};
    Hook _createHook;
};

} // namespace crabe::infrastructure

#endif
