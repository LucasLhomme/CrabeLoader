/*
** CrabeLoader
** File description:
** Implements PortalService:
** Coordinates hardware drivers, handles hotplug scanning, and drives the virtual portal.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/portal/portal_service.hpp"
#include "shared/logger.hpp"

namespace crabe::infrastructure::portal {

PortalService::PortalService() = default;
PortalService::~PortalService() {
    Shutdown();
}

PortalService& PortalService::Instance() {
    static PortalService s_instance;
    return s_instance;
}

void PortalService::Initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    LOG_INFO("PortalService: Initialisation du sous-système de portail");
    m_driver = CreatePortalDriver();
    if (m_driver && m_driver->IsConnected()) {
        LOG_INFO("PortalService: Détecté: %s", m_driver->GetTypeDescription());
    } else {
        LOG_INFO("PortalService: Aucun portail physique actif au démarrage");
    }
    m_lastScanTime = std::chrono::steady_clock::now();
}

void PortalService::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_driver) {
        m_driver->Close();
        m_driver.reset();
    }
    m_eventQueue.clear();
}

void PortalService::ScanForDevice() {
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - m_lastScanTime).count() < 3) {
        return;
    }
    m_lastScanTime = now;

    if (!m_driver || !m_driver->IsConnected()) {
        auto newDriver = CreatePortalDriver();
        if (newDriver && newDriver->IsConnected()) {
            LOG_INFO("PortalService: Nouveau portail détecté: %s", newDriver->GetTypeDescription());
            m_driver = std::move(newDriver);
        }
    }
}

void PortalService::Update() {
    std::lock_guard<std::mutex> lock(m_mutex);
    ScanForDevice();

    if (!m_driver || !m_driver->IsConnected()) {
        return;
    }

    std::vector<PortalTagEvent> events;
    m_driver->Poll(events);

    for (const auto& ev : events) {
        if (ev.type == PortalTagEvent::Type::Placed) {
            LOG_INFO("PortalService: Figurine posée sur slot %d (SKU: %u)", ev.slot, ev.sku);
            if (ev.slot == 1) m_slot1Sku = ev.sku;
            else if (ev.slot == 2) m_slot2Sku = ev.sku;
            else if (ev.slot == 3) m_slot3Sku = ev.sku;
        } else {
            LOG_INFO("PortalService: Figurine retirée du slot %d", ev.slot);
            if (ev.slot == 1) m_slot1Sku.reset();
            else if (ev.slot == 2) m_slot2Sku.reset();
            else if (ev.slot == 3) m_slot3Sku.reset();
        }
        m_eventQueue.push_back(ev);
    }
}

bool PortalService::IsConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_driver && m_driver->IsConnected();
}

PortalStatus PortalService::GetStatus() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    PortalStatus status{};
    status.slot1Sku = m_slot1Sku;
    status.slot2Sku = m_slot2Sku;
    status.slot3Sku = m_slot3Sku;

    if (m_driver && m_driver->IsConnected()) {
        status.connected = true;
        status.type = m_driver->GetType();
        status.typeDescription = m_driver->GetTypeDescription();

        if (status.type == PortalType::XboxOneGipLocked) {
            status.diagnosticMessage = "Portail Xbox One détecté (INF-8037059) — Matériellement verrouillé par puce de sécurité Microsoft GIP.";
        }
    } else {
        status.connected = false;
        status.type = PortalType::None;
        status.typeDescription = "Aucun portail physique";
    }

    return status;
}

bool PortalService::SetLed(uint8_t slot, uint8_t r, uint8_t g, uint8_t b) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_driver || !m_driver->IsConnected()) return false;
    return m_driver->SetLed(slot, r, g, b);
}

std::optional<PortalTagEvent> PortalService::PollEvent() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_eventQueue.empty()) return std::nullopt;

    PortalTagEvent ev = m_eventQueue.front();
    m_eventQueue.erase(m_eventQueue.begin());
    return ev;
}

void PortalService::EmulatePlaceFigure(uint8_t slot, uint32_t sku) {
    std::lock_guard<std::mutex> lock(m_mutex);
    LOG_INFO("PortalService [Virtuel]: Pose de la figurine SKU %u sur le slot %d", sku, slot);
    if (slot == 1) m_slot1Sku = sku;
    else if (slot == 2) m_slot2Sku = sku;
    else if (slot == 3) m_slot3Sku = sku;

    PortalTagEvent ev{};
    ev.type = PortalTagEvent::Type::Placed;
    ev.slot = slot;
    ev.sku = sku;
    m_eventQueue.push_back(ev);
}

void PortalService::EmulateRemoveFigure(uint8_t slot) {
    std::lock_guard<std::mutex> lock(m_mutex);
    LOG_INFO("PortalService [Virtuel]: Retrait de la figurine du slot %d", slot);
    if (slot == 1) m_slot1Sku.reset();
    else if (slot == 2) m_slot2Sku.reset();
    else if (slot == 3) m_slot3Sku.reset();

    PortalTagEvent ev{};
    ev.type = PortalTagEvent::Type::Removed;
    ev.slot = slot;
    ev.sku = 0;
    m_eventQueue.push_back(ev);
}

} // namespace crabe::infrastructure::portal
