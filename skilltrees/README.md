# skilltrees/

Load-time patches for the game's `gamedb` data. Despite the folder name, the
mechanism is not limited to skill trees: it can reach any chunk the engine
compiles, and is the only way to modify `gamedb` without editing game files.

Nothing here is ever written into the game's install.

**Contents**

- [When to use this instead of a mod](#when-to-use-this-instead-of-a-mod)
- [File formats](#file-formats)
- [Match hints](#match-hints)
- [Constraints](#constraints)
- [Variable reference](#variable-reference)
- [Runtime API](#runtime-api)
- [Troubleshooting](#troubleshooting)
- [Limitations](#limitations)

---

## When to use this instead of a mod

Per-character progression data (`gamedb/core/<ProgressionTree>.lua`, e.g.
`in2_avg_hulk.lua`) is precompiled Lua 5.1 bytecode loaded through the hooked
`luaL_loadbuffer`, but with `name = NULL` and **before the game's Lua state is
ready** — that is, before any `mods/*.lua` can run. A `Crabe.SkillTree.override()`
call from a mod is always too late for this data: the engine has already read and
cached it.

`skilltrees/*` is read by `Loader::loadOverridesFromDisk` during
`Loader::initialize`, before the `loadbuffer`/`pcall` hooks are installed, so the
patches are registered in time.

Use `mods/` for anything that loads after the Lua state is up. Use `skilltrees/`
for `gamedb`.

---

## File formats

Both are keyed by [match hint](#match-hints), not by path.

### `*.patch` — targeted field assignments (default choice)

Plain Lua, executed immediately after the matched chunk's own `pcall` returns,
against the globals that chunk just defined. The original chunk loads and runs
unmodified; only the fields named in the patch are touched.

```lua
-- skilltrees/HULK_BASEHEALTH.patch
ProgressionTable.BaseValues.HULK_BASEHEALTH.Value = 100000
ProgressionTable.Upgrades.HULK_RANGED_DMG2.PCCoreAbility = 1
```

Any Lua expression works on the right-hand side, including a whole table
literal — so granting a character an existing ability node wholesale is a single
assignment.

The same mechanism reaches non-progression data. `tcw_macewindu.patch` walks
`Data.Records` (the `ActorList` global) to repair an actor's `Type`; the same
approach can **add** an actor row without touching any game file.

Implementation: `Loader::armPatchIfMatched` detects the match at compile time,
`LuaCall::runPatch` applies the patch right after that chunk's call returns.

### `*.lua` — full chunk replacement

The file's entire content replaces the original chunk's bytes before
compilation (`Loader::findLoadOverride`). Needed only when "assign into an
existing global" does not fit — for example restructuring a `Grid3` layout.

To build one: decompile the original, edit, save here as `<matchHint>.lua`.

```bash
java -jar tools/unluac/unluac.jar <path-to-gamedb-file> > out.lua
```

Lua 5.1 loads plain source exactly like precompiled bytecode, so nothing needs
recompiling.

---

## Match hints

`gamedb` chunks reach the `loadbuffer` hook with no name (~1500 anonymous chunks
per boot), so content is the only available anchor. **The filename without its
extension is the match hint**: a string that must appear verbatim in the target
chunk's compiled bytes.

Safe choices are one of the chunk's own table keys — an `Upgrade` name, an
`AttributeName`, a `BaseValues` key. Those survive compilation as literal string
constants and are effectively unique to one character.

`HULK_BASEHEALTH` is both the filename and a key inside
`gamedb/core/in2_avg_hulk.lua`, so the match is guaranteed.

A hint may match more than one chunk. Only one patch is armed at a time
(`_armedPatchSource`), and it is consumed after the next `pcall`, so a patch may
run against a chunk you did not intend. **Write every patch to be guarded and
idempotent**: check that the global you expect exists and that the change is not
already applied.

---

## Constraints

### No standard library

Patches targeting `gamedb` chunks run in a Lua state with no base library
loaded. `type`, `string.lower`, `string.find`, `print`, `pairs` and `ipairs` are
all `nil`. Calling one aborts the patch:

```
LuaCall: skill-tree patch failed to run (status 2):
skilltree_patch:33: attempt to call global 'type' (a nil value)
```

Use core syntax only — indexing, comparison, `while`, `#`, `local`:

```lua
if Data and Data.Records then
    local recs = Data.Records
    local i, n = 1, #recs
    while i <= n do
        local rec = recs[i]
        if rec and rec.Name == "tcw_macewindu" then
            rec.Type = "Avatar"
        end
        i = i + 1
    end
end
```

Compare strings against each casing you expect rather than lowercasing them, and
match by equality rather than substring search.

### Patch scope

A `*.patch` affects every chunk matching its hint. A patch against a shared
`ProgressionTree` applies to **all** catalog rows referencing that tree,
including the character who owns it — there is no per-row override.

---

## Variable reference

`reference/` holds one auto-generated file per shipped character (105 files;
`reference/README.md` is the index), listing every `BaseValues` and `Upgrades`
key taken from the game's own `gamedb/core/*.lua`, decompiled with
`tools/unluac`. Use it to find the exact key and field before writing a patch.

```lua
-- reference/AVG_Hulk.md: HULK_BASEHEALTH.AttributeName = "health", Value = 150
ProgressionTable.BaseValues.HULK_BASEHEALTH.Value = 1000

-- HULK_RANGED_DMG2 exists with PCCoreAbility = 0 (locked behind skill points);
-- set it to 1 to grant the ability outright
ProgressionTable.Upgrades.HULK_RANGED_DMG2.PCCoreAbility = 1
```

Not covered: each character's `Grid3` — the tree's visual layout, which node sits
where and its connections. It is a more deeply nested structure in the same file
that the generator did not flatten, since it concerns UI placement rather than
tunable values. Decompile the character's `gamedb/core/*.lua` directly to
restructure it.

---

## Runtime API

`Crabe.SkillTree.override()` (`src/api/16_skilltree.lua`) exposes the same
full-replacement mechanism as `*.lua`, callable from a mod via
`Crabe._registerLoadOverride`.

It will **not** reach `gamedb/core` files in time — see
[above](#when-to-use-this-instead-of-a-mod). It exists for data that loads after
mods run.

There is no live equivalent of `*.patch`. Add one the same way
(`Crabe._registerSkillTreePatch`) if a use case outside `gamedb` needs it.

---

## Troubleshooting

| Symptom | Cause |
|---------|-------|
| `attempt to call global '…' (a nil value)` | the patch used the standard library — see [Constraints](#constraints) |
| `skill-tree patch failed to compile` | Lua syntax error; the message carries the line |
| No `skill-tree patch applied` line at all | the hint never matched — verify the string exists in the target chunk |
| `applied` logged but nothing changed | the patch ran against a different chunk, or its guard rejected the data. Patches cannot log (no `print`), so assign to a global and read it back to confirm |
| Change has no effect in game | the file is shadowed by a `.zip`; compare the loaded chunk size in `loader.log` against the file on disk — see `characters/README.md` |

Expected log lines on a healthy boot:

```
Loader: N load override(s) and M patch(es) registered from skilltrees/.
LuaCall: skill-tree patch applied (<size> bytes).
```

---

## Limitations

- **A brand new tree name is untested.** A `characters/*.lua` entry's
  `ProgressionTree` normally names one of the ~104 shipped trees. Dumping `_G`
  in the menu state found no Lua registry to inject into — the name → data
  resolution is native-side. However, 110 of the 118 shipped `ProgressionTree`
  values map to a `gamedb/core/<lowercase>.lua` file, so supplying a new file
  may work. This has not been verified. Reusing an existing tree is the safe
  option.
- **No per-character override of a shared tree** — see
  [Patch scope](#patch-scope).
- **Patches are silent.** With no `print`, a patch cannot report what it did.
