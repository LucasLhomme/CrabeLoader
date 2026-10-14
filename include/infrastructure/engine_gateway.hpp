/*
** CrabeLoader
** File description:
** Declares the bridge to the engine's own figure registry (Gateway), which the PC build still
** carries: tags added by UID, and the writes the game makes back to them, seen as events.
** Knows no gameplay: it adds and removes the tags it is given, Lua decides which and when.
**
** Authors: @LucasLhomme
*/

#ifndef ENGINE_GATEWAY_HPP_
#define ENGINE_GATEWAY_HPP_

#include <array>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <set>

#include "infrastructure/hook.hpp"

namespace crabe::infrastructure {

class EngineGateway final {
public:
    enum class Field : std::uint8_t { Experience, Level, LastPlayed, Action, Owner, Progression };

    struct TagWrite {
        std::uint64_t uid{0};
        Field field{Field::Experience};
        std::uint32_t value{0};
        std::array<std::uint8_t, 8> progression{};
    };

    struct TagData {
        std::uint32_t experience{0};
        std::uint8_t level{0};
        std::uint32_t lastPlayed{0};
        std::uint8_t action{0};
        std::uint8_t owner{0};
        std::optional<std::array<std::uint8_t, 8>> progression;
    };

    static EngineGateway& get();

    /// Resolves the entry points from the game profile and checks each one by its bytes.
    /// Then hooks the six UID-keyed Gateway::Write* wrappers the game saves a figure through.
    /// Leaves the bridge absent, with a warning, when anything does not match.
    void initialize();
    void uninitialize();

    /// Adds a tag to the registry the way a reader would, then sets its progression when given.
    /// The engine announces the tag to the game before this returns, so call it on the game thread.
    /// With `watch`, the game's later writes to this UID are queued for pollWrite.
    [[nodiscard]] bool addTag(std::uint64_t uid, std::uint32_t characterId, std::uint8_t antenna,
                              const TagData& data, bool watch);

    /// Removes a tag by UID and stops watching it.
    /// The engine announces the removal before this returns, so call it on the game thread.
    /// False when the bridge is absent or the call faulted.
    [[nodiscard]] bool removeTag(std::uint64_t uid);

    /// Character id of the virtual reader's current avatar tag, whose UID is that same id.
    /// Zero when the virtual reader holds no avatar, std::nullopt when the bridge is absent.
    [[nodiscard]] std::optional<std::uint32_t> virtualAvatarTag() const;

    /// Takes the oldest write the game made to a watched tag.
    /// Writes are queued in the order the game made them, at most 256 at a time.
    [[nodiscard]] std::optional<TagWrite> pollWrite();

    /// Name of a field as Lua sees it: experience, level, lastPlayed, action, owner or progression.
    [[nodiscard]] static const char* fieldName(Field field);

private:
    EngineGateway() = default;

    void resolveOnce();
    void record(std::uint64_t uid, Field field, std::uint32_t value, const std::uint8_t* progression);

    /// Detour of one Gateway::Write* wrapper: the engine writes first, then a watched UID is recorded.
    /// The wrappers are __thiscall(uid, value, flush) and return whether the tag took the value.
    /// Writes made by addTag itself are not recorded.
    template <Field F>
    static bool __fastcall hkWrite(void* gateway, void* edx, std::uint32_t uidLo, std::uint32_t uidHi,
                                   std::uint32_t value, std::uint32_t flush);

    bool _attempted{false};
    bool _resolved{false};
    std::uintptr_t _gatewayPtr{0};
    std::uintptr_t _virtualReaderPtr{0};
    std::uintptr_t _addTag{0};
    std::uintptr_t _removeTag{0};
    std::array<std::uintptr_t, 6> _writers{};
    std::array<Hook, 6> _writeHooks;

    mutable std::mutex _mutex;
    std::set<std::uint64_t> _watched;
    std::deque<TagWrite> _writes;
};

} // namespace crabe::infrastructure

#endif
