#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace crabe::infrastructure::portal {

enum class PortalType {
    None,
    StandardHid,      // PS3 / PS4 / Wii / Wii U / PC (VID 0x0E6F, PID 0x0129)
    Xbox360,          // Xbox 360 (VID 0x24C6, PID 0xFA00)
    XboxOneGipLocked  // Xbox One INF-8037059 (VID 0x0E6F, PID 0x0229)
};

struct PortalTagEvent {
    enum class Type { Placed, Removed } type;
    uint8_t slot; // 1 = Player 1, 2 = Player 2, 3 = Playset / Hexagon
    uint32_t sku; // Decrypted Character SKU ID (e.g. 1000200 = Anakin)
};

struct PortalStatus {
    bool connected{false};
    PortalType type{PortalType::None};
    std::string typeDescription;
    std::optional<uint32_t> slot1Sku;
    std::optional<uint32_t> slot2Sku;
    std::optional<uint32_t> slot3Sku;
    std::string diagnosticMessage;
};

class IPortalDriver {
public:
    virtual ~IPortalDriver() = default;

    virtual bool Open() = 0;
    virtual void Close() = 0;
    [[nodiscard]] virtual bool IsConnected() const = 0;
    [[nodiscard]] virtual PortalType GetType() const = 0;
    [[nodiscard]] virtual const char* GetTypeDescription() const = 0;
    virtual bool SetLed(uint8_t slot, uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void Poll(std::vector<PortalTagEvent>& outEvents) = 0;
};

// Factory to scan and create the appropriate driver
[[nodiscard]] std::unique_ptr<IPortalDriver> CreatePortalDriver();

} // namespace crabe::infrastructure::portal
