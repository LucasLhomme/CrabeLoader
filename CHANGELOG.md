# Changelog

All notable changes to CrabeLoader are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
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
- **Settings Screen Race Condition:** Resolved timing issue where game options menu would render blank when injecting options.
- **Multi-State Mod Reload (F4):** Ensured mod reloads are properly dispatched across all active engine Lua states.
- **Hook Registry Safety:** Prevented hook collisions and attributed memory hook ownership.
