/*
** CrabeLoader
** File description:
** Declares PortalWorker, the background thread that finds a physical base, opens it,
** runs the Disney protocol on it and reconnects when it is unplugged.
** The game thread never blocks on hardware: it reads a snapshot, queues requests, drains events.
**
** Authors: @LucasLhomme
*/

#pragma once

#include "infrastructure/portal/portal_protocol.hpp"
#include "infrastructure/portal/portal_transport.hpp"

#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

namespace crabe::infrastructure::portal {

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

    // Queues an LED change; false when no base is connected (the change is dropped).
    bool SetLed(std::uint8_t slot, std::uint8_t r, std::uint8_t g, std::uint8_t b);

    // Queues saving progression onto the figure of a player slot (see DisneyBaseProtocol::WriteProgress).
    // False when no base is connected or nothing was queued.
    bool WriteProgress(std::uint8_t slot, const FigureProgress& progress);
    [[nodiscard]] Snapshot GetSnapshot() const;
    [[nodiscard]] std::vector<PortalTagEvent> TakeEvents();

private:
    // Discovery loop: opens the first transport that answers, runs its session, then rescans after a delay.
    void Run(std::stop_token stop);

    // Runs the protocol on one opened transport until stop, a wake timeout, repeated write failures or
    // a lost read. Every reported figure gets a Removed event when the session ends.
    void RunSession(IPortalTransport& transport, std::stop_token stop);
    void Publish(bool connected, PortalType type, std::string description, std::string diagnostic);

    // Sleeps in 50 ms steps; false as soon as a stop is requested.
    static bool SleepOrStop(std::stop_token stop, std::chrono::milliseconds duration);

    mutable std::mutex m_mutex;
    DisneyBaseProtocol m_protocol;
    Snapshot m_snapshot;
    std::vector<PortalTagEvent> m_events;
    std::jthread m_thread;
};

} // namespace crabe::infrastructure::portal
