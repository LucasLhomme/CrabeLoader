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

## Adding to this file

New work orders append a section of their own, in the same shape:

```markdown
## T<n> — <what the change was>

Covered automatically: <what the suite already proves, and where>.

- [ ] **<Short claim.>** <How to check it, and what a failure looks like.>
```

Say what a failure looks like, not just what to do. An item nobody can fail is
not worth checking.
