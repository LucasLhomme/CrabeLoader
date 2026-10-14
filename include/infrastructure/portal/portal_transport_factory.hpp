/*
** CrabeLoader
** File description:
** Declares the factory of the Xbox One base transport.
** The implementation lives in portal_transport_gip.cpp and stays hidden behind IPortalTransport.
** Discovery calls it once the base is found with the WinUSB driver.
**
** Authors: @LucasLhomme
*/

#pragma once

#include "infrastructure/portal/portal_transport.hpp"

#include <memory>

namespace crabe::infrastructure::portal {

// Creates the Xbox One base transport (INF-8037059): GIP over WinUSB with the v1 security handshake.
[[nodiscard]] std::unique_ptr<IPortalTransport> MakeXboxOneTransport();

} // namespace crabe::infrastructure::portal
