/*
** CrabeLoader
** File description:
** Implements telemetry collectors, statistical percentile math, and JSON report export.
** Guarantees thread-safe recording across concurrent worker threads and Lua fibers.
** Emits structured metrics to disk without blocking the main game rendering pipeline.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/telemetry_monitor.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <numeric>
#include "third_party/json.hpp"

namespace crabe::infrastructure {

/// Initializes the telemetry monitor and sets the initial session timestamp.
TelemetryMonitor::TelemetryMonitor(size_t windowCapacity)
    : _windowCapacity(windowCapacity == 0 ? 1000 : windowCapacity),
      _startTime(std::chrono::steady_clock::now())
{
}

/// Ingests a new frame delta and recalculates pacing statistics.
void TelemetryMonitor::recordFrameTime(double deltaSeconds)
{
    const double ms = deltaSeconds * 1000.0;
    std::lock_guard<std::mutex> lock(_mutex);

    _totalFrames++;
    _totalFrametimeMs += ms;

    if (_frametimesMs.empty() || ms < _minFrametimeMs) {
        _minFrametimeMs = ms;
    }
    if (ms > _maxFrametimeMs) {
        _maxFrametimeMs = ms;
    }

    if (ms > 50.0) {
        _stutters50ms++;
    } else if (ms > 33.33) {
        _stutters33ms++;
    } else if (ms > 16.67) {
        _stutters16ms++;
    }

    _frametimesMs.push_back(ms);
    if (_frametimesMs.size() > _windowCapacity) {
        _frametimesMs.pop_front();
    }
}

/// Updates current memory allocations and identifies potential leaks.
void TelemetryMonitor::recordMemorySample(double workingSetMb, double privateBytesMb, double luaHeapKb)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_initialWorkingSetMb <= 0.0) {
        _initialWorkingSetMb = workingSetMb;
    }
    if (workingSetMb > _peakWorkingSetMb) {
        _peakWorkingSetMb = workingSetMb;
    }

    _currentWorkingSetMb = workingSetMb;
    _currentPrivateBytesMb = privateBytesMb;
    _currentLuaHeapKb = luaHeapKb;
}

/// Tracks VFS resolutions and increments cache hit counters.
void TelemetryMonitor::recordVfsLookup(bool isHit)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _vfsLookups++;
    if (isHit) {
        _vfsHits++;
    }
}

/// Increments event dispatch count and records peak listeners.
void TelemetryMonitor::recordEventDispatched(size_t listenerCount)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _eventsDispatched++;
    if (listenerCount > _peakListeners) {
        _peakListeners = listenerCount;
    }
}

/// Updates active fiber count and peak concurrency records.
void TelemetryMonitor::recordFiberCount(size_t activeCount)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _fibersScheduled++;
    _currentFibers = activeCount;
    if (activeCount > _peakFibers) {
        _peakFibers = activeCount;
    }
}

/// Computes the requested percentile value from buffered frame durations.
double TelemetryMonitor::calculatePercentile(double percentile) const
{
    if (_frametimesMs.empty()) {
        return 0.0;
    }

    std::vector<double> sorted(_frametimesMs.begin(), _frametimesMs.end());
    std::sort(sorted.begin(), sorted.end());

    const double rank = (percentile / 100.0) * static_cast<double>(sorted.size() - 1);
    const size_t lower = static_cast<size_t>(std::floor(rank));
    const size_t upper = static_cast<size_t>(std::ceil(rank));
    const double weight = rank - static_cast<double>(lower);

    return sorted[lower] + weight * (sorted[upper] - sorted[lower]);
}

/// Computes frametime standard deviation as a measure of jitter.
double TelemetryMonitor::calculateJitter() const
{
    if (_frametimesMs.size() < 2) {
        return 0.0;
    }

    const double mean = std::accumulate(_frametimesMs.begin(), _frametimesMs.end(), 0.0) /
                        static_cast<double>(_frametimesMs.size());

    double sumSqDiff = 0.0;
    for (double val : _frametimesMs) {
        const double diff = val - mean;
        sumSqDiff += diff * diff;
    }

    return std::sqrt(sumSqDiff / static_cast<double>(_frametimesMs.size() - 1));
}

/// Compiles and returns a complete diagnostic snapshot of the system.
TelemetrySnapshot TelemetryMonitor::getSnapshot() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    TelemetrySnapshot s;

    const auto now = std::chrono::steady_clock::now();
    s.elapsedSeconds = std::chrono::duration<double>(now - _startTime).count();

    s.framePacing.totalFrames = _totalFrames;
    s.framePacing.averageFrametimeMs = _totalFrames > 0 ? (_totalFrametimeMs / static_cast<double>(_totalFrames)) : 0.0;
    s.framePacing.minFrametimeMs = _minFrametimeMs;
    s.framePacing.maxFrametimeMs = _maxFrametimeMs;
    s.framePacing.p95FrametimeMs = calculatePercentile(95.0);
    s.framePacing.p99FrametimeMs = calculatePercentile(99.0);
    s.framePacing.jitterMs = calculateJitter();
    s.framePacing.stutters16ms = _stutters16ms;
    s.framePacing.stutters33ms = _stutters33ms;
    s.framePacing.stutters50ms = _stutters50ms;

    s.memory.initialWorkingSetMb = _initialWorkingSetMb;
    s.memory.peakWorkingSetMb = _peakWorkingSetMb;
    s.memory.currentWorkingSetMb = _currentWorkingSetMb;
    s.memory.currentPrivateBytesMb = _currentPrivateBytesMb;
    s.memory.currentLuaHeapKb = _currentLuaHeapKb;
    s.memory.steadyStateDriftMb = _initialWorkingSetMb > 0.0 ? (_currentWorkingSetMb - _initialWorkingSetMb) : 0.0;
    s.memory.leakSuspected = s.memory.steadyStateDriftMb > 60.0;

    s.vfs.totalLookups = _vfsLookups;
    s.vfs.totalHits = _vfsHits;
    s.vfs.hitRatio = _vfsLookups > 0 ? (static_cast<double>(_vfsHits) / static_cast<double>(_vfsLookups)) : 0.0;

    s.events.totalDispatched = _eventsDispatched;
    s.events.peakListenerCount = _peakListeners;

    s.fibers.totalScheduled = _fibersScheduled;
    s.fibers.currentActive = _currentFibers;
    s.fibers.peakActive = _peakFibers;

    return s;
}

/// Serializes system snapshot metrics into structured JSON format.
std::string TelemetryMonitor::exportJson() const
{
    const TelemetrySnapshot s = getSnapshot();

    nlohmann::json j;
    j["elapsed_seconds"] = s.elapsedSeconds;

    j["frame_pacing"] = {
        {"total_frames", s.framePacing.totalFrames},
        {"average_frametime_ms", s.framePacing.averageFrametimeMs},
        {"min_frametime_ms", s.framePacing.minFrametimeMs},
        {"max_frametime_ms", s.framePacing.maxFrametimeMs},
        {"p95_frametime_ms", s.framePacing.p95FrametimeMs},
        {"p99_frametime_ms", s.framePacing.p99FrametimeMs},
        {"jitter_ms", s.framePacing.jitterMs},
        {"stutters_16ms", s.framePacing.stutters16ms},
        {"stutters_33ms", s.framePacing.stutters33ms},
        {"stutters_50ms", s.framePacing.stutters50ms}
    };

    j["memory"] = {
        {"initial_working_set_mb", s.memory.initialWorkingSetMb},
        {"peak_working_set_mb", s.memory.peakWorkingSetMb},
        {"current_working_set_mb", s.memory.currentWorkingSetMb},
        {"current_private_bytes_mb", s.memory.currentPrivateBytesMb},
        {"current_lua_heap_kb", s.memory.currentLuaHeapKb},
        {"steady_state_drift_mb", s.memory.steadyStateDriftMb},
        {"leak_suspected", s.memory.leakSuspected}
    };

    j["vfs"] = {
        {"total_lookups", s.vfs.totalLookups},
        {"total_hits", s.vfs.totalHits},
        {"hit_ratio", s.vfs.hitRatio}
    };

    j["events"] = {
        {"total_dispatched", s.events.totalDispatched},
        {"peak_listeners", s.events.peakListenerCount}
    };

    j["fibers"] = {
        {"total_scheduled", s.fibers.totalScheduled},
        {"current_active", s.fibers.currentActive},
        {"peak_active", s.fibers.peakActive}
    };

    return j.dump(2);
}

/// Clears collected metrics and restarts the timer.
void TelemetryMonitor::reset()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _frametimesMs.clear();
    _totalFrames = 0;
    _totalFrametimeMs = 0.0;
    _minFrametimeMs = 0.0;
    _maxFrametimeMs = 0.0;
    _stutters16ms = 0;
    _stutters33ms = 0;
    _stutters50ms = 0;
    _vfsLookups = 0;
    _vfsHits = 0;
    _eventsDispatched = 0;
    _peakListeners = 0;
    _fibersScheduled = 0;
    _currentFibers = 0;
    _peakFibers = 0;
    _startTime = std::chrono::steady_clock::now();
}

}
