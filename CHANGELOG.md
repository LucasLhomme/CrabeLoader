# Changelog

All notable changes to CrabeLoader are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Security

- Mod directory and file names are no longer interpolated into Lua source. A mod
  folder called `Bob's mod` used to produce an incomprehensible Lua syntax error,
  and a crafted folder name could execute arbitrary Lua **outside the sandbox**,
  before `Crabe.Sandbox.create` was ever called — and folder names come from
  downloaded archives. Paths and mod names are now pushed onto the Lua stack and
  received by a compile-time constant chunk as varargs, so caller data never
  reaches the Lua parser.
- The loader no longer falls back to running a mod **unsandboxed** when
  `Crabe.Sandbox` is missing. A missing sandbox module now refuses the mod with a
  message naming what is unavailable. Silently running mods unsandboxed is worse
  than not loading them.

### Fixed

- Hot-reload no longer leaks subscriptions. `Crabe.Mod.reload()` cleared the mod
  registry but left `Game._tickCallbacks`, `Game._deathWatchers`,
  `Crabe.Events._listeners` and `Crabe.Mod._reloadCallbacks` untouched, so after
  N reloads every mod's `onTick` ran N times per frame and every event was
  dispatched N times. Measured against a stock Lua 5.1 interpreter over the real
  API sources: 50 reload cycles with three mod subscriptions grew the three
  registries from `(2, 0, 0)` to `(53, 51, 51)`; they now stay flat and return
  exactly to baseline.

  Registrations are recorded against the mod that made them by the new
  `Crabe.Registry` (`src/api/02b_registry.lua`), and reload revokes them.
  Registrations made while no mod is executing belong to `"core"` and survive —
  which keeps the death-watcher pump in `20_hooks.lua` alive across reloads.

  **Known limitation:** a mod that registers from its `onInit` callback — the
  documented primary idiom — is still not covered, because `dispatchInit()` runs
  after the mod chunk with no current owner, so those subscriptions fall to
  `"core"`. Measured leak is about one callback per mod per reload, down from
  one per subscription per reload.

### Changed

- **Breaking for out-of-tree C++ code:** every namespace now lives under `crabe::`,
  lowercase, with layer sub-namespaces. `Crabe::Domain` becomes `crabe::domain`,
  the parallel `Multiplayer::*` hierarchy folds into `crabe::multiplayer::*`, the
  flat `Memory` / `LuaSymbols` / `LuaRuntime` / `InputNatives` become
  `crabe::memory` / `crabe::lua_symbols` / `crabe::lua_runtime` /
  `crabe::input_natives`, and the formerly global types (`Loader`, `LuaCall`,
  `Logger`, `RenderHook`, `CodeCave`, `CrashHandler`, `InputHook`, `MessageHook`,
  `Overlay`, `ImGuiBindings`, `Hook`, and the `LogLevel` / `LogEntry` /
  `WindowMode` / `LuaApiAddresses` types) move into their layer's namespace.
  The DLL's 77 exported symbols are unchanged, so the game side is unaffected.
- All source and header file names are `snake_case`. `Loader.cpp` becomes
  `loader.cpp`, `luacall.*` becomes `lua_call.*`, `codecave.*` becomes
  `code_cave.*`, and the `I*.hpp` interfaces become `i_*.hpp`. Types keep their
  `PascalCase` names.

### Removed

- `Loader::loadModDirectory` and its private `resolveEntryScript` helper. Both were
  dead code duplicating the live implementation in `ModManager`, which is the only
  mod-loading path actually reached from `Loader::onLoadmods`. No behaviour change.

## [0.2.0]

- Baseline release. See the repository history for details.
