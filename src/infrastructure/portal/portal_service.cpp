/*
** CrabeLoader
** File description:
** Implements PortalService: game-thread view of the physical base (worker thread) and of the
** virtual portal, with one slot table and one event queue.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/portal/portal_service.hpp"
#include "shared/logger.hpp"

#include <algorithm>

namespace crabe::infrastructure::portal {

PortalService::~PortalService() {
    Shutdown();
}

PortalService& PortalService::Instance() {
    static PortalService s_instance;
    return s_instance;
}

void PortalService::Initialize() {
    if (m_started) return;
    crabe::shared::Logger::getInstance().info("PortalService: démarrage du sous-système de portail");
    m_worker.Start();
    m_started = true;
}

void PortalService::Shutdown() {
    if (!m_started) return;
    m_worker.Stop();
    m_started = false;
    std::lock_guard lock(m_mutex);
    m_events.clear();
}

void PortalService::Apply(const PortalTagEvent& event) {
    if (event.slot < 1 || event.slot > 3) return;
    if (event.type == PortalTagEvent::Type::Placed) m_slotSku[event.slot] = event.sku;
    else m_slotSku[event.slot].reset();
    m_events.push_back(event);
}

void PortalService::Update() {
    if (!m_started) return;
    const auto events = m_worker.TakeEvents();
    if (events.empty()) return;

    std::lock_guard lock(m_mutex);
    for (const auto& event : events) {
        crabe::shared::Logger::getInstance().info("PortalService: figurine {} sur le slot {} (SKU {})",
                                                  event.type == PortalTagEvent::Type::Placed ? "posée" : "retirée",
                                                  event.slot, event.sku);
        Apply(event);
    }
}

bool PortalService::IsConnected() const {
    return m_worker.GetSnapshot().connected;
}

PortalStatus PortalService::GetStatus() const {
    const auto snapshot = m_worker.GetSnapshot();

    PortalStatus status;
    status.connected = snapshot.connected;
    status.type = snapshot.type;
    status.typeDescription = snapshot.description.empty() ? "Aucun portail physique" : snapshot.description;
    status.diagnosticMessage = snapshot.diagnostic;

    std::lock_guard lock(m_mutex);
    status.slot1Sku = m_slotSku[1];
    status.slot2Sku = m_slotSku[2];
    status.slot3Sku = m_slotSku[3];
    return status;
}

bool PortalService::SetLed(std::uint8_t slot, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return m_worker.SetLed(slot, r, g, b);
}

bool PortalService::WriteProgress(std::uint8_t slot, const FigureProgress& progress) {
    return m_worker.WriteProgress(slot, progress);
}

std::optional<PortalTagEvent> PortalService::PollEvent() {
    std::lock_guard lock(m_mutex);
    if (m_events.empty()) return std::nullopt;
    const PortalTagEvent event = m_events.front();
    m_events.erase(m_events.begin());
    return event;
}

void PortalService::EmulatePlaceFigure(std::uint8_t slot, std::uint32_t sku) {
    crabe::shared::Logger::getInstance().info("PortalService [virtuel]: SKU {} posé sur le slot {}", sku, slot);
    PortalTagEvent event;
    event.type = PortalTagEvent::Type::Placed;
    event.slot = slot;
    event.sku = sku;
    std::lock_guard lock(m_mutex);
    Apply(event);
}

void PortalService::EmulateRemoveFigure(std::uint8_t slot) {
    crabe::shared::Logger::getInstance().info("PortalService [virtuel]: retrait du slot {}", slot);
    PortalTagEvent event;
    event.type = PortalTagEvent::Type::Removed;
    event.slot = slot;
    std::lock_guard lock(m_mutex);
    Apply(event);
}

} // namespace crabe::infrastructure::portal
