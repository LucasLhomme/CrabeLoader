#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "infrastructure/telemetry_monitor.hpp"
#include "third_party/json.hpp"

namespace {

/// Asserts that a boolean condition is satisfied or exits immediately.
void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "[FAIL] " << message << std::endl;
        std::exit(1);
    }
}

/// Asserts that two double precision values are equal within an epsilon.
void requireNear(double a, double b, double eps, const char* message)
{
    if (std::abs(a - b) > eps) {
        std::cerr << "[FAIL] " << message << " (expected " << b << ", got " << a << ")" << std::endl;
        std::exit(1);
    }
}

/// Verifies frame pacing metrics, percentiles, and stutter detection.
void testFramePacing()
{
    using namespace crabe::infrastructure;
    std::unique_ptr<ITelemetryCollector> collector = std::make_unique<TelemetryMonitor>(500);

    for (int i = 0; i < 90; ++i) {
        collector->recordFrameTime(0.01667);
    }
    for (int i = 0; i < 5; ++i) {
        collector->recordFrameTime(0.025);
    }
    for (int i = 0; i < 3; ++i) {
        collector->recordFrameTime(0.035);
    }
    for (int i = 0; i < 2; ++i) {
        collector->recordFrameTime(0.055);
    }

    auto* exporter = dynamic_cast<ITelemetryExporter*>(collector.get());
    require(exporter != nullptr, "Cast to ITelemetryExporter failed");

    TelemetrySnapshot s = exporter->getSnapshot();
    require(s.framePacing.totalFrames == 100, "Total frames mismatch");
    require(s.framePacing.stutters16ms == 5, "16ms stutters mismatch");
    require(s.framePacing.stutters33ms == 3, "33ms stutters mismatch");
    require(s.framePacing.stutters50ms == 2, "50ms stutters mismatch");
    require(s.framePacing.p99FrametimeMs >= 50.0, "p99 frametime mismatch");
    require(s.framePacing.minFrametimeMs > 0.0, "min frametime should be non-zero");
    require(s.framePacing.maxFrametimeMs >= 55.0, "max frametime mismatch");
}

/// Verifies memory baseline tracking, peak working set, and drift calculation.
void testMemoryTracking()
{
    using namespace crabe::infrastructure;
    TelemetryMonitor monitor;

    monitor.recordMemorySample(500.0, 480.0, 2048.0);
    monitor.recordMemorySample(520.0, 500.0, 2100.0);
    monitor.recordMemorySample(580.0, 560.0, 2400.0);
    monitor.recordMemorySample(540.0, 520.0, 2200.0);

    TelemetrySnapshot s = monitor.getSnapshot();
    requireNear(s.memory.initialWorkingSetMb, 500.0, 0.01, "Initial working set mismatch");
    requireNear(s.memory.peakWorkingSetMb, 580.0, 0.01, "Peak working set mismatch");
    requireNear(s.memory.currentWorkingSetMb, 540.0, 0.01, "Current working set mismatch");
    requireNear(s.memory.steadyStateDriftMb, 40.0, 0.01, "Drift mismatch");
    require(!s.memory.leakSuspected, "Drift <= 60 MB should not suspect leak");

    monitor.recordMemorySample(600.0, 580.0, 2500.0);
    s = monitor.getSnapshot();
    requireNear(s.memory.steadyStateDriftMb, 100.0, 0.01, "High drift mismatch");
    require(s.memory.leakSuspected, "Drift > 60 MB should suspect leak");
}

/// Verifies VFS cache hit ratio calculation and event counters.
void testVfsAndEvents()
{
    using namespace crabe::infrastructure;
    TelemetryMonitor monitor;

    monitor.recordVfsLookup(true);
    monitor.recordVfsLookup(true);
    monitor.recordVfsLookup(true);
    monitor.recordVfsLookup(false);

    monitor.recordEventDispatched(4);
    monitor.recordEventDispatched(8);
    monitor.recordEventDispatched(2);

    monitor.recordFiberCount(5);
    monitor.recordFiberCount(12);
    monitor.recordFiberCount(7);

    TelemetrySnapshot s = monitor.getSnapshot();
    require(s.vfs.totalLookups == 4, "Total VFS lookups mismatch");
    require(s.vfs.totalHits == 3, "Total VFS hits mismatch");
    requireNear(s.vfs.hitRatio, 0.75, 0.001, "VFS hit ratio mismatch");

    require(s.events.totalDispatched == 3, "Events dispatched mismatch");
    require(s.events.peakListenerCount == 8, "Peak listeners mismatch");

    require(s.fibers.totalScheduled == 3, "Fibers scheduled mismatch");
    require(s.fibers.currentActive == 7, "Current fibers mismatch");
    require(s.fibers.peakActive == 12, "Peak fibers mismatch");
}

/// Verifies JSON serialization against parsed fields.
void testJsonSerialization()
{
    using namespace crabe::infrastructure;
    TelemetryMonitor monitor;

    monitor.recordFrameTime(0.016);
    monitor.recordMemorySample(100.0, 90.0, 1024.0);
    monitor.recordVfsLookup(true);
    monitor.recordEventDispatched(2);
    monitor.recordFiberCount(3);

    std::string jsonStr = monitor.exportJson();
    nlohmann::json parsed = nlohmann::json::parse(jsonStr);

    require(parsed.contains("frame_pacing"), "JSON missing frame_pacing");
    require(parsed.contains("memory"), "JSON missing memory");
    require(parsed.contains("vfs"), "JSON missing vfs");
    require(parsed.contains("events"), "JSON missing events");
    require(parsed.contains("fibers"), "JSON missing fibers");
    require(parsed["frame_pacing"]["total_frames"] == 1, "Parsed total frames mismatch");
    require(parsed["vfs"]["hit_ratio"] == 1.0, "Parsed hit ratio mismatch");
}

/// Verifies thread safety during concurrent recording across multiple threads.
void testConcurrency()
{
    using namespace crabe::infrastructure;
    TelemetryMonitor monitor;

    std::vector<std::thread> workers;
    for (int t = 0; t < 4; ++t) {
        workers.emplace_back([&monitor]() {
            for (int i = 0; i < 500; ++i) {
                monitor.recordFrameTime(0.016);
                monitor.recordVfsLookup(i % 2 == 0);
                monitor.recordEventDispatched(1);
                monitor.recordFiberCount(static_cast<size_t>(i % 10));
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    TelemetrySnapshot s = monitor.getSnapshot();
    require(s.framePacing.totalFrames == 2000, "Concurrent frames mismatch");
    require(s.vfs.totalLookups == 2000, "Concurrent VFS lookups mismatch");
    require(s.events.totalDispatched == 2000, "Concurrent events mismatch");
}

}

/// Main test entry point executing all telemetry test cases.
int main()
{
    std::cout << "[Test] Starting Telemetry & Monitoring Unit Tests...\n";

    testFramePacing();
    std::cout << "  [PASS] Frame pacing and stutter metrics\n";

    testMemoryTracking();
    std::cout << "  [PASS] Memory tracking and leak detection\n";

    testVfsAndEvents();
    std::cout << "  [PASS] VFS and Event telemetry\n";

    testJsonSerialization();
    std::cout << "  [PASS] Structured JSON export\n";

    testConcurrency();
    std::cout << "  [PASS] Concurrent multithreaded recording\n";

    std::cout << "[Test] ALL TELEMETRY UNIT TESTS PASSED.\n";
    return 0;
}
