/*
** CrabeLoader
** File description:
** Value types shared by the portal subsystem: the 32-byte report, the base kind, figure events.
** Slots use the Lua-facing numbering: 1 = Player 1, 2 = Player 2, 3 = Playset / hexagon.
** FigureProgress mirrors the progression saved on a character figure, read from its newest copy.
**
** Authors: @LucasLhomme
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace crabe::infrastructure::portal {

inline constexpr std::size_t kReportSize = 32;
using Report = std::array<std::uint8_t, kReportSize>;

enum class PortalType {
    None,
    StandardHid,
    Xbox360,
    XboxOne
};

struct FigureProgress {
    std::uint32_t experience{0};
    std::uint8_t level{0};
    std::uint32_t lastPlayed{0};
    std::uint8_t actionFlags{0};
    std::array<std::uint8_t, 8> skillBits{};
};

struct PortalTagEvent {
    enum class Type { Placed, Removed } type{Type::Placed};
    std::uint8_t slot{0};
    std::uint32_t sku{0};
    std::uint64_t uid{0};
    std::optional<FigureProgress> progress;
};

struct PortalStatus {
    bool connected{false};
    PortalType type{PortalType::None};
    std::string typeDescription;
    std::optional<std::uint32_t> slot1Sku;
    std::optional<std::uint32_t> slot2Sku;
    std::optional<std::uint32_t> slot3Sku;
    std::string diagnosticMessage;
};

} // namespace crabe::infrastructure::portal
