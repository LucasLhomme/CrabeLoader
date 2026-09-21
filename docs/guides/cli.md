# crabe-cli — Offline Mod Tooling

`crabe-cli` is a standalone console tool for checking a mod without launching Disney Infinity 3.0. It links CrabeLoader's domain layer only (manifest parsing, SemVer, the dependency resolver) — no Win32 API, no Direct3D, no Dear ImGui, no Lua runtime — so it runs anywhere a CrabeLoader checkout builds, and it is the same logic the loader itself uses, not a reimplementation of it.

It is a developer/CI convenience, not part of the shipped `bink2w32.dll`. Configure with `-DCRABELOADER_BUILD_CLI=ON` to build it (the test suite turns this on automatically); a plain configure without it produces the DLL alone, exactly as before.

```
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -DCRABELOADER_BUILD_CLI=ON
cmake --build build --config Release --target crabe-cli
```

---

## `new-mod` — scaffold a mod

```
crabe-cli new-mod <id> [--name <name>] [--dir <path>]
```

Writes a mod folder the loader actually accepts: `mod.json` (manifestVersion 1), `main.lua` (registering a lifecycle with `Crabe.Mod.register`, the same idiom [the Mod Development Guide](mods.md) and `src/api/03_lifecycle.lua` expect), `i18n/en.toml`, `README.md`, and `.gitignore`.

- `<id>` must satisfy the same pattern every manifest id does: `^[a-z0-9]+(\.[a-z0-9-]+)+$` (e.g. `com.example.my-mod`). Refused otherwise.
- `--name` defaults to a title-cased reading of the id's last segment (`my-mod` → `My Mod`).
- `--dir` defaults to `./<id's last segment>`. `new-mod` refuses to write into a directory that already exists and already has files in it — it never overwrites.

`i18n/en.toml` is a placeholder, not a loader feature: CrabeLoader has no built-in localization loader yet, so nothing reads this file automatically. It exists so a mod's user-facing strings live in one place from day one, ready for a mod to parse itself (or for the platform to grow first-class support later).

Exit codes: `0` scaffolded; `1` on any failure (invalid id, bad usage, a non-empty target directory, or an I/O error while writing).

```
$ crabe-cli new-mod com.example.hello --name "Hello Mod"
new-mod: scaffolded 'com.example.hello' in '.\hello'
  Next: crabe-cli validate .\hello
```

---

## `validate` — check one mod

```
crabe-cli validate <path> [--json]
```

`<path>` may be a mod folder, a `mod.json` file directly, or a standalone `.lua` script. Most schema-level checking — the id pattern, SemVer fields, and that every `dependencies` / `optionalDependencies` / `loadAfter` / `loadBefore` / `conflicts` / `provides` entry is a well-formed id and version range — already happens inside `ModManifest::parse` itself: a manifest that comes back `Valid` has already passed all of that. `validate` adds what parsing a JSON file in isolation cannot know:

- **the entry script exists on disk** — the declared `entry`, or the `main.lua` / any top-level `.lua` fallback the loader itself falls back to (`ModManager::resolveEntryScript`, `loadModDirectory`);
- **what id the loader will actually assign** — a manifest that declares no `id` (every `manifestVersion` 0 mod.json, or a bare directory of `.lua` files) is warned about and shown the `local.<folder>` id `ModManager` would synthesise, via the same `assignModIds`/`synthesiseModId` code the loader runs (`domain/mod_id.hpp`);
- unknown/ignored keys in `mod.json` (warning);
- a `manifestVersion` 0 manifest's version string that is not valid SemVer — tolerated by parsing for backward compatibility, but worth flagging since the resolver cannot compare it against any dependency's version range (warning).

A bare directory of `.lua` files with no `mod.json` at all is a legitimate mod shape, not an error — CrabeLoader has always allowed it. It is only a warning if the folder holds *nothing* runnable either.

**Exit codes:** `0` clean · `1` on error (a structural defect: malformed JSON, a missing entry script, a path that does not exist at all) · `2` on warnings only.

```
$ crabe-cli validate ./hello
validate: ./hello
  id:      com.example.hello
  version: 0.1.0
  entry:   ./hello/main.lua
  OK
```

### `--json`

```json
{
  "target": "./hello",
  "hasManifest": true,
  "id": "com.example.hello",
  "idSynthesised": false,
  "version": "0.1.0",
  "entryScript": "./hello/main.lua",
  "entryScriptExists": true,
  "exitCode": 0,
  "ok": true,
  "findings": [
    { "severity": "warning", "message": "...", "line": 0, "column": 0 }
  ]
}
```

`line`/`column` are `0` when a finding has no position in `mod.json` (most warnings; a malformed-JSON error carries a real position, taken straight from `ManifestDiagnostic`).

---

## `resolve` — run the real dependency resolver

```
crabe-cli resolve <mods-dir> [--json]
```

Discovers `<mods-dir>` and runs `crabe::domain::resolve` over it exactly the way `ModManager::discoverAndLoadMods` does, then prints the load order and every rejection with its reason — answering "why isn't my mod loading?" without starting the game. It does not run anything: crabe-cli never touches Lua, so it reports what *would* load, not whether each entry script runs without erroring once it does.

Three things keep this in step with the loader rather than merely resembling it:

1. **discovery** — the same skip rules (a leading `.` or `_` in a folder name is not a mod) and the same malformed-`mod.json`-becomes-absent fallback, copied from `ModManager::discoverAndLoadMods` steps 1–2;
2. **id assignment** — calls `crabe::domain::assignModIds`, the exact function `ModManager` itself calls (`domain/mod_id.hpp`), not a reimplementation of it. A mod declaring no id gets the same `local.<folder>` id, numbered apart from a collision the same way, in the same order;
3. **resolution** — one call to `crabe::domain::resolve` with the loader's own compiled-in version (`crabe::version::{Major,Minor,Patch}`), the same pure function `ModManager` calls.

**Exit codes:** `0` nothing rejected · `1` otherwise — either the resolver rejected at least one mod (see the output for which and why), or the tool itself could not run (`<mods-dir>` does not exist or is not a directory), disambiguated by the message on stderr rather than by a third exit code.

```
$ crabe-cli resolve ./mods
resolve: ./mods (loader v0.2.0)
  2 to load, 1 rejected
  [1/2] com.example.core 1.0.0 in 'core'
  [2/2] com.example.app 1.0.0 in 'app'
  [--] com.example.old-hud 0.9.0 in 'old-hud' -- not loaded, cyclic dependency: ...
```

### `--json`

```json
{
  "modsDir": "./mods",
  "loaderVersion": "0.2.0",
  "exitCode": 1,
  "ok": false,
  "loadOrder": [ { "id": "com.example.core", "folder": "core", "version": "1.0.0" } ],
  "rejected": [
    {
      "id": "com.example.old-hud",
      "folder": "old-hud",
      "version": "0.9.0",
      "reason": "CyclicDependency",
      "reasonText": "cyclic dependency",
      "detail": "..."
    }
  ],
  "notes": []
}
```

`reason` is one of the `crabe::domain::Rejection` enumerators by name (`MissingDependency`, `VersionMismatch`, `CyclicDependency`, `LoaderTooOld`, `LoaderTooNew`, `DuplicateSupersededId`, `DeclaredConflict`, `InvalidManifest`) — stable for scripting. `notes` carries anything that happened along the way that is not itself a rejection: a malformed `mod.json` that fell back to absent, or a synthesised id that had to be renumbered past a collision.

---

## `check-api` — catch Lua typos before they become a runtime error

```
crabe-cli check-api <path> [--api-def <file>]
```

Scans `<path>` (a `.lua` file, or a directory walked recursively) for calls into `Crabe.*` / `Game.*` that closely resemble a documented function but do not exactly match one — the classic sign of a typo (`Crabe.Mod.regsiter`, `Crabe.wrte`). `<path>` is checked against `docs/crabe_api.def.lua`, embedded into the binary at build time so the tool works on a mod folder anywhere, without a CrabeLoader checkout nearby; `--api-def` points it at a different definitions file instead.

### What it catches, and what it does not — read this before trusting a clean run

`crabe_api.def.lua` documents CrabeLoader's **public** `Crabe`/`Game` surface (`docs/modding.md`, Layer 3). It is *not*, and cannot safely be treated as, an exhaustive list of every real member of those tables: `Crabe.Registry` and `Crabe.Scheduler`, for instance, are real (`src/api/03_lifecycle.lua` calls both) and are not declared there. Flagging "any call under `Crabe`/`Game` this file does not mention" would therefore flag real, working code — exactly the confident false positive that gets a linter switched off.

So `check-api` does something narrower and safer: it only flags a call whose full dotted path is a **small edit distance** from something the definitions file *does* declare (distance ≤ 1 for a short name, ≤ 2 otherwise), and always names the function it thinks you meant. A genuinely undocumented-but-real member is never close enough to anything documented to trip that threshold, so it is left alone. This is a deliberate under-reporting bias: a linter that cries wolf gets ignored, and a missed typo costs you a run of the game to find; a false accusation of a typo on real code costs you the tool's credibility.

Also out of reach, by design, not oversight:

- **calls built at runtime** — `Crabe["wr".."ite"](...)`, a call reached only through a local alias (`local w = Crabe.write; w(...)`), or anything else that is not literally spelled `A.B.C(` in the source;
- **calls whose root is shadowed** — a `local Crabe = ...` or a function parameter named `Crabe`/`Game` anywhere in the file skips checking that root for the *whole file* (named in the output), rather than risk flagging a call that is legitimately going through something else;
- **arity and argument types** — a call with the wrong number of arguments, or the wrong type, is invisible to this tool;
- **the ~900 native engine functions** (`docs/nativedb.md`) — bare, undotted globals like `UI_GetSparks` or `Players_ForceAvatar` are never checked at all, because `crabe_api.def.lua` does not (and cannot practically) catalogue them, and guessing would be almost pure noise.

**Exit codes:** `0` clean · `1` the tool itself could not run (bad target path, an API definitions file with no `function` declarations in it) · `2` at least one call was flagged. Flags sit at the same advisory tier as `validate`'s warnings — never `1` — because a near-miss is a heuristic, not a certainty.

```
$ crabe-cli check-api ./hello
check-api: 1 file(s) scanned against <embedded copy of docs/crabe_api.def.lua>
2 possible typo(s):
  ./hello/main.lua:4:9: `Crabe.wrte` is not declared in crabe_api.def.lua; did you mean `Crabe.write`?
```

There is no `--json` for `check-api` (only `validate` and `resolve` are meant for CI gating in T16); pipe/grep the human output, or add `--json` in a later pass if that turns out to be worth it.

---

## See also

- [Mod Development Guide](mods.md) — the lifecycle `new-mod`'s scaffold registers into.
- [`docs/crabe_api.def.lua`](../crabe_api.def.lua) — `check-api`'s ground truth, and what to extend when you add a new `Crabe.*` function.
- [`docs/nativedb.md`](../nativedb.md) — the engine natives `check-api` deliberately does not check.
