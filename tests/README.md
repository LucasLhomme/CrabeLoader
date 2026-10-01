# CrabeLoader Test Suite & Benchmarks Reference

This document outlines the complete test suite architecture for **CrabeLoader**, the execution commands, and the **official baseline benchmarks**. When refactoring or optimizing C++ or Lua subsystems, use these quantitative metrics to objectively measure performance gains and verify zero regression.

---

## 1. Testing Strategy Overview

The test suite is structured across three complementary tiers:

```
                  ┌──────────────────────────────────────────────┐
                  │          In-Game Multi-Workload Bench        │
                  │   test_bench.ps1 (Real DI3 engine process,   │
                  │   RAM, Frametimes, Fibers, F4, Concurrency)  │
                  └──────────────────────┬───────────────────────┘
                                         │
                  ┌──────────────────────┴───────────────────────┐
                  │       Lua API & Coroutine Engine Suite       │
                  │   run_lua_tests.py (82 offline tests, Mocks, │
                  │   Quarantine, Scheduler, Pacing, Registry)   │
                  └──────────────────────┬───────────────────────┘
                                         │
                  ┌──────────────────────┴───────────────────────┐
                  │        C++ Offline Unit Tests & Bench        │
                  │   test_vfs_override, test_telemetry,         │
                  │   crabe_fate (DAG, Topo sort, O(1) lookups)  │
                  └──────────────────────────────────────────────┘
```

---

## 2. Offline C++ Unit Tests & Micro-Benchmarks

These tests run standalone without requiring the `DisneyInfinity3.exe` game process. They enable sub-second verification cycles during development.

### A. VFS Override Micro-Benchmark (`test_vfs_override.exe`)
* **Source file:** [`tests/cpp/test_vfs_override.cpp`](file:///e:/Dev/DIM2/CrabeLoader/tests/cpp/test_vfs_override.cpp)
* **Tested component:** [`src/infrastructure/vfs_override_manager.cpp`](file:///e:/Dev/DIM2/CrabeLoader/src/infrastructure/vfs_override_manager.cpp)
* **Purpose:** Validates case-insensitive virtual path normalization, deterministic mod mount priority, and measures raw $O(1)$ lookup throughput.
* **Execution command:**
  ```powershell
  cmake --build build --config Release --target test_vfs_override
  .\build\Release\test_vfs_override.exe
  ```
* **Baseline Benchmark:**
  * **Volume:** 100,000 consecutive virtual path resolutions.
  * **Total duration:** **~31.0 ms**
  * **Unit latency:** **~0.31 µs / lookup**
* **Optimization target:** Any algorithm or hash-table refactoring should maintain or reduce latency below `0.30 µs`.

---

### B. Telemetry & Statistical Pacing Engine (`test_telemetry.exe`)
* **Source file:** [`tests/cpp/test_telemetry.cpp`](file:///e:/Dev/DIM2/CrabeLoader/tests/cpp/test_telemetry.cpp)
* **Tested component:** [`src/infrastructure/telemetry_monitor.cpp`](file:///e:/Dev/DIM2/CrabeLoader/src/infrastructure/telemetry_monitor.cpp)
* **Purpose:** Validates SOLID interfaces (`ITelemetryCollector`, `ITelemetryExporter`), percentile accuracy (P95, P99), frametime jitter (standard deviation), stutter classification (>16 ms, >33 ms, >50 ms), memory drift leak triggers, and concurrent multithreading safety.
* **Execution command:**
  ```powershell
  cmake --build build --config Release --target test_telemetry
  .\build\Release\test_telemetry.exe
  ```
* **Baseline Benchmark:**
  * **Concurrent workload:** 4 worker threads performing 500 recording iterations (2,000 simultaneous metric updates).
  * **Thread safety:** 100% counter consistency with zero deadlocks or data corruption.
  * **Execution duration:** **< 2 ms**.

---

### B2. Update Check (`test_update_checker.exe`) and Installer (`test_installer.exe`)
* **Source files:** [`tests/cpp/test_update_checker.cpp`](file:///e:/Dev/DIM2/CrabeLoader/tests/cpp/test_update_checker.cpp), [`tests/cpp/test_installer.cpp`](file:///e:/Dev/DIM2/CrabeLoader/tests/cpp/test_installer.cpp)
* **Purpose:** The update check runs against fake network, prompt and browser ports (up to date, offline, draft, declined, accepted, unreadable answer, `[updates]` config key). The installer suite drives the real file operations in a temporary folder: fresh install, update, every refusal leaving the folder untouched, a locked file, SHA-256 vectors, and the Steam library parser.
* **Execution command:**
  ```powershell
  cmake --build build --config Release --target test_update_checker test_installer
  .\build\Release\test_update_checker.exe
  .\build\Release\test_installer.exe
  ```
* **Manual probe:** `update_probe.exe` runs the real check outside the game (see [`docs/guides/updates_and_installer.md`](file:///e:/Dev/DIM2/CrabeLoader/docs/guides/updates_and_installer.md)).

---

### C. Dependency Graph & DAG Regression Suite (`crabe_fate.exe`)
* **Source file:** [`tests/cpp/crabe_fate_main.cpp`](file:///e:/Dev/DIM2/CrabeLoader/tests/cpp/crabe_fate_main.cpp)
* **Test directories:** [`tests/fate/samples/`](file:///e:/Dev/DIM2/CrabeLoader/tests/fate/samples/) and [`tests/fate/ref/`](file:///e:/Dev/DIM2/CrabeLoader/tests/fate/ref/)
* **Purpose:** Validates topological sorting of the directed acyclic graph (DAG) of mod dependencies, circular dependency detection, malformed JSON resilience, and apostrophe handling in mod folder names.
* **Execution command:**
  ```powershell
  powershell -ExecutionPolicy Bypass -File .\crabe-fate.ps1
  # or directly:
  .\build\Release\crabe_fate.exe
  ```
* **Baseline Benchmark:**
  * **6 golden regression suites:** 01_minimal, 02_apostrophe, 03_dep_chain, 04_circular_error, 05_vfs_tree, 06_malformed_json.
  * **Total duration:** **~4.0 - 5.2 ms**.

---

## 3. Offline Lua API & Concurrency Suite

This suite executes all embedded Lua runtime modules inside a standalone emulator (`crabe_lua.exe`, standalone Lua 5.1).

* **Test runner:** [`tests/run_lua_tests.py`](file:///e:/Dev/DIM2/CrabeLoader/tests/run_lua_tests.py) and [`tests/lua/run_tests.lua`](file:///e:/Dev/DIM2/CrabeLoader/tests/lua/run_tests.lua)
* **Covered test suites:**
  1. `test_events.lua`: pub/sub event bus, one-time listeners (`once`), listener error reporting.
  2. `test_injection.lua`: code injection protection during module chunk compilation.
  3. `test_lifecycle_ownership.lua`: strict attribution of hooks and subscriptions to owner mods.
  4. `test_quarantine.lua`: fault-isolation cascade (10 consecutive failures isolate callback, 3 disabled callbacks disable mod).
  5. `test_registry.lua`: idempotent registration, unregistration, and zero leaks.
  6. `test_reload.lua`: resilience across 50 consecutive hot-reload cycles returning to exact baseline.
  7. `test_scheduler.lua`: coroutine engine (`Crabe.spawn`, `Crabe.wait`, temporal advancement).
  8. `test_settings.lua`: persistent menu configuration.
  9. `test_vfs_api.lua`: Lua redirection stubs and API bindings.
  10. `test_monitoring.lua`: 100 concurrent scheduled fibers, fiber error containment, frametime pacing statistics.
* **Execution command:**
  ```powershell
  python tests/run_lua_tests.py
  ```
* **Baseline Benchmark:**
  * **Total:** **82 passed, 0 failed**.
  * **Execution duration:** **~180 - 250 ms**.

---

## 4. In-Game Multi-Workload & Endurance Bench (`tests/test_bench.ps1`)

The in-game test bench is the definitive endurance and performance benchmark. It deploys the compiled DLL ([`build/Release/bink2w32.dll`](file:///e:/Dev/DIM2/CrabeLoader/build/Release/bink2w32.dll)) into the *Disney Infinity 3.0* game directory as a proxy, injects concurrent test mods, boots the game, and continuously tracks memory and frametime metrics.

* **Script:** [`tests/test_bench.ps1`](file:///e:/Dev/DIM2/CrabeLoader/tests/test_bench.ps1)
* **Launcher Script:** [`tests/test_game.ps1`](file:///e:/Dev/DIM2/CrabeLoader/tests/test_game.ps1)
* **Concurrent workloads applied:**
  * **VFS:** 20 path resolutions every 5 frames (~14,000 resolutions / minute).
  * **Events:** 10 pub/sub events every 10 frames (~3,500 events / minute).
  * **Companion Mod:** Bidirectional inter-mod pub/sub communication (`ping` $\rightarrow$ `pong`).
  * **Scheduler:** Recurring spawn of asynchronous coroutine fibers yielding with `Crabe.wait(150)` and resuming.
  * **Chaos Testing:** Controlled error probes confirming zero render-thread disruption.
  * **F4 Hot-Reload:** Automated in-world pulses of F4 hot-reload cycles.

### Cross-Machine Game Directory Configuration

The scripts automatically discover the game folder across different environments using the following priority order:
1. **Command line argument**: `-GameDir "C:\path\to\Disney Infinity 3.0 Gold Edition"`
2. **Environment variable**: `$env:CRABE_GAME_DIR = "C:\path\to\Disney Infinity 3.0 Gold Edition"`
3. **Steam installation registry**: `HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 541670`
4. **Common Steam library locations**: `D:\SteamLibrary`, `C:\Program Files (x86)\Steam`, `C:\SteamLibrary`, `E:\SteamLibrary`, `F:\SteamLibrary`

If your game is installed in a non-standard location, set the environment variable once in your PowerShell session:
```powershell
$env:CRABE_GAME_DIR = "C:\YourCustomPath\Disney Infinity 3.0 Gold Edition"
```
Or supply `-GameDir` directly to any script invocation.

### Execution Commands

```powershell
# 1. In-World Interactive Mode (Recommended: prompts to load Toy Box/void, then triggers in-world F4 stress & soak):
powershell -ExecutionPolicy Bypass -File .\tests\test_bench.ps1 -DurationSeconds 60 -F4StressCount 5

# 2. Automated Empty World Mode (Zero-Noise isolated environment for surgical micro-stutter profiling):
powershell -ExecutionPolicy Bypass -File .\tests\test_bench.ps1 -EmptyWorld -WarmupSeconds 25 -DurationSeconds 60 -F4StressCount 5

# 3. Custom Game Directory override:
powershell -ExecutionPolicy Bypass -File .\tests\test_bench.ps1 -GameDir "E:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition" -DurationSeconds 60 -F4StressCount 5

# 4. Standard Dev Game Launcher:
powershell -ExecutionPolicy Bypass -File .\tests\test_game.ps1
```

### Baseline In-Game Metrics Reference

Consolidated from test audit outputs ([`tests/log.txt`](file:///e:/Dev/DIM2/CrabeLoader/tests/log.txt)) and machine-readable telemetry ([`tests/telemetry_report.json`](file:///e:/Dev/DIM2/CrabeLoader/tests/telemetry_report.json)):

| Monitoring Metric | Baseline Reference | Alert Threshold |
| :--- | :---: | :---: |
| **Process Boot Working Set** (Empty NT stub) | **3.39 MB** | < 10 MB |
| **In-World Baseline** (3D engine & shaders ready) | **639.35 MB** | ~600 - 800 MB (level-dependent) |
| **Peak Observed Working Set** (Multi-workloads) | **673.38 MB** | < 850 MB |
| **Steady-State Drift** (True Leak Metric) | **+24.33 MB** (over 60s) | **< 60.0 MB** (Critical threshold) |
| **Hot-Reload F4 Memory Cost** | **+0.4 MB to +2.7 MB / reload** | < 5 MB / reload |
| **Unhandled SEH Exceptions / Crashes** | **0** | **0 required** |
| **Average Frametime** | **16.6 ms** (solid 60 FPS) | < 20 ms |
| **Micro-stutters (> 33 ms)** | **0 to 2 frames** | < 5 frames / minute |
| **Lua Heap Size (GC)** | **~2.3 MB - 2.8 MB** | Stable post-GC |

---

## 5. Developer Optimization & Verification Checklist (Before / After)

When submitting an optimization, follow this checklist to verify measurable improvement:

1. **VFS Optimization (e.g. hash algorithm, cache improvements):**
   * *Tool:* Run `test_vfs_override.exe`.
   * *Target:* Total time for 100,000 lookups should drop below **30 ms** (baseline: 30.9 ms / 0.31 µs).
2. **Memory / Allocator Optimization:**
   * *Tool:* Run `test_bench.ps1 -DurationSeconds 60`.
   * *Target:* Memory drift (`Steady-State Drift`) should decrease (e.g. drop from +24 MB towards < 15 MB).
3. **Frametime Pacing / Smoothness Optimization:**
   * *Tool:* Check `stutters16ms` and `stutters33ms` in `telemetry_report.json`.
   * *Target:* Zero dropped frames during high-frequency VFS bursts.
4. **Non-Regression Verification:**
   * All 82 Lua unit tests and 6 CRABE-FATE regression tests must maintain a **100% PASS** rate.

