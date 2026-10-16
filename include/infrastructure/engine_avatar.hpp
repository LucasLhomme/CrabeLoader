/*
** CrabeLoader
** File description:
** Declares the bridge to the engine's avatar leveling, which Lua only raises one level at a time.
** Calls AvatarLeveling::LevelUp with an absolute level, so a hero can also be set back down.
** Knows no gameplay rules: it sets the level it is asked for, Lua decides.
**
** Authors: @LucasLhomme
*/

#ifndef ENGINE_AVATAR_HPP_
#define ENGINE_AVATAR_HPP_

#include <cstdint>
#include <optional>

namespace crabe::infrastructure {

class EngineAvatar final {
public:
    static EngineAvatar& get();

    // Sets the player's avatar to an absolute level through the engine's own level-up path.
    // Returns the level the game reports afterwards, or std::nullopt if unresolved or faulted.
    [[nodiscard]] std::optional<int> setAvatarLevel(int playerId, std::uint32_t characterId, int level);

private:
    EngineAvatar() = default;

    void resolveOnce();

    bool _attempted{false};
    std::uintptr_t _flowAddress{0};
    std::uintptr_t _levelingAddress{0};
    std::uintptr_t _getLevel{0};
    std::uintptr_t _levelUp{0};
};

} // namespace crabe::infrastructure

#endif
