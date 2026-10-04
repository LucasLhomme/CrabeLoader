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

    /// Resolves the entry points and hooks CreateFreeCameras. Must run before the first
    /// world load, or that world's free camera is bound to a property set that is gone.
    void initialize();
    void uninitialize();

    /// Advances the engine free-camera state for `playerId` and returns whether it is now on.
    /// `skipNoControl` turns a second call into "off" rather than "on without control".
    /// std::nullopt when this build does not carry the entry points or the call faulted.
    [[nodiscard]] std::optional<bool> toggle(int playerId, bool skipNoControl);

    /// Whether `playerId`'s current camera is the engine free camera, read from the
    /// engine rather than remembered. std::nullopt when the player has no camera scene
    /// (front end, mid-load) or the read faulted.
    [[nodiscard]] std::optional<bool> isActive(int playerId) const;

    /// Counts the camera-scene sets the engine has built, one per world load. A free
    /// camera switched on under an older value died with the world it was flying in.
    [[nodiscard]] std::uint32_t sceneGeneration() const;

private:
    EngineFreeCamera() = default;

    bool resolve();

    /// Players::InitializeScenes rebuilds the camera-property-set manager on every world
    /// load, and its ids start over at 1. Only DeleteFreeCameras clears the free-camera id,
    /// and a world change never calls it, so the stale id would name one of the new world's
    /// own sets. Clearing it here makes the engine create a real free-camera set again.
    static void __fastcall hkCreateFreeCameras(void* scenes, void* edx);

    /// Name of `playerId`'s current camera, for the post-activation check.
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
