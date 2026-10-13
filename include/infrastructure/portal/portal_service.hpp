#pragma once

#include "infrastructure/portal/portal_driver.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace crabe::infrastructure::portal {

class PortalService final {
public:
    PortalService();
    ~PortalService();

    PortalService(const PortalService&) = delete;
    PortalService& operator=(const PortalService&) = delete;
    PortalService(PortalService&&) = delete;
    PortalService& operator=(PortalService&&) = delete;

    static PortalService& Instance();

    void Initialize();
    void Shutdown();
    void Update();

    [[nodiscard]] bool IsConnected() const;
    [[nodiscard]] PortalStatus GetStatus() const;
    bool SetLed(uint8_t slot, uint8_t r, uint8_t g, uint8_t b);

    // Pops the next event from the queue (if any)
    [[nodiscard]] std::optional<PortalTagEvent> PollEvent();

    // Virtual Portal API (accessible from CrabeMenu to place/remove virtual figures)
    void EmulatePlaceFigure(uint8_t slot, uint32_t sku);
    void EmulateRemoveFigure(uint8_t slot);

private:
    void ScanForDevice();

    std::unique_ptr<IPortalDriver> m_driver;
    mutable std::mutex m_mutex;

    std::optional<uint32_t> m_slot1Sku;
    std::optional<uint32_t> m_slot2Sku;
    std::optional<uint32_t> m_slot3Sku;

    std::vector<PortalTagEvent> m_eventQueue;
    std::chrono::steady_clock::time_point m_lastScanTime;
};

} // namespace crabe::infrastructure::portal
