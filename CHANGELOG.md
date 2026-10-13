# Changelog

All notable changes to CrabeLoader are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Loose textures are served:** a `.tbody` under `mods/<mod>/textures/` now replaces the archive texture of the same id, with no zip needed. The VFS only saw Win32 file calls, and the engine reads textures out of `assets/textures/<xx>.zip` by hash, so loose textures were indexed but never used. A hook on the engine's file loader (`EngineAssetLoader`) now answers those requests with the mod's file, in a buffer from the engine's own allocator, and falls back to the archive on any failure. Only `.tbody` requests are touched. `crabe_disable_asset_override.txt` next to the executable turns it off; `crabe_probe_asset_loader.txt` logs every texture request. New `VfsOverrideManager::readOverrideBytes`, covered by `test_vfs_override`.

### Fixed
- **Settings-screen options appear without an F4:** `Crabe.Settings` gave up looking for a screen after about eight seconds, and the chunk patch alone did not always catch it, so in 3 of 17 measured sessions the "Borderless Window" and "Frame Rate Limit" rows were missing from the video settings until a hot reload. A poller now checks about twice a second for the whole session, is owned by the loader so a mod reload cannot orphan it, and does nothing once the screen is wrapped. The saved window mode and frame limit were never affected: they are applied natively at startup.
- **"Unlimited" frame rate is really unlimited:** the game presents with a sync interval of 1 (vsync) from its first frame, and the loader passed that through, so "Unlimited" and any cap at or above the display refresh stayed pinned at the refresh rate (180 FPS on a 180 Hz screen). The loader now presents with interval 0 in those cases and keeps the game's own value for lower caps. The sync interval and swap chain description are logged when they change.
- **Window mode actually applies:** the `SetFullscreenState` hook was declared but never installed, and the configured mode was never applied at startup, so the game stayed in its own exclusive fullscreen (topmost, frameless) whatever `[display].windowMode` said. `SetFullscreenState` now refuses exclusive fullscreen and `GetFullscreenState` reports it as granted (the game otherwise retries forever and never renders), `ResizeTarget` is swallowed, an exclusive swap chain is dropped at the first Present, and the configured mode is applied there.
- **Window mode application is idempotent:** re-asserting the mode (the game asking for fullscreen again, `WM_DISPLAYCHANGE`) touches nothing when the window already matches, never steals focus, and never moves a windowed game the user placed. Windowed placement is remembered across a round trip through borderless.
- **Overlay in windowed mode:** ImGui now draws in back-buffer pixels, so the console and the menu are no longer shrunk into the top-left corner (and clicks no longer offset) when the window is smaller than the render resolution.
- **Alt+Enter** no longer toggles repeatedly while held, and the config file is only rewritten on an explicit mode change.

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
