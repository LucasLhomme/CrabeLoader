/*
** CrabeLoader
** File description:
** Declares the bridge to two engine primitives the shipped Lua cannot reach: a player
** camera's eye position, and FullKinematicStateData::Place, the setter that moves an actor.
** Knows no gameplay: it reads one position and places one actor, Lua decides when.
**
** Authors: @LucasLhomme
*/

#ifndef ENGINE_KINEMATICS_HPP_
#define ENGINE_KINEMATICS_HPP_

#include <array>
#include <cstdint>
#include <optional>

namespace crabe::infrastructure {

class EngineKinematics final {
public:
    using Position = std::array<float, 3>;

    static EngineKinematics& get();

    /// Eye position of `playerId`'s current camera, the free camera while it runs.
    /// std::nullopt when the build lacks the entry points, the player has no camera, or the read faulted.
    [[nodiscard]] std::optional<Position> cameraEye(int playerId);

    /// Places the actor behind `actorHandle` at `position` through the engine's own setter.
    /// False when the build lacks the entry points, the handle names no kinematic actor, or the call faulted.
    [[nodiscard]] bool placeActor(std::uint32_t actorHandle, const Position& position);

private:
    EngineKinematics() = default;

    void resolveOnce();
    bool resolvePlace(std::uintptr_t base);
    bool resolveCamera(std::uintptr_t base);

    bool _attempted{false};
    std::uintptr_t _agentStateManager{0};
    std::uintptr_t _lookupState{0};
    std::uintptr_t _fullKinematicType{0};
    std::uintptr_t _place{0};
    std::uintptr_t _players{0};
};

} // namespace crabe::infrastructure

#endif
