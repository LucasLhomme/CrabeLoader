# Manual test checklist

Some behaviour can only be observed with Disney Infinity 3.0 running. This file
is where that behaviour is written down, so it is checked deliberately instead
of assumed.

An item belongs here when it cannot be observed without the game: it needs the
engine's Lua state, a real DirectX device, real frame timing, or a real mod
folder on disk. Anything that can be observed without the game belongs in the
automated suite instead (`tests/`, run with `ctest --test-dir build -C Release`)
— a unit test that mocks the engine and then asserts the mock behaved is not a
test of anything.

## How to use this file

1. Build `bink2w32.dll` and drop it into the game folder alongside
   `bink2w32_orig.dll`.
2. Work through every unchecked item under the work orders your change touches.
3. Record the date and the game build you checked against.

Items stay in the file after they pass: a later change can break them again.
Nothing is ever deleted, only re-run.

## Running the automated suite first

```
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -DCRABELOADER_AS_SHARED=ON -DCRABELOADER_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

There is no point checking anything below until that passes.

---

## T5 — mod loading no longer builds Lua by string interpolation

Covered automatically: the chunk text itself, including hostile mod names, in
`tests/lua/test_injection.lua`. What that suite cannot reach is
`LuaCall::runChunkWithArgs`, which needs the game's own `luaL_loadbuffer` and
`lua_pcall` to be hooked before it does anything at all.

- [ ] **A normal mod still initialises.** With a mod in `mods/<name>/`, launch
      the game and confirm the overlay console reports it loading and its
      `onInit` running. Nothing in the log about the module search path.
- [ ] **A mod folder named `Bob's mod` loads.** Rename a working mod's folder to
      `Bob's mod`, launch, and confirm it loads exactly as it did before the
      rename — no Lua syntax error, no "could not extend the Lua module search
      path" line, no silent skip.

## T6 — hot reload revokes mod subscriptions

Covered automatically: the revocation bookkeeping across 50 reload cycles, in
`tests/lua/test_reload.lua`. What that suite cannot reach is frame cost: the
regression it guards showed up as the game getting slower, and only the game
can show that.

- [ ] **Ten reloads do not degrade frame time.** Load a mod that registers a
      `Game.onTick` handler, note the frame time, press F4 ten times, and
      confirm the frame time is where it started. Before the fix each reload
      left the previous generation's handler running, so the tenth frame ran the
      handler ten times.
- [ ] **A reloaded mod's `onTick` still fires.** After those ten reloads,
      confirm the handler is still doing its job — revoking too much looks
      exactly like revoking correctly until something stops working.
- [ ] **`Game.onDeath` still fires after a reload.** With a mod that registers an
      `onDeath` watcher, reload, then die once, and confirm the watcher fires
      once — not zero times (the core death pump was revoked) and not twice (the
      previous generation's watcher survived).

---

## T8/T9 — manifests and load order

- [ ] **Existing mods still load.** `mod.json` is now parsed as real JSON rather
      than scanned for quoted substrings. Launch and confirm every mod in `mods/`
      still appears in the load report with `loaded`.
- [ ] **The new load order is sensible.** Order is now resolved and deterministic,
      sorted by mod id where nothing constrains it, instead of following the
      filesystem. `crabemenu` in particular moves earlier than it used to. Confirm
      nothing depends on the old accidental order.

## T10 — game profile and binary validation

- [ ] **Fill the three PE constants first — this one is a prerequisite, not a
      check.** `src/domain/game_profile.cpp`, the `di3-gold-steam-1.0` entry
      (around line 155), has `timeDateStamp`, `sizeOfImage` and `checkSum` set to
      `kUnmeasured`. Read them off the shipped `DisneyInfinity3.exe` with
      `dumpbin /headers DisneyInfinity3.exe` (or CFF Explorer / PE-bear) and fill
      them in. No agent can obtain these.
- [ ] **Until they are filled, the loader runs degraded on every machine** and
      **multiplayer is switched off**. Lua mods should still load. Confirm that is
      what you see, and that the log says so in as many words.
- [ ] **After filling them, the profile matches.** Launch and confirm the log
      names `di3-gold-steam-1.0`, multiplayer is no longer skipped, and Lua hooks
      still install.

## T11 — configuration and quarantine

- [ ] **F4 and Insert still work with no `crabe.toml` present.** The defaults are
      the values that used to be hardcoded, so a first launch must behave exactly
      as before.
- [ ] **Rebinding takes effect.** Set `[keybinds] hotReload` to another key in
      `Crabe/crabe.toml`, relaunch, and confirm the new key reloads and the old
      one does not.
- [ ] **The window-mode toggle still persists.** Toggle it, restart, and confirm
      the mode survives — it now lives in `crabe.toml`, not
      `crabe_window_mode.cfg`, which is migrated once and then left alone.
- [ ] **A profile disables a mod.** Put a mod's id in a `[profiles.<name>]`
      `enabled` list, set `[general] profile`, and confirm the others do not load.
- [ ] **Quarantine fires on a deliberately broken mod.** A mod whose `onTick`
      always errors should have that callback disabled after ten ticks, and the
      log should say `repeated N times` rather than one line per frame.

## T12 — crash reporter

- [ ] **A broken mod produces a crash report naming it.** Write a mod that
      dereferences nil or otherwise faults, and confirm
      `Crabe/crash-<stamp>.txt` appears next to the DLL with a `.dmp` beside it.
- [ ] **Read the mod attribution carefully — there is a known limit.** A mod that
      faults while *loading* is named in `Active mod`. A mod that faults inside its
      `draw` callback is **not**: Lua picks the mod there, so C++ never learns
      which, and the report shows `Active hook : Loader::dispatchDraw` with
      `Active mod : (none)`. Confirm which case you are looking at before
      concluding the report is wrong.
- [ ] **Thread role is correct for a render-thread fault.** A fault inside
      `Present` should read `Thread role : Render`, not `Script`.
- [ ] **The reporter does not fire on exceptions the game handles itself.** Play
      for a few minutes and confirm no crash report appears and frame time is
      unaffected. The filter is unit-tested, but which codes this game raises per
      frame is something only a play session can measure.

## T13 — hook registry with ownership

Covered automatically: the whole ownership policy, in `tests/cpp/
test_hook_registry.cpp` against `FakeHookBackend` — collision refusal, reverse
order removal, `findCovering` at every boundary, and 100 install/remove cycles
returning the table to its exact prior state. The crash report's attribution is
proved end to end by `ctest -R crabe_crash_report`, which faults for real and
checks the report names the hook covering the faulting address.

What none of that can reach: whether the loader's own hooks still *work*. Every
one of them now goes through the registry, so a mistake there does not fail a
test — it silently stops hooking, and the only symptom is a feature not
happening. Check all four.

- [ ] **The overlay still opens and draws.** Press Insert for the console and F5
      for the mod menu. If `RenderHook::IDXGISwapChain::Present` failed to
      install, the game runs normally and nothing appears at all — no error
      dialog, no visible fault. `Crabe/crabe.log` is where to look: a refusal now
      logs as `HookRegistry: '<name>' ... refused` or `... failed`, with the
      reason spelled out.
- [ ] **Alt-Tab and a resolution change do not crash.** That exercises
      `ResizeBuffers`, whose hook is installed the same way. A black or stretched
      overlay after resizing means the hook went in but the render target was not
      recreated; nothing appearing at all means the hook did not go in.
- [ ] **A controller is still detected.** Plug in a pad and confirm the game
      responds to it, then check `InputHook::XInputGetState` is in the log's hook
      list. The pad working proves nothing on its own — the hook is an observer
      and the game works without it — so this one has to be read from the log.
- [ ] **Mods still load and F4 still reloads them.** Mod loading runs through the
      three `LuaCall::*` hooks; if they failed, no mod runs at all. Press F4 and
      confirm the log says `ModManager: reloading all mods...` followed by the
      mods loading again.
- [ ] **The log lists every hook exactly once, with no collision.** Search
      `Crabe/crabe.log` for `HookRegistry:`. Expect one `installed at 0x...` line
      per hook and **no** `refused` line. A refusal at startup means two of the
      loader's own hooks now overlap — a real defect this registry has just made
      visible for the first time, not a false alarm to be silenced.
- [ ] **A crash report names the hook it died in.** Using the same deliberately
      broken mod as the T12 items, confirm `Crabe/crash-<stamp>.txt` carries an
      `--- Installed hooks ---` section listing each hook with its owner. If the
      fault happened inside the few bytes a hook patched, an `In hook` line
      names it; if it happened in the detour body instead, that line is absent
      and `Active hook` is what identifies it. Both are correct — see the known
      limitation in `CHANGELOG.md` before reporting the absent line as a bug.
- [ ] **Multiplayer, if you use it, still redirects.** The WinHTTP detours go
      through the same registry now, under names like `hook@0x77001234` because
      that subsystem was deliberately not edited by this work order. They should
      appear in the log's hook list and behave exactly as before.

---
## Hot-reload ownership — the onInit leak

- [ ] **A mod that subscribes from `onInit` survives repeated reloads without
      doubling.** Write a mod whose `onInit` calls `Game.onTick(...)` and logs a
      line from that tick callback. Press F4 ten times, then read the log: the
      line must still appear once per frame, not eleven times. Before the fix it
      gained one copy per reload.
- [ ] **`onInit` runs once per reload, not twice.** Log a line from `onInit`
      itself and press F4. Exactly one line per press. Any side effect a mod puts
      in `onInit` — a spawn, a memory patch, a file write — was previously
      applied twice on every reload, so check anything of yours that does.
- [ ] **A tick callback registered in `onInit` that keeps throwing disables its
      own mod, not the API.** Make that callback error every frame. After ten
      frames the quarantine message must name your mod. If it names `core`, the
      ownership bracket is not taking effect in the shipped DLL.

## Adding to this file

New work orders append a section of their own, in the same shape:

```markdown
## T<n> — <what the change was>

Covered automatically: <what the suite already proves, and where>.

- [ ] **<Short claim.>** <How to check it, and what a failure looks like.>
```

Say what a failure looks like, not just what to do. An item nobody can fail is
not worth checking.
