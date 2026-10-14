/*
** CrabeLoader
** File description:
** Declares IPortalTransport, the link that moves 32-byte Disney reports to one physical base.
** Also declares the discovery entry point that lists the transports of the plugged-in bases.
** Each platform's base (HID, Xbox 360, Xbox One) has its own implementation.
**
** Authors: @LucasLhomme
*/

#pragma once

#include "infrastructure/portal/portal_types.hpp"

#include <memory>
#include <string>
#include <vector>

namespace crabe::infrastructure::portal {

class IPortalTransport {
public:
    enum class ReadResult { Ok, Timeout, Lost };

    virtual ~IPortalTransport() = default;

    // Opens the device and performs whatever link-level session the hardware needs.
    // On failure, Diagnostic() explains why in a form fit for the player.
    [[nodiscard]] virtual bool Open() = 0;
    virtual void Close() = 0;

    // Sends one 32-byte Disney report to the base, bounded by a write timeout.
    [[nodiscard]] virtual bool Write(const Report& report) = 0;
    // Waits at most timeoutMs for one report, so the owning worker thread can always be stopped.
    // A transport is used from a single thread.
    [[nodiscard]] virtual ReadResult Read(Report& report, unsigned timeoutMs) = 0;

    [[nodiscard]] virtual PortalType Type() const = 0;
    [[nodiscard]] virtual std::string Description() const = 0;
    [[nodiscard]] virtual std::string Diagnostic() const { return {}; }
    // Tag scan interval the engine uses on this platform (3 on Xbox One for FCC reasons, else 1).
    [[nodiscard]] virtual std::uint8_t RunningScanInterval() const { return 1; }
};

struct DiscoveryResult {
    std::vector<std::unique_ptr<IPortalTransport>> transports;
    std::string diagnostic;
};

// Returns a transport for every plugged-in base that can be tried. A base that is present but cannot
// be opened, typically for a missing WinUSB driver, leaves a player-facing message in diagnostic.
[[nodiscard]] DiscoveryResult DiscoverTransports();

} // namespace crabe::infrastructure::portal
