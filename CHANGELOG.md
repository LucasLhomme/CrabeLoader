# Changelog

All notable changes to CrabeLoader are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.1.0] - 2026-10-16

### Added
- **Physical Disney Infinity Base (Portal) Support & Engine Gateway:** Complete USB base subsystem (`src/infrastructure/portal/`) supporting Xbox One (WinUSB + GIP security handshake), Xbox 360 (WinUSB), and HID bases (PS3, PS4, Wii, Wii U, PC). Includes background figure polling, per-figure encryption/decryption, RGB LED control, and safe double-buffered progression writes with read-back. The `Crabe.Portal` Lua API exposes portal status, LED controls, and figure placement events, while `Crabe.Gateway` bridges real RFID UIDs directly into the engine's figure registry for console-style per-figure progression.
- **In-Game Key Bindings Mod (`mods/keybinds`):** Restores the vanilla controls configuration menu under **Options › Controls** for both Keyboard/Mouse and Gamepad/Controller, with real game glyphs (`cbi_*`). Players can capture, swap, and reset inputs on the fly; configurations are saved to `Crabe/keybinds.json` and restored at launch. Includes the new `Crabe.Input.isKeyDown` native query.
- **`CrabeLoader.asi` + `dinput8.dll` Bootstrap Architecture:** The loader now builds as `CrabeLoader.asi`, initiated by a lightweight proxy bootstrap `dinput8.dll`. At the game's entry point, before any game logic executes, the bootstrap restores original bytes, loads `CrabeLoader.asi`, and calls its bootstrap export. Compatible with third-party ASI loaders (e.g. Ultimate ASI Loader) by installing `CrabeLoader.asi` standalone.
- **Native `.asi` Plugin Loader & C Plugin API:** Loads 32-bit `.asi` plugins from the game folder, `scripts/`, `plugins/`, and enabled mod directories prior to game initialization. Includes a public C SDK header (`include/sdk/crabe_plugin.h`) enabling native plugins to log under their own prefix, install hooks via `HookRegistry`, queue Lua code, and exchange key-value data with Lua mods through `Crabe.Plugins.get` / `Crabe.Plugins.set`.
- **Dynamic Build Fingerprint & Versioning:** Builds are stamped with a descriptive label (`1.1.0-dev.<YYYYMMDD>.<HHMM>+<commit>`), surfaced in `loader.log`, overlay titles, crash dumps, and via `Crabe.versionFull` / `Crabe.buildCommit`. Release builds stamped via `-DCRABELOADER_RELEASE_BUILD=ON` resolve cleanly to `1.1.0`.
- **Zero-Touch Loose Texture Overrides:** `.tbody` textures dropped into `mods/<mod>/textures/` dynamically replace vanilla archive textures by hash without requiring `.zip` repacking, handled directly inside `EngineAssetLoader` using the engine's allocator.
- **Automated Documentation Website (Astro Starlight & GitHub Pages):** Deployed documentation website featuring full-text search, dark obsidian aesthetics, automatic generation of Lua API pages from `crabe_api.def.lua`, and GitHub Actions CI/CD deployment.
- **Comprehensive Modder How-To Guides Suite:** Ten in-depth technical guides covering playable character creation pipelines (`build_character.py`), asset replacement, custom enemy/boss creation (`RadahnPrime` archetype inheritance), event bus, lifecycle engine, coroutines, freecam, spawners, weapons, and 2D ImGui drawing.
- **Native C++23 Settings & Engine Patches (`patch/`):** Display resolution presets (from 640x480 up to 8K UHD), borderless window mode toggle, and frame rate limiter options are now implemented in pure compiled C++23 (`src/infrastructure/patch/`) through `PatchResolution`, `PatchBorderless`, and `PatchKeybind`. Presets sorting, deduplication (`std::unique`), active mode detection, and `RenderHook` synchronization run natively in C++ using direct Lua C callbacks, eliminating the need for standalone Lua mod files in `mods/`.
- **Bundled Release Mods:** `resolution.lua` and `window_mode.lua` are now automatically packaged into the release zip's `mods/` folder.

### Changed
- **Modern CMake `FetchContent` Integration:** Upstream third-party dependencies (`nlohmann/json` v3.11.3, `toml++` v3.4.0, `MinHook` v1.3.3, `Dear ImGui` v1.90.9, `Lua 5.1.5`) are now fetched and built automatically via CMake `FetchContent` with strict SHA-256 validation.
- **Codebase Comments Standardization:** Enforced a clean modern C++ standard across all 213 files: maximum 3 physical lines per function comment directly above declarations/definitions, zero inline comments inside function bodies, and zero comments on types, members, enums, macros, or constants. Verified by `tools/check_headers.py`.
- **Proxy Header Relocation:** Relocated Bink export forwarding definitions to `include/infrastructure/proxy.hpp` and provided zero-cost in-build compatibility shims (`include_shims`).
- **Installer Upgrades (`CrabeInstaller.exe`):** Rewritten installer embedding both `CrabeLoader.asi` and `dinput8.dll`, supporting fresh installs, updates, migration from 1.0 (restoring original `bink2w32.dll`), safe staging with SHA-256 verification, and automatic rollback on failure.

### Removed
- **Vendored Third-Party Source Trees:** Purged 53,236 lines of vendored code (`include/imgui`, `include/minhook`, `src/minhook`) from the repository in favor of CMake `FetchContent`.

### Fixed
- **DirectInput Gameplay Locking & Input Capture:** Implemented `DirectInputHook::setCaptured` and `Crabe.Input.captureDirectInput` (`Crabe._setDirectInputCaptured`), blanking `IDirectInputDevice8` keyboard state, mouse state, and buffered events when gameplay controls are captured. Fixes player character and camera continuing to move while navigating menus like CrabeMenu.
- **In-Game Keybinds Menu Container Isolation:** Restricted native keybinding rows (`Keyboard and mouse bindings`, `Controller bindings`) to `SettingsMenu` in the frontend main menu, preventing unparented and broken `SettingsExtras` screens from mounting inside the in-game pause menu (`InGameSettingsMenu`).
- **Alt-Tab Freeze During Level Loads:** Hooked `ShowWindow` to refuse minimize requests non-blockingly using `ShowWindowAsync` and query visibility without synchronous cross-thread Win32 calls, eliminating the deadlock between the main Scaleform thread and the DirectX render thread.
- **Settings Screen Injector Reliability:** Replaced short-lived timer polling with a permanent background poller ensuring video settings options ("Borderless Window", "Frame Rate Limit") reliably mount in the game's menus across all sessions.
- **True Unlimited Frame Rate:** Present calls with interval 0 when uncapped or exceeding refresh rate, preventing vsync locks at high refresh rates.
- **Borderless Window Mode Application:** Fixed missing `SetFullscreenState` hook installation so configured display modes apply reliably from startup.
- **Overlay Windowed Mode Coordinates:** ImGui coordinates now map accurately to back-buffer pixels regardless of client window scale.
- **Alt+Enter Debouncing:** Eliminated rapid toggle loops while Alt+Enter is held and restricted config writes to explicit mode changes.
- **Escape & Back Navigation Unblocking Across Playset & Character Selection:** Neutralized Disney's PC "Onboard Tutorial" state checks (`UI_IsOnboardTutorialActive` and `UI_ShouldOnboardTutorialRestrictInput`) that silently swallowed the Escape / Cancel key (`KeyMap.GN_LST_CANCEL`) in `PlaysetListWin32`, `MenuBase` (`PlaysetSelectMenu`, `GameLoad`), and `VirtualReaderPC_Loadout` / `VirtualReaderPC_GridCharacter`. Restored proper parent menu refocusing upon canceling character loadout.
- **MSVC Multi-Processor Compilation:** Enabled `/MP` in CMake configuration for MSVC, speeding up full rebuild times by compiling translation units concurrently across all available CPU cores.

## [1.0.0] - 2026-10-01

### Added
- **Update Check:** At launch CrabeLoader asks GitHub for the latest release and, if a newer stable one exists, shows a native Yes/No box that opens the releases page. Never blocks the game, asks again only at the next launch, and can be disabled with `[updates] check = false` in `crabe.toml`.
- **`CrabeInstaller.exe`:** One-click installer (native folder picker, Steam detection, `bink2w32.dll` proxying with the original kept as `bink2w32_orig.dll`, `mods/` folder and `README.txt`). It identifies the real original by SHA-256 and refuses to touch anything it does not recognise. Built with `-DCRABELOADER_BUILD_INSTALLER=ON`.
- **`Crabe.Settings` API:** Allows mods to inject custom toggle and selection options into the game's native settings menus.
- **Borderless Window Mode:** Full borderless window support with instant Alt+Tab transitions and background FPS throttling (30 FPS) when unfocused.
- **Configuration Engine (`crabe.toml`):** Dedicated TOML configuration file supporting display settings, keybinding rebinding, profile management, and fault quarantine.
- **Mod Manifest Support (`mod.json`):** Semantic versioning (`SemVer`), range constraints, and automatic topological dependency resolution.
- **Game Profile & PE Identifier:** Runtime PE header validation and fingerprinting for Disney Infinity 3.0 Gold Edition builds.
- **SEH Crash Reporter:** Hardware-level exception handling generating Windows minidumps and module+RVA callstack traces.
- **CLI Tooling (`crabe-cli`):** Offline utility for scaffolding, validating manifests, and checking Lua API usages.
- **Hardware Virtual Reader API:** Native Lua runtime bindings for emulating physical Disney Infinity RFID base portals, figures, and discs.

### Changed
- **Architecture Modernization:** Unified codebase under C++23 standards, strict RAII ownership, and the root `crabe::` namespace.
- **Naming Conventions:** Standardized source file and header names to lowercase `snake_case`.
- **Mod Lifecycle:** Mod subscriptions and lifecycle hooks are now explicitly tracked and revoked on reload.

### Fixed
- **IP Addresses in `loader.log`:** The multiplayer module no longer writes the player's local or public IP address, or a Direct Connect target address, to the log. Logs can now be shared in bug reports without editing them first.
- **Settings Screen Race Condition:** Resolved timing issue where game options menu would render blank when injecting options.
- **Multi-State Mod Reload (F4):** Ensured mod reloads are properly dispatched across all active engine Lua states.
- **Hook Registry Safety:** Prevented hook collisions and attributed memory hook ownership.
