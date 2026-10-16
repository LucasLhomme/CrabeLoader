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
#include <map>
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

    // Resolves entry points and hooks figure write wrappers.
    void initialize();
    void uninitialize();

    // Adds a tag to the figure registry with optional progression and watch status.
    [[nodiscard]] bool addTag(std::uint64_t uid, std::uint32_t characterId, std::uint8_t antenna,
                              const TagData& data, bool watch);

    // Removes a tag by UID and stops watching it.
    [[nodiscard]] bool removeTag(std::uint64_t uid);

    // Returns the character ID of the virtual reader's current avatar tag, if present.
    [[nodiscard]] std::optional<std::uint32_t> virtualAvatarTag() const;

    // Retrieves and removes the oldest queued tag write event.
    [[nodiscard]] std::optional<TagWrite> pollWrite();

    // Returns the string name corresponding to a tag data field.
    [[nodiscard]] static const char* fieldName(Field field);

private:
    EngineGateway() = default;

    void resolveOnce();
    void record(std::uint64_t uid, Field field, std::uint32_t value, const std::uint8_t* progression);

    // Detour for Gateway write wrappers that records modifications on watched UIDs.
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
    std::map<std::uint8_t, std::uint64_t> _addedOnAntenna;
    std::deque<TagWrite> _writes;
};

} // namespace crabe::infrastructure

#endif
