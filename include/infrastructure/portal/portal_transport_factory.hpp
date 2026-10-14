#pragma once

#include "infrastructure/portal/portal_transport.hpp"

#include <memory>

namespace crabe::infrastructure::portal {

// Xbox One base (INF-8037059): GIP over WinUSB with the v1 security handshake, implemented in
// portal_transport_gip.cpp.
[[nodiscard]] std::unique_ptr<IPortalTransport> MakeXboxOneTransport();

} // namespace crabe::infrastructure::portal
