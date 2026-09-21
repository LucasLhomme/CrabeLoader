# Changelog

All notable changes to CrabeLoader are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- The loader identifies which build of the game it is running against, instead of
  applying 33 hardcoded addresses to whatever happens to be loaded. Every measured
  address now lives in a `GameProfile` rather than scattered through
  `lua_symbols.cpp` and `memory_patcher.cpp`, and a PE-header check (TimeDateStamp,
  SizeOfImage, CheckSum) decides between three outcomes: load normally on a matched
  profile; run degraded with multiplayer disabled and Lua mods still loading when no
  profile matches but every signature scan resolves; or refuse to patch, leaving the
  game unmodded, when no profile matches and a scan fails. The refusal never kills
  the process — the player loses mods, not their session.

  **The profile's three PE constants ship unmeasured**, so until a human fills them
  in from the shipped executable the loader runs degraded on every machine and
  multiplayer is disabled. See `docs/testing/manual_checklist.md`.

### Added

- Mod load order is resolved from declared dependencies instead of being whatever
  `directory_iterator` happened to yield. `crabe::domain::resolve` is a pure
  function — no I/O, no logging, no Lua, no Win32 — that turns manifests plus the
  loader version into a load order and a typed rejection list, handling required
  and optional dependencies, `provides` capabilities, `loadAfter` / `loadBefore`,
  declared conflicts, duplicate ids (highest version wins), loader-version bounds,
  and cycles, rejecting transitively to a fixed point. Ties break on id, so the
  result is reproducible: a test shuffles 17 mods 100 times and gets the same
  answer every round. The loader now emits a load report, one line per mod.

  **Behaviour change:** load order is now deterministic rather than
  filesystem-dependent, so mods may load in a different order than before. Mods
  that declare no dependencies are ordered by id.
- A manifest that declares no `id` — every v0 manifest — is given
  `local.<sanitised-folder-name>` by the loader, so existing mods take part in
  resolution without needing a manifest rewrite. Collisions are numbered, and a
  declared id always wins over a synthesised one.

### Added

- `mod.json` is parsed as real JSON, with a `SemVer` type supporting comparison
  and the `>=1.2.0`, `~1.2`, `1.x` and `*` range forms. Manifests now distinguish
  three states — valid, malformed, absent — and a malformed one reports the file,
  the line, the column and the byte offset instead of being silently misread.
  The v1 schema adds `id`, `authors`, `license`, `maxLoaderVersion`,
  `dependencies`, `optionalDependencies`, `loadAfter`, `loadBefore`, `conflicts`
  and `provides`; a manifest with no `manifestVersion` is read as v0 and still
  yields only `name`, `version`, `minLoaderVersion` and `entry`, so existing mods
  load unchanged. Unknown keys are recorded rather than rejected.
- A test suite, behind `-DCRABELOADER_BUILD_TESTS=ON` (off by default, so the
  shipped DLL builds exactly as before). Two CTest targets: `crabe_cpp`, a
  doctest binary covering the pure cores of `crabe::memory::patternScan` and
  `CodeCave`'s instruction-boundary measurement; and `crabe_lua_api`, which runs
  `src/api/*.lua` under a real Lua 5.1.5 interpreter, built from the upstream
  tarball — the game's own Lua is a 5.1 build, though the exact patch release is
  not established — in the order `tools/embed_api.py`
  embeds them, and exercises `Crabe.Registry`, `Crabe.Events`,
  `Crabe.Scheduler`, hot reload, and the mod-loading chunks lifted out of
  `src/domain/mod_manager.cpp` at test time. CI runs both.
- `docs/testing/manual_checklist.md`, for the behaviour that can only be
  observed with the game running.

### Fixed

- Manifest values are no longer misread. The previous `mod.json` reader searched
  for `"key"`, then the next colon, then the next two quotes, so a nested object,
  an array, an escaped quote, or the key name appearing inside a value produced a
  **wrong value rather than an error** — and `_valid` accepted input that was not
  JSON at all. Measured against the new fixtures, the old reader answered
  `entry` with the manifest's description sentence when `"entry"` appeared inside
  a `tags` array, truncated a name at its first escaped quote, and returned a
  nested object's `decoy.lua` in preference to the real entry script. That last
  one means it could run a different file than the manifest declared.

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
