#pragma once

#include "infrastructure/portal/portal_protocol.hpp"
#include "infrastructure/portal/portal_transport.hpp"

#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

namespace crabe::infrastructure::portal {

// Owns the background thread that finds a physical base, opens it, runs the Disney protocol on
// it and reconnects when it is unplugged. The game thread never blocks on the hardware: it only
// reads a snapshot, queues LED changes and drains figure events.
class PortalWorker final {
public:
    struct Snapshot {
        bool connected{false};
        PortalType type{PortalType::None};
        std::string description;
        std::string diagnostic;
    };

    PortalWorker() = default;
    ~PortalWorker();
    PortalWorker(const PortalWorker&) = delete;
    PortalWorker& operator=(const PortalWorker&) = delete;

    void Start();
    void Stop();

    // False when no base is connected (the change is dropped).
    bool SetLed(std::uint8_t slot, std::uint8_t r, std::uint8_t g, std::uint8_t b);
    // Queues saving progression onto the figure of a player slot (see DisneyBaseProtocol).
    bool WriteProgress(std::uint8_t slot, const FigureProgress& progress);
    [[nodiscard]] Snapshot GetSnapshot() const;
    [[nodiscard]] std::vector<PortalTagEvent> TakeEvents();

private:
    void Run(std::stop_token stop);
    void RunSession(IPortalTransport& transport, std::stop_token stop);
    void Publish(bool connected, PortalType type, std::string description, std::string diagnostic);
    static bool SleepOrStop(std::stop_token stop, std::chrono::milliseconds duration);

    mutable std::mutex m_mutex;
    DisneyBaseProtocol m_protocol;
    Snapshot m_snapshot;
    std::vector<PortalTagEvent> m_events;
    std::jthread m_thread;
};

} // namespace crabe::infrastructure::portal
