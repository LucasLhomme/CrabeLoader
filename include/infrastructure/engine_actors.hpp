/*
** CrabeLoader
** File description:
** Declares the bridge to CActorCreator::CreateActor, the engine's own way to build an actor
** from a parameter string, and to an actor's named ActorState bits (its combat team among
** them), and to ActorCommands::DamageRadius, the engine's area damage; all reached through the
** Script VM natives that wrap them.
** Knows no gameplay: it creates actors, flips states and deals damage where it is told, Lua decides.
**
** Authors: @LucasLhomme
*/

#ifndef ENGINE_ACTORS_HPP_
#define ENGINE_ACTORS_HPP_

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace crabe::infrastructure {

class EngineActors final {
public:
    using Position = std::array<float, 3>;

    static EngineActors& get();

    /// Creates an actor from an engine parameter string ("DNAFile=characters/X.dnax", or
    /// "CreationKey=x" for an actor list entry of the current world) at `position`, facing
    /// `heading` radians. Returns its actor handle, or 0 when the build lacks the entry
    /// points, the engine built nothing, or the call faulted. Script thread only.
    [[nodiscard]] std::uint32_t createActor(const std::string& parameters, const Position& position, float heading);

    /// Sets (`on`) or clears the ActorState bit named `state` ("CombatTeam2"...) on the actor
    /// behind `actorHandle`. False when the build lacks the entry points, the name is no
    /// ActorState, the actor has no state data, or the call faulted.
    [[nodiscard]] bool setActorState(std::uint32_t actorHandle, const std::string& state, bool on);

    /// Whether the actor behind `actorHandle` carries the ActorState bit named `state`.
    /// std::nullopt when it cannot be read.
    [[nodiscard]] std::optional<bool> testActorState(std::uint32_t actorHandle, const std::string& state);

    /// Deals `damage` of the engine damage type `damageType` ("damageExplosive", "damageNormal",
    /// "damageSpecial", or a four-character code) to every actor in the sphere of `radius` around
    /// `center`, the way a script's DamageRadiusExceptActor does, from no source and no owner.
    /// The actor behind `exceptActorHandle` (0 for none) is spared. True when the call ran; the
    /// victims' own health, armour and team rules decide what it did. Script thread only.
    [[nodiscard]] bool damageRadius(const Position& center, float radius, float damage,
                                    const std::string& damageType, std::uint32_t exceptActorHandle);

private:
    EngineActors() = default;

    void resolveOnce();
    bool resolveCreate(std::uintptr_t base);
    bool resolveStates(std::uintptr_t base);
    bool resolveDamage(std::uintptr_t base);
    [[nodiscard]] const void* bitMask(const std::string& state) const;
    [[nodiscard]] void* stateData(std::uint32_t actorHandle) const;

    bool _attempted{false};
    std::uintptr_t _parameterSetConstruct{0};
    std::uintptr_t _parameterSetDestruct{0};
    std::uintptr_t _createActor{0};

    std::uintptr_t _bitMaskManager{0};
    std::uintptr_t _actorStateCategory{0};
    std::uintptr_t _getBitMask{0};
    std::uintptr_t _stateDataType{0};
    std::uintptr_t _getAgentData{0};
    std::uintptr_t _addState{0};
    std::uintptr_t _removeState{0};
    std::uintptr_t _filterAny{0};

    std::uintptr_t _damageRadius{0};
    std::uintptr_t _actorFromHandle{0};
};

} // namespace crabe::infrastructure

#endif
