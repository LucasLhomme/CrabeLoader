/*
** CrabeLoader
** File description:
** Declares the bridge to actor creation, state bits and area damage through Script VM natives.
** Resolves engine entry points dynamically from script native wrappers without Rumpus editor.
** Knows no gameplay rules: it builds actors and flips states as requested, Lua decides.
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

    // Creates an actor from engine parameter string at position and heading.
    // Returns actor handle, or 0 on failure. Script thread only.
    [[nodiscard]] std::uint32_t createActor(const std::string& parameters, const Position& position, float heading);

    // Sets or clears the named ActorState bit on the actor behind the given handle.
    // Returns false if resolution fails, state is invalid, or the call faults.
    [[nodiscard]] bool setActorState(std::uint32_t actorHandle, const std::string& state, bool on);

    // Tests whether the actor carries the named ActorState bit.
    // Returns std::nullopt if the state cannot be read.
    [[nodiscard]] std::optional<bool> testActorState(std::uint32_t actorHandle, const std::string& state);

    // Deals damage of damageType to actors within radius of center, sparing exceptActorHandle.
    // Returns true if the call executed successfully. Script thread only.
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
