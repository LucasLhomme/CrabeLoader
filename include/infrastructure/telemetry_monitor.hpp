/*
** CrabeLoader
** File description:
** Declares telemetry tracking structures and collector interfaces for profiling.
** Provides statistical frametime percentiles, jitter calculation, and memory leak alerts.
** Defers metric ingestion and snapshot formatting to concrete monitor implementations.
**
** Authors: @LucasLhomme
*/

#ifndef TELEMETRY_MONITOR_HPP_
#define TELEMETRY_MONITOR_HPP_

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace crabe::infrastructure {

/// Telemetry metrics for frame pacing and stutter detection.
struct FramePacingMetrics {
    uint64_t totalFrames{0};
    double averageFrametimeMs{0.0};
    double minFrametimeMs{0.0};
    double maxFrametimeMs{0.0};
    double p95FrametimeMs{0.0};
    double p99FrametimeMs{0.0};
    double jitterMs{0.0};
    uint64_t stutters16ms{0};
    uint64_t stutters33ms{0};
    uint64_t stutters50ms{0};
};

/// Telemetry metrics for process memory tracking and leak detection.
struct MemoryMetrics {
    double initialWorkingSetMb{0.0};
    double peakWorkingSetMb{0.0};
    double currentWorkingSetMb{0.0};
    double currentPrivateBytesMb{0.0};
    double currentLuaHeapKb{0.0};
    double steadyStateDriftMb{0.0};
    bool leakSuspected{false};
};

/// Telemetry metrics for VFS resolutions and cache performance.
struct VfsMetrics {
    uint64_t totalLookups{0};
    uint64_t totalHits{0};
    double hitRatio{0.0};
};

/// Telemetry metrics for the event bus.
struct EventMetrics {
    uint64_t totalDispatched{0};
    size_t peakListenerCount{0};
};

/// Telemetry metrics for coroutine fiber scheduler.
struct FiberMetrics {
    uint64_t totalScheduled{0};
    size_t currentActive{0};
    size_t peakActive{0};
};

/// Aggregate telemetry snapshot combining all system metrics.
struct TelemetrySnapshot {
    std::string timestamp;
    double elapsedSeconds{0.0};
    FramePacingMetrics framePacing;
    MemoryMetrics memory;
    VfsMetrics vfs;
    EventMetrics events;
    FiberMetrics fibers;
};

/// Abstract interface for recording telemetry events.
class ITelemetryCollector {
public:
    virtual ~ITelemetryCollector() = default;

    /// Records the execution duration of a single rendered frame.
    virtual void recordFrameTime(double deltaSeconds) = 0;

    /// Records current memory usage and updates baseline drift.
    virtual void recordMemorySample(double workingSetMb, double privateBytesMb, double luaHeapKb) = 0;

    /// Records a virtual file system lookup and cache hit status.
    virtual void recordVfsLookup(bool isHit) = 0;

    /// Records an event dispatch with its associated listener count.
    virtual void recordEventDispatched(size_t listenerCount) = 0;

    /// Updates the count of active coroutine fibers.
    virtual void recordFiberCount(size_t activeCount) = 0;
};

/// Abstract interface for reading and serializing telemetry snapshots.
class ITelemetryExporter {
public:
    virtual ~ITelemetryExporter() = default;

    /// Generates a complete snapshot of all collected telemetry metrics.
    [[nodiscard]] virtual TelemetrySnapshot getSnapshot() const = 0;

    /// Serializes the aggregate telemetry data into a JSON string.
    [[nodiscard]] virtual std::string exportJson() const = 0;

    /// Resets all internal metric accumulators to initial state.
    virtual void reset() = 0;
};

/// Thread-safe performance and telemetry monitor implementing SOLID collection and export.
class TelemetryMonitor final : public ITelemetryCollector, public ITelemetryExporter {
public:
    /// Constructs a telemetry monitor with an optional sliding window capacity.
    explicit TelemetryMonitor(size_t windowCapacity = 1000);

    ~TelemetryMonitor() override = default;

    TelemetryMonitor(const TelemetryMonitor&) = delete;
    TelemetryMonitor& operator=(const TelemetryMonitor&) = delete;
    TelemetryMonitor(TelemetryMonitor&&) = delete;
    TelemetryMonitor& operator=(TelemetryMonitor&&) = delete;

    /// Records the execution duration of a single rendered frame.
    void recordFrameTime(double deltaSeconds) override;

    /// Records current memory usage and updates baseline drift.
    void recordMemorySample(double workingSetMb, double privateBytesMb, double luaHeapKb) override;

    /// Records a virtual file system lookup and cache hit status.
    void recordVfsLookup(bool isHit) override;

    /// Records an event dispatch with its associated listener count.
    void recordEventDispatched(size_t listenerCount) override;

    /// Updates the count of active coroutine fibers.
    void recordFiberCount(size_t activeCount) override;

    /// Generates a complete snapshot of all collected telemetry metrics.
    [[nodiscard]] TelemetrySnapshot getSnapshot() const override;

    /// Serializes the aggregate telemetry data into a JSON string.
    [[nodiscard]] std::string exportJson() const override;

    /// Resets all internal metric accumulators to initial state.
    void reset() override;

private:
    [[nodiscard]] double calculatePercentile(double percentile) const;
    [[nodiscard]] double calculateJitter() const;

    mutable std::mutex _mutex;
    size_t _windowCapacity{1000};
    std::chrono::steady_clock::time_point _startTime;

    std::deque<double> _frametimesMs;
    uint64_t _totalFrames{0};
    double _totalFrametimeMs{0.0};
    double _minFrametimeMs{0.0};
    double _maxFrametimeMs{0.0};
    uint64_t _stutters16ms{0};
    uint64_t _stutters33ms{0};
    uint64_t _stutters50ms{0};

    double _initialWorkingSetMb{0.0};
    double _peakWorkingSetMb{0.0};
    double _currentWorkingSetMb{0.0};
    double _currentPrivateBytesMb{0.0};
    double _currentLuaHeapKb{0.0};

    uint64_t _vfsLookups{0};
    uint64_t _vfsHits{0};

    uint64_t _eventsDispatched{0};
    size_t _peakListeners{0};

    uint64_t _fibersScheduled{0};
    size_t _currentFibers{0};
    size_t _peakFibers{0};
};

} // namespace crabe::infrastructure

#endif /* !TELEMETRY_MONITOR_HPP_ */
