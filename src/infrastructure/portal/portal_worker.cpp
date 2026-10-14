/*
** CrabeLoader
** File description:
** Background session thread of the physical portal: discovery, open, protocol loop, reconnect.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/portal/portal_worker.hpp"
#include "shared/logger.hpp"

#include <algorithm>

namespace crabe::infrastructure::portal {

namespace {

using Clock = std::chrono::steady_clock;

constexpr auto kRescanDelay = std::chrono::milliseconds(2000);
constexpr auto kWakeDeadline = std::chrono::seconds(15);
constexpr unsigned kReadSliceMs = 15;
constexpr int kMaxWriteFailures = 3;

void LogLine(std::string_view message) {
    crabe::shared::Logger::getInstance().info("{}", message);
}

} // namespace

PortalWorker::~PortalWorker() {
    Stop();
}

void PortalWorker::Start() {
    if (m_thread.joinable()) return;
    m_thread = std::jthread([this](std::stop_token stop) { Run(stop); });
}

void PortalWorker::Stop() {
    if (!m_thread.joinable()) return;
    m_thread.request_stop();
    m_thread.join();
}

bool PortalWorker::SetLed(std::uint8_t slot, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    std::lock_guard lock(m_mutex);
    if (!m_snapshot.connected) return false;
    m_protocol.SetLed(slot, r, g, b);
    return true;
}

bool PortalWorker::WriteProgress(std::uint8_t slot, const FigureProgress& progress) {
    std::lock_guard lock(m_mutex);
    if (!m_snapshot.connected) return false;
    return m_protocol.WriteProgress(slot, progress);
}

PortalWorker::Snapshot PortalWorker::GetSnapshot() const {
    std::lock_guard lock(m_mutex);
    return m_snapshot;
}

std::vector<PortalTagEvent> PortalWorker::TakeEvents() {
    std::lock_guard lock(m_mutex);
    std::vector<PortalTagEvent> events;
    events.swap(m_events);
    return events;
}

void PortalWorker::Publish(bool connected, PortalType type, std::string description, std::string diagnostic) {
    std::lock_guard lock(m_mutex);
    m_snapshot.connected = connected;
    m_snapshot.type = type;
    m_snapshot.description = std::move(description);
    m_snapshot.diagnostic = std::move(diagnostic);
}

bool PortalWorker::SleepOrStop(std::stop_token stop, std::chrono::milliseconds duration) {
    const auto end = Clock::now() + duration;
    while (Clock::now() < end) {
        if (stop.stop_requested()) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return !stop.stop_requested();
}

void PortalWorker::Run(std::stop_token stop) {
    std::string lastDiagnostic;
    while (!stop.stop_requested()) {
        DiscoveryResult found = DiscoverTransports();
        std::string diagnostic = found.diagnostic;

        for (auto& transport : found.transports) {
            if (stop.stop_requested()) break;

            if (!transport->Open()) {
                diagnostic = transport->Diagnostic();
                transport->Close();
                continue;
            }

            crabe::shared::Logger::getInstance().info("PortalWorker: portail ouvert: {}", transport->Description());
            Publish(true, transport->Type(), transport->Description(), {});
            RunSession(*transport, stop);
            transport->Close();
            Publish(false, PortalType::None, "Aucun portail physique", "Portail déconnecté.");
            crabe::shared::Logger::getInstance().info("PortalWorker: session terminée");
            diagnostic = "Portail déconnecté.";
            break;
        }

        if (diagnostic.empty()) diagnostic = "Aucun portail physique détecté.";
        if (diagnostic != lastDiagnostic) {
            crabe::shared::Logger::getInstance().info("PortalWorker: {}", diagnostic);
            lastDiagnostic = diagnostic;
        }
        {
            std::lock_guard lock(m_mutex);
            if (!m_snapshot.connected) {
                m_snapshot.type = PortalType::None;
                m_snapshot.description = "Aucun portail physique";
                m_snapshot.diagnostic = diagnostic;
            }
        }

        if (!SleepOrStop(stop, kRescanDelay)) break;
    }
}

void PortalWorker::RunSession(IPortalTransport& transport, std::stop_token stop) {
    {
        std::lock_guard lock(m_mutex);
        m_protocol = DisneyBaseProtocol(DisneyBaseProtocol::Options{transport.RunningScanInterval()});
        m_protocol.SetLogSink(LogLine);
    }

    const auto started = Clock::now();
    bool awake = false;
    int writeFailures = 0;

    while (!stop.stop_requested()) {
        const auto now = Clock::now();
        {
            std::lock_guard lock(m_mutex);
            m_protocol.Tick(now);
            awake = m_protocol.IsAwake();
        }
        if (!awake && now - started > kWakeDeadline) {
            LogLine("portal: la base ne répond pas au réveil, nouvelle tentative de connexion");
            break;
        }

        Report out;
        for (;;) {
            {
                std::lock_guard lock(m_mutex);
                if (!m_protocol.PopOutgoing(out)) break;
            }
            if (transport.Write(out)) {
                writeFailures = 0;
            } else if (++writeFailures >= kMaxWriteFailures) {
                LogLine("portal: écritures en échec, la base est considérée perdue");
                goto session_end;
            }
        }

        Report in;
        const auto result = transport.Read(in, kReadSliceMs);
        if (result == IPortalTransport::ReadResult::Lost) goto session_end;
        {
            std::lock_guard lock(m_mutex);
            if (result == IPortalTransport::ReadResult::Ok) m_protocol.OnReport(in, Clock::now());
            auto events = m_protocol.TakeEvents();
            m_events.insert(m_events.end(), events.begin(), events.end());
        }
    }

session_end:
    std::lock_guard lock(m_mutex);
    m_protocol.DropAllTags();
    auto events = m_protocol.TakeEvents();
    m_events.insert(m_events.end(), events.begin(), events.end());
}

} // namespace crabe::infrastructure::portal
