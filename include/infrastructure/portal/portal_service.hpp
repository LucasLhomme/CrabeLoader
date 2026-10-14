#pragma once

#include "infrastructure/portal/portal_types.hpp"
#include "infrastructure/portal/portal_worker.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

namespace crabe::infrastructure::portal {

// Game-thread facade of the portal subsystem: physical base (through the worker thread) and
// virtual figures placed from Lua share one slot table and one event queue.
class PortalService final {
public:
    PortalService() = default;
    ~PortalService();

    PortalService(const PortalService&) = delete;
    PortalService& operator=(const PortalService&) = delete;
    PortalService(PortalService&&) = delete;
    PortalService& operator=(PortalService&&) = delete;

    static PortalService& Instance();

    void Initialize();
    void Shutdown();
    // Once per tick: moves figure events from the worker into the slot table and the queue.
    void Update();

    [[nodiscard]] bool IsConnected() const;
    [[nodiscard]] PortalStatus GetStatus() const;
    bool SetLed(std::uint8_t slot, std::uint8_t r, std::uint8_t g, std::uint8_t b);
    // Saves progression onto the physical figure of a player slot; false when nothing was queued.
    bool WriteProgress(std::uint8_t slot, const FigureProgress& progress);

    // Pops the next event from the queue (if any).
    [[nodiscard]] std::optional<PortalTagEvent> PollEvent();

    // Virtual portal: place or remove a figure without hardware.
    void EmulatePlaceFigure(std::uint8_t slot, std::uint32_t sku);
    void EmulateRemoveFigure(std::uint8_t slot);

private:
    void Apply(const PortalTagEvent& event);

    PortalWorker m_worker;
    mutable std::mutex m_mutex;
    std::optional<std::uint32_t> m_slotSku[4]; // index 1..3
    std::vector<PortalTagEvent> m_events;
    bool m_started{false};
};

} // namespace crabe::infrastructure::portal
