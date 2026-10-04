/*
** CrabeLoader
** File description:
** Declares the bridge to engine primitives the shipped Lua cannot reach: a player camera's
** eye position, an actor's position, and FullKinematicStateData::Place, the setter that moves
** an actor. Knows no gameplay: it reads positions and places one actor, Lua decides when.
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

    /// World position of the actor behind `actorHandle`, read as KinematicStateGetActualPosition does.
    /// std::nullopt when the build lacks the entry points, the handle names no kinematic actor, or the read faulted.
    [[nodiscard]] std::optional<Position> actorPosition(std::uint32_t actorHandle);

    /// Places the actor behind `actorHandle` at `position` through the engine's own setter.
    /// False when the build lacks the entry points, the handle names no kinematic actor, or the call faulted.
    [[nodiscard]] bool placeActor(std::uint32_t actorHandle, const Position& position);

private:
    EngineKinematics() = default;

    void resolveOnce();
    bool resolvePlace(std::uintptr_t base);
    bool resolvePosition(std::uintptr_t base);
    [[nodiscard]] std::uintptr_t fullKinematicState(std::uint32_t actorHandle) const;
    bool resolveCamera(std::uintptr_t base);

    bool _attempted{false};
    std::uintptr_t _agentStateManager{0};
    std::uintptr_t _lookupState{0};
    std::uintptr_t _fullKinematicType{0};
    std::uintptr_t _place{0};
    std::uintptr_t _refreshState{0};
    std::uintptr_t _statePosition{0};
    std::uintptr_t _players{0};
};

} // namespace crabe::infrastructure

#endif
