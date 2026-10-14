#pragma once

#include "infrastructure/portal/portal_types.hpp"

#include <memory>
#include <string>
#include <vector>

namespace crabe::infrastructure::portal {

// Moves 32-byte Disney reports between the host and one physical base. Blocking calls are
// bounded by a timeout so the owning worker thread can always be stopped. A transport is used
// from a single thread.
class IPortalTransport {
public:
    enum class ReadResult { Ok, Timeout, Lost };

    virtual ~IPortalTransport() = default;

    // Opens the device and performs whatever link-level session the hardware needs.
    // On failure, Diagnostic() explains why in a form fit for the player.
    [[nodiscard]] virtual bool Open() = 0;
    virtual void Close() = 0;

    [[nodiscard]] virtual bool Write(const Report& report) = 0;
    [[nodiscard]] virtual ReadResult Read(Report& report, unsigned timeoutMs) = 0;

    [[nodiscard]] virtual PortalType Type() const = 0;
    [[nodiscard]] virtual std::string Description() const = 0;
    [[nodiscard]] virtual std::string Diagnostic() const { return {}; }
    // Tag scan interval the engine uses on this platform (3 on Xbox One for FCC reasons, else 1).
    [[nodiscard]] virtual std::uint8_t RunningScanInterval() const { return 1; }
};

// What discovery found: transports ready to try, plus anything the player should know about
// (a base that is plugged in but cannot be opened, typically for a missing WinUSB driver).
struct DiscoveryResult {
    std::vector<std::unique_ptr<IPortalTransport>> transports;
    std::string diagnostic;
};

[[nodiscard]] DiscoveryResult DiscoverTransports();

} // namespace crabe::infrastructure::portal
