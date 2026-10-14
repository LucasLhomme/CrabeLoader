#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace crabe::infrastructure::portal {

// A Disney base talks in fixed 32-byte reports on every platform.
inline constexpr std::size_t kReportSize = 32;
using Report = std::array<std::uint8_t, kReportSize>;

enum class PortalType {
    None,
    StandardHid,   // PS3 / PS4 / Wii / Wii U / PC (VID 0x0E6F, PID 0x0129)
    Xbox360,       // Xbox 360 (VID 0x24C6, PID 0xFA00), through WinUSB
    XboxOne        // Xbox One INF-8037059 (VID 0x0E6F, PID 0x0229), GIP over WinUSB
};

// Progression saved on a character figure (sectors 1-2, two copies; the newest valid one).
// Read only: nothing is ever written back to the figure.
struct FigureProgress {
    std::uint32_t experience{0};
    std::uint8_t level{0};
    std::uint32_t lastPlayed{0};
    std::uint8_t actionFlags{0};                  // IGP action bits (bit 6 = max level)
    std::array<std::uint8_t, 8> skillBits{};      // bit n = skillBits[n / 8] & (1 << (n % 8))
};

// What the base reports for a figure, in the Lua-facing slot numbering.
// slot: 1 = Player 1, 2 = Player 2, 3 = Playset / hexagon.
struct PortalTagEvent {
    enum class Type { Placed, Removed } type{Type::Placed};
    std::uint8_t slot{0};
    std::uint32_t sku{0};   // identity block SKU (0 when unknown)
    std::uint64_t uid{0};   // 7-byte RFID UID, big-endian
    std::optional<FigureProgress> progress; // Placed only, character figures with saved data
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
