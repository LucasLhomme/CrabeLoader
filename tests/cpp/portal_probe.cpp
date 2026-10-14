/*
** CrabeLoader
** File description:
** Out-of-game probe for the physical portal: runs the real PortalWorker in a console.
** Prints connection changes and figure events with saved progress, cycles the LEDs once.
** Usage: portal_probe.exe [seconds], 30 seconds by default.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/portal/portal_worker.hpp"
#include "shared/logger.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

using namespace crabe::infrastructure::portal;

int main(int argc, char** argv) {
    const int seconds = argc > 1 ? std::atoi(argv[1]) : 30;
    auto& log = crabe::shared::Logger::getInstance();
    PortalWorker worker;
    worker.Start();

    bool wasConnected = false;
    int ledStep = 0;
    const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    while (std::chrono::steady_clock::now() < end) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        const auto snapshot = worker.GetSnapshot();
        if (snapshot.connected != wasConnected) {
            wasConnected = snapshot.connected;
            log.info("probe: connected={} type={} ({})", snapshot.connected, static_cast<int>(snapshot.type),
                     snapshot.description);
        }
        if (snapshot.connected && ledStep < 12) {
            static constexpr std::uint8_t kColors[4][3] = {{0, 255, 255}, {255, 0, 255}, {255, 215, 0}, {0, 0, 0}};
            if (ledStep % 3 == 0) {
                const auto& c = kColors[ledStep / 3];
                log.info("probe: LED {} {} {}", c[0], c[1], c[2]);
                static_cast<void>(worker.SetLed(0, c[0], c[1], c[2]));
            }
            ++ledStep;
        }
        for (const auto& event : worker.TakeEvents()) {
            log.info("probe: {} slot={} sku={} uid={:014X}",
                     event.type == PortalTagEvent::Type::Placed ? "POSE" : "RETIRE", event.slot, event.sku, event.uid);
            if (event.progress) {
                std::string bits;
                for (int n = 0; n < 64; ++n) {
                    if (event.progress->skillBits[n / 8] & (1 << (n % 8))) bits += std::to_string(n) + " ";
                }
                log.info("probe:   niveau {} xp {} flags 0x{:02X} competences [{}]", event.progress->level,
                         event.progress->experience, event.progress->actionFlags, bits);
            }
        }
    }
    const auto snapshot = worker.GetSnapshot();
    log.info("probe: fin, connected={} diagnostic={}", snapshot.connected, snapshot.diagnostic);
    worker.Stop();
    return 0;
}
