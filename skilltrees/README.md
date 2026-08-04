# skilltrees/

CrabeLoader's own folder for skill-tree / ability overrides — sibling to
`api/` and `mods/`, never the game's own `assets/gamedb/core/` files. Nothing
in here is ever written into the game's install.

## Why this exists as a separate folder instead of a mod

The per-character progression data (`gamedb/core/<ProgressionTree>.lua`,
e.g. `in2_avg_hulk.lua`) is precompiled Lua 5.1 bytecode, loaded through the
same hooked `luaL_loadbuffer` as everything else — but with `name = NULL`
(confirmed live: no path/filename ever reaches the hook for these), and
**before** the game's Lua state is ready, i.e. before `mods/*.lua` gets a
chance to run at all. A live `Crabe.SkillTree.override()` call from a mod is
always too late for this specific data — the engine has already read and
cached it by the time any Lua code of ours executes.

`skilltrees/*` is read directly by the loader's C++ (`Loader::initialize`,
`src/Loader.cpp`) before it installs the `loadbuffer`/`pcall` hooks, so it is
in place in time.

## Two formats, same match-hint idea

There is no filename/path to key off (see above) — content is the only
available anchor for both formats. **The filename (without extension) is the
match hint**: a string that must appear verbatim in the target chunk's own
compiled bytes. A safe choice is one of the chunk's own table keys (an
`Upgrade` name, an `AttributeName`, a `BaseValues` key) — those survive
compilation as literal string constants and are effectively unique to that
one character.

### `*.patch` — a few fields, no extraction (use this by default)

The file's content is plain Lua: a handful of
`ProgressionTable.Foo.Bar = value` assignments. The *original* chunk loads
and runs completely unmodified; the patch then runs immediately after,
against the `ProgressionTable` global it just set (`Loader::armPatchIfMatched`
in `hkLoadbuffer` detects the match at compile time, `LuaCall::runPatch` in
`hkPcall` applies it right after that exact chunk's own call returns). Only
the fields you name are touched — nothing to decompile, edit and keep in
sync with a full 10KB file for a two-line balance change.

Any valid Lua expression works on the right-hand side, including a whole
table literal — so this also covers giving a character an existing ability
node wholesale (`ProgressionTable.Upgrades.SOME_KEY = {UpgradeName=...,
AttributeName=..., Value=..., PCCoreAbility=1, ...}`), not just single
numbers. See `HULK_BASEHEALTH.patch` for a worked example.

### `*.lua` — full chunk replacement (rarely needed)

The file's entire content replaces the original chunk's raw bytes before
compilation (`Loader::findLoadOverride`). Needed only when a patch's
"existing global, assign into it" model doesn't fit — e.g. restructuring
`Grid3`'s layout wholesale. To build one: decompile the original with
`tools/unluac` (`java -jar tools/unluac/unluac.jar <path-to-gamedb-file> >
out.lua`), edit, save here as `<matchHint>.lua`. Lua 5.1 loads plain source
exactly like precompiled bytecode, so nothing needs recompiling.

## Full variable reference, per character

`reference/` has one auto-generated file per real character (104 total,
[reference/README.md](reference/README.md) is the index) listing every
`BaseValues` and `Upgrades` key straight out of the game's own
`gamedb/core/*.lua`, decompiled with `tools/unluac` — the same source
`HULK_BASEHEALTH.patch` was hand-picked from. Use it to find the exact key
and field name to target before writing a `.patch`:

```lua
-- reference/AVG_Hulk.md says HULK_BASEHEALTH.AttributeName = "health", Value = 150
ProgressionTable.BaseValues.HULK_BASEHEALTH.Value = 1000

-- reference/AVG_Hulk.md's Upgrades table says HULK_RANGED_DMG2 exists with
-- PCCoreAbility = 0 (locked behind skill points by default) -- set it to 1
-- to grant the ability for free instead of just changing its numbers:
ProgressionTable.Upgrades.HULK_RANGED_DMG2.PCCoreAbility = 1
```

Not covered: each character's `Grid3` (the skill tree's visual layout --
which node sits where, its connections) is a separate, more deeply nested
structure in the same file that the generator didn't flatten, since it's
about UI placement, not tunable gameplay values. Decompile the character's
own `gamedb/core/*.lua` with `tools/unluac` directly if you need to
restructure that.

## `Crabe.SkillTree.override()` (`src/api/16_skilltree.lua`)

Same underlying full-override mechanism as `*.lua` above, callable live from
a mod (`Crabe._registerLoadOverride`). It will *not* reach gamedb/core files
in time (see above) — it exists for anything that loads after mods run.
There is no live equivalent for `*.patch` yet; add one the same way
(`Crabe._registerSkillTreePatch`) if a use case outside gamedb needs it.
