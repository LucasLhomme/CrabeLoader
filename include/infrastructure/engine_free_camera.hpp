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

#include <cstdint>
#include <optional>

namespace crabe::infrastructure {

class EngineFreeCamera final {
public:
    static EngineFreeCamera& get();

    /// Advances the engine free-camera state for `playerId` and returns whether it is now on.
    /// `skipNoControl` turns a second call into "off" rather than "on without control".
    /// std::nullopt when this build does not carry the entry points or the call faulted.
    [[nodiscard]] std::optional<bool> toggle(int playerId, bool skipNoControl);

private:
    EngineFreeCamera() = default;

    bool resolve();
    void resolveSceneHelpers();

    /// Drops a free-camera entry left on the scene stack after a world change. The engine
    /// reads it as "already on" and does nothing, so without this the camera never comes back.
    void dropStaleEntry(int playerId) const;

    /// Binds the scene's FreeCam to the free-camera property set. After some world changes
    /// the set has no camera for the new scene, so a push lands on the stack yet selects
    /// nothing and the view stays on the game camera. AddCamera is a no-op when the binding
    /// exists and refuses a camera of the wrong type, so calling it every time is safe.
    void rebindFreeCamera(int playerId) const;

    /// Name of `playerId`'s current camera, for the post-activation check.
    [[nodiscard]] const char* currentCameraName(int playerId) const;

    bool _attempted{false};
    std::uintptr_t _activate{0};
    std::uintptr_t _scenes{0};
    std::uintptr_t _sceneHookSlot{0};
    std::uintptr_t _findCamera{0};
    std::uintptr_t _isOnStack{0};
    std::uintptr_t _removeFromStack{0};
    std::uintptr_t _findPropertySet{0};
    std::uintptr_t _addCamera{0};
};

} // namespace crabe::infrastructure

#endif
