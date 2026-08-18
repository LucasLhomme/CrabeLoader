# characters/

Roster definitions. Files here add or modify entries in the character-select
grid.

Every `characters/*.lua` file is concatenated — each wrapped in its own
`do...end` — into a single chunk that the loader runs immediately after the game
builds `VirtualReaderPC_Data.AvatarData`. That is the only window in which the
catalog can still be modified: `Presentation/pressstart.lua:59` hands the table
to native code via `VirtualReaderPC_SetData`, after which it is frozen.

Filenames are not read by anything. Only the `.lua` extension matters
(`Loader::loadCharactersFromDisk` skips every other file).

**Contents**

- [Quick start](#quick-start)
- [API reference](#api-reference)
- [Data model](#data-model)
- [Procedure: adding a character](#procedure-adding-a-character)
- [Constraints](#constraints)
- [Optional tooling](#optional-tooling)
- [Troubleshooting](#troubleshooting)
- [Reference data](#reference-data)
- [Limitations](#limitations)

---

## Quick start

Reskin an existing character — one file, no other step:

```lua
-- characters/CRABE_Thanos.lua
Crabe.VirtualReader.addCharacter({
    Name = "Thanos",
    baseCharacter = "AVG_Thor",          -- existing catalog Name; its sku_id is reused
    Icon = "HUD_PlayerIcons_IronMan",
    Description = "New identity on Thor's model",
    MetaData = "Marvel,Franchise_AVG",
})
```

Surface a character that ships with the game but has no catalog row — also one
file, no other step:

```lua
-- characters/CRABE_MaceWindu.lua
Crabe.VirtualReader.exposeCharacter({
    Name = "TCW_MaceWindu",              -- must match an ActorList actor
    ProgressionTree = "IN3_TCW_MaceWindu",
    Description = "Upsell_TCW_MaceWindu_desc",
    MetaData = "StarWars,Franchise_TCW",
})
```

No `sku_id`, no external tool, no game file modified: the loader derives an id
from `Name` and injects the matching figure registry slot into memory before the
game reads the gateway.

Deploy by copying `api/`, `characters/` and `skilltrees/` into the game folder.

---

## API reference

Defined in `src/api/12_virtualreader.lua`. Call these from `characters/*.lua`,
never from `Game.onTick` — a mod runs after the catalog is frozen.

### `Crabe.VirtualReader.addCharacter(entry) -> row`

Adds a catalog row that reuses another character's model and `sku_id`.

| Field | Required | Notes |
|-------|----------|-------|
| `Name` | yes | display identity; any string |
| `baseCharacter` | yes¹ | existing catalog `Name`; its `sku_id`, `Icon`, `ProgressionTree`, `MetaData` and store SKUs are inherited |
| `sku_id` | yes¹ | alternative to `baseCharacter`: an existing `sku_id` |
| `Icon`, `Description`, `MetaData`, `ProgressionTree` | no | override inherited values |

¹ exactly one of `baseCharacter` / `sku_id`.

Raises if `Name` is missing, if `baseCharacter` does not resolve, or if no
`sku_id` could be determined.

### `Crabe.VirtualReader.exposeCharacter(entry) -> row`

Adds a catalog row for a character whose actor, `.dnax`, assets and skill tree
already exist. The model, animations, voice and abilities are that character's
own.

| Field | Required | Notes |
|-------|----------|-------|
| `Name` | yes | **must match an ActorList actor with `Type = "Avatar"`**, compared lowercased |
| `sku_id` | no | omit it — the loader derives a stable id from `Name` and registers it. Pass one only to pin a specific value |
| `ProgressionTree` | recommended | name of a real `assets/gamedb/core/<x>.lua` |
| `Icon` | no | omit to fall back to `HUD_PlayerIcons_Default` |
| `Description`, `MetaData` | no | localisation key and brand/filter tags |

Raises if `Name` is missing, if `baseCharacter` is passed, or if the `Name` or
the resulting `sku_id` is already in the grid.

`Name` cannot be validated at call time: the ActorList lives in the world/gamedb
Lua state, not the front-end state this code runs in. Verify it by hand against
`assets/gamedb/core/*_actors.lua`.

### `Crabe.VirtualReader.skuForName(name) -> string`

The `sku_id` `exposeCharacter` would allocate for that name. Same formula as
`Gateway::allocateSku` in `src/gateway.cpp` — see
[`sku_id` allocation](#sku_id-allocation). Useful to check a value before
pinning it, or to detect a collision with a shipped row.

### `Crabe.VirtualReader.editCharacter(nameOrSkuId, patch) -> row`

Mutates an existing row in place. `sku_id` is left alone, so selection keeps
working. Use to re-tag, re-icon or relabel a shipped character.

### `Crabe.VirtualReader.listCharacters()` / `findCharacter(nameOrSkuId)`

Return the live `AvatarData` table, and a single row by `Name` or `sku_id`.
Both return `nil` in a Lua state without `VirtualReaderPC_Data`.

### `Crabe.VirtualReader.installGridUnlock()`

Disables the grid's lock check so every tile is selectable. Called
unconditionally from `src/api/21_virtualreader_unlock.lua`; callers do not need
to invoke it. Row fields `IsLocked`, `IsTrialPlayable`, `SteamDLCAppId`,
`PCSKU` and `WINRTSKU` have no effect on selectability.

---

## Data model

A playable character is six records across six files. All six must agree.

| # | Layer | Location | Contents |
|---|-------|----------|----------|
| 1 | Catalog | `assets/presentation/virtualreaderpc_data.lua` | `VirtualReaderPC_Data.AvatarData` — 127 rows, 11 uniform fields |
| 2 | Actor list | `assets/gamedb/core/<zone>_actors.lua` | `Data.Records` — `Name`, `Type`, `SubType`, `Parms` (`DNAFile=…`) |
| 3 | Actor definition | `assets/characters/<name>.dnax` | Lua 5.1 **bytecode** building a `DNA` table of ~90 components |
| 4 | 3D assets | `assets/characters/<name>/<name>.zip` | mesh, skeleton, textures; AES-128-CTR |
| 5 | Progression | `assets/gamedb/core/<progressiontree>.lua` | `ProgressionTable` — `BaseValues`, `Upgrades`, `Grid3` |
| 6 | Figure registry | `assets/gateway*.lua` | encrypted `sku_id` → character record |

### Resolution order

```
sku_id  →  AvatarData row (pushed native by VirtualReaderPC_SetData)
        →  Name
        →  Name:lower() in the ActorList, Type = "Avatar"
        →  that row's DNAFile  →  assets/characters/<x>.dnax
```

**`Name` resolves the model, not `sku_id`.** A `sku_id` that appears nowhere in
the shipped data works, provided its row's `Name` designates a real actor.
Neither the `.dnax` filename nor its `Identification.Name` field participates in
resolution.

### Layer 6 — the gateway

Five files (`gateway.lua`, `_in3`, `_mba`, `_moa`, `_dor`), each holding the same
374 slots, 116 of them `TagType = "AVATAR"`. Each slot's key is the encrypted
`sku_id`. Decrypted, an avatar slot is:

```
[1000201]
   name        = TCW_ObiWan          -- the catalog Name
   sku         = 4                   -- IEEE-754 double
   version     = 3
   TagType     = AVATAR
   keysum      = -2081114000         -- signed int32 stored as a double
   displayName = IGP_ObiWan
   playset     = TheCloneWars,Empire:CC_EMP_OBIWAN,PlaysetX:CC_PSX_OBIWAN
```

Without a slot for a given `sku_id`, the game refuses to play the character and
shows *“Figurine Disney Infinity manquante”*, regardless of layers 1–5.

**The loader supplies that slot itself.** `src/gateway.cpp` reimplements the
encryption in C++, builds the slot as Lua source, and hands it to
`registerSkillTreePatch`, which runs it immediately after the gateway chunk
loads. The registry is modified **in memory only** — the five `gateway*.lua`
files are never written to, and it works whether the chunk came from a loose
file or from `startup.zip`.

**Encryption** — AES-128-CBC, key `000102030405060708090a0b0c0d0e0f`, IV
`0123456789012345` (UTF-8), PKCS7. The fixed IV makes it deterministic:
identical plaintexts produce identical ciphertexts across slots. Each plaintext
carries a leading type byte — `0x01` number (little-endian double follows),
`0x02` string, `0x03` bool.

**`keysum`** is not validated. Copy the clone source's value; it may be shared
between slots. Its derivation is unknown — CRC32, djb2, sdbm, FNV-1a and byte
sums all fail against the 116 shipped samples.

**`playset`** may name another character's codes without effect in Toy Box.

---

## Procedure: adding a character

Decide the route first: does the character already ship with a model? Search
`assets/gamedb/core/*_actors.lua` for its actor name. If a row exists with
`Type = "Avatar"` and the `.dnax` it names is on disk, use route B.

### Route A — reskin

1. Write `characters/<name>.lua` calling `addCharacter` with `baseCharacter`.
2. Deploy `characters/`.
3. Launch.

No gateway slot, no archive handling. The character shares the base's `sku_id`,
model and progression.

### Route B — expose a shipped character

**Step 1 — verify the character is complete.**

| Check | Location |
|-------|----------|
| actor row with `Type = "Avatar"` | `assets/gamedb/core/*_actors.lua` |
| the `.dnax` it names | `assets/characters/<name>.dnax` |
| 3D assets | `assets/characters/<name>/` |
| skill tree | `assets/gamedb/core/in3_<name>.lua` |

A missing tree is not fatal — reuse another character's `ProgressionTree`; the
character loads with those abilities.

**Step 2 — write `characters/CRABE_<Name>.lua`.** A single `exposeCharacter`
call naming the actor. Nothing else: the `sku_id` and the figure registry slot
are handled by the loader at boot.

**Step 3 — deploy** `api/`, `characters/`, `skilltrees/` into the game folder.
`test_game.ps1` does this, but it also rebuilds the DLL and ends on a blocking
log tail; a plain copy suffices when no C++ changed.

**Step 4 — verify** in `loader.log`:

- `Loader: N character definition(s) registered from characters/`
- `Loader: figure registry slot for '<Name>' -> sku <id>`
- `LuaCall: skill-tree patch applied (...)` — one for the slot, one per `skilltrees/*.patch`
- no `[ERROR]` lines

Then enter a Toy Box, open *My Collection*, and select the tile.

Read those lines for what they are: they prove the slot was *built* and the
patch *ran*. The injected code is guarded by `if container then … end`, so a
failed container lookup produces an identical log and no slot. The grid is the
only real confirmation.

### Reference implementation — `CRABE_MaceWindu.lua`

Mace Windu shipped as a Clone Wars mission giver: complete gameplay, no
shop-facing art, no catalog row — layers 2 to 5 all present, layer 1 missing:

- actor `tcw_macewindu` → `characters/TCW_MaceWindu.dnax`, `Type = "Avatar"`
- `tcw_macewindu.dnax` — `ActorTypeMask = "AV_MaceWindu"`, `DialogID = "MCE"`,
  `LoadAudio = "TCW_MaceWindu"`
- `characters/tcw_macewindu/` — 8.1 MB: model, robe, destruction pieces,
  team-up, mission-giver variant
- `gamedb/core/in3_tcw_macewindu.lua` — `MACEWINDU_FINISHER`,
  `MACEWINDU_RICOCHET_LIGHTSABER`, `MACEWINDU_SUPERJUMP`,
  `MACEWINDU_CHARGEATTACK`, six ground combos

Applied: one `characters/CRABE_MaceWindu.lua`, twelve lines of it declarative.
The loader derived `sku_id` 1000444 from the name and injected the slot in
memory; `startup.zip` and the five `gateway*.lua` files stayed vanilla. Verified
in game on 2026-08-18 — the character loads with his own model. No binary patch
of any kind, and nothing written to the player's install.

Two cosmetic gaps remain: no `HUD_PlayerIcons_MaceWindu` asset ships, and the
description key has no entry in `assets/lang/main/*.dct`.

---

## Constraints

### Archive precedence

When a path exists both as a loose file under `assets/` and inside a shipped
`.zip`, **the engine reads the archive**. Editing the loose file has no effect.

Verify which copy is loaded by comparing the compiled-chunk size in
`loader.log` against the files on disk:

| File | loose | archive | loaded |
|------|-------|---------|--------|
| `gateway.lua` | 406768 | 64800 | 64800 while `startup.zip` exists |
| `toybox_actors.lua` | 470303 | 248965 | 248965 (`core.zip`) |

This no longer concerns the gateway: the slot is patched into the chunk in
memory after it loads, whichever copy the engine picked. It still applies to
anything that has to change on disk.

To make loose files authoritative, extract the archive and remove it. Breeze's
installer does this via a `QuickBMSExtract` action with
`"DeleteWhenComplete": true` over `startup.zip`, `startup2.zip`,
`presentation.zip` and `main.zip`, before replacing `gateway.lua`.

### No standard library in the gamedb Lua state

`skilltrees/*.patch` files targeting gamedb chunks run in a state where `type`,
`string.*`, `print`, `pairs` and `ipairs` are all `nil`. Use core syntax only:
indexing, comparison, `while`, `#`.

This is also the mechanism for editing an ActorList without touching game files:
a patch can add to or modify `Data.Records` after the chunk loads.

### gamedb chunks load anonymously

No filename reaches the `loadbuffer` hook for gamedb data (~1500 anonymous
chunks per boot), so `skilltrees/*.patch` matches on chunk *content* — the
filename stem is the match hint. Pick a string that is unique to the target
chunk, and make the patch idempotent and guarded, since a hint may match more
than one chunk.

### Lua decimal escapes

Lua consumes up to three digits after a backslash: `"\11"` followed by a literal
`9` reads back as `\119`. Always pad to three digits (`\011`). `di3_gateway.py`
pads and re-parses every literal it emits.

---

## Optional tooling

`tools/di3_gateway/di3_gateway.py` reads and edits gateway files. **The procedure above does not
use it.** The loader performs the encryption itself, in C++, at boot; Python is
needed neither to author a character nor to play one. Keep the script for
inspection, and for the rare case where a slot must exist on disk.

No third-party package required: uses `cryptography` or `pycryptodome` when
installed, otherwise a built-in AES-128.

```bash
cd tools/di3_gateway
python di3_gateway.py check   --game "<game>"                 # verify crypto round-trips
python di3_gateway.py list    --game "<game>"                 # AVATAR slots
python di3_gateway.py dump    --game "<game>" --filter Windu  # all slots, filtered
python di3_gateway.py freesku --game "<game>" [--name <Name>] # unused sku_ids
```

`--file <path>...` targets specific files instead of discovering
`assets/gateway*.lua`.

**`check`** re-encrypts every slot key, field name, string and number and
compares against the file. If it does not report `OK` for every file, the format
has changed and nothing else should be trusted.

`register` and `add` also exist and write slots into the files — dry runs
unless `--write`, backing up to `backups/gateway_<sku>_<timestamp>/` first, and
skipping files that already contain the `sku_id`. They predate the in-memory
path and are no longer part of the normal procedure. A slot written on disk for
a character the loader also injects is redundant but harmless.

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| *“Figurine Disney Infinity manquante”* | no gateway slot for that `sku_id` | confirm `figure registry slot for …` in `loader.log`; if it is there, the container lookup inside the patch failed silently |
| Tile appears, character never loads | `Name` matches no ActorList actor with `Type = "Avatar"` | correct `Name`, or add the actor via a `skilltrees/*.patch` |
| Tile does not appear at all | the `characters/*.lua` chunk did not run | check for `character definition(s) registered` in `loader.log` |
| Edit appears to do nothing | a `.zip` shadows the loose file | compare loaded size against on-disk size |
| `attempt to call global '…' (a nil value)` | a `skilltrees/*.patch` used the standard library | rewrite using core syntax only |
| Tile shows as locked | `installGridUnlock` did not run | confirm `src/api/21_virtualreader_unlock.lua` is deployed |

---

## Reference data

### Shipped complete, no catalog row

Actor, `.dnax`, own assets and own skill tree all present:

| Actor | Assets | Tree | Icon ships |
|-------|--------|------|------------|
| `ant_antman` | 9.2 MB | `in3_ant_antman` | yes |
| `tcw_macewindu` | 8.1 MB | `in3_tcw_macewindu` | no |
| `dor_dory` | 0.4 MB | `in3_dor_dory` | yes |
| `dor_nemo` | 0.4 MB | `in3_dor_nemo` | yes |
| `emp_r2d2` | 0.3 MB | `emuinfo_emp_r2d2` | no |

Around eleven more have their own assets but no dedicated tree — alternate
costumes such as `emp_luke_flightsuit` (8.4 MB), `emp_hansolo_carbonite`
(8.0 MB), `gog_gamora_armor` (6.6 MB), `avg_falcon_movie` (6.2 MB),
`tbx_mickey_kingdomhearts` (3.6 MB). Reuse the base character's tree.

### Costume variants

A variant `.dnax` differs from its base by two assignments —
`DefaultCostume` and `dynexp_prebuiltName` — and shares `ModelName`. Costumes
themselves are partial `DNA` overlays (53 `*_costume*.dnax` files ship).

### `sku_id` allocation

Shipped rows use 1000001–1000029, 1000100–1000129 and 1000302–1000338. Breeze
takes 1000300/1000301 for Dory and Nemo. Modded characters are allocated from
1000340–1000999, clear of both:

```
h = 0
for each byte b of Name:  h = (h * 31 + b) % 2147483648
sku = 1000340 + h % 660
```

Multiply-and-add only, no XOR, so `src/gateway.cpp` and
`src/api/12_virtualreader.lua` agree — Lua 5.1 has no bitwise operators. Being
derived from the name, the id is stable across installs, which is what makes a
`characters/*.lua` file portable: sharing it does not require sharing anything
else. Two authors collide only by choosing the same character name.

`freesku` lists ids unused by both the gateway and the catalog, for the case
where a derived id has to be overridden.

### Known data defects

Eight shipped rows name a `ProgressionTree` with no file behind it —
`AVG_AntMan` points at `IN3_AVG_AntMan` while only `in2_avg_antman.lua` and
`in3_ant_antman.lua` exist. Do not copy a tree name from a row without checking
`assets/gamedb/core/` for it.

---

## Limitations

- **New 3D geometry** is out of scope for this API. A character with no shipped
  assets needs new `.bent`/`.oct`/`.mtb`/textures and a repack of the encrypted
  archives; QuickBMS reimport fails on character `.zip` files, `offsetting.exe`
  is the working path. The `Biped` rig is shared by all humanoids, so animation
  is inherited.
- **New icons** require a `.mtb` in `assets/ui/icons/`. Omitting `Icon` is
  sufficient for testing.
- **A new `ProgressionTree` name** is untested. 110 of the 118 shipped values map
  to a `gamedb/core/<lowercase>.lua` file, so a new file is likely to work, but
  this has not been verified. Reusing an existing tree is the safe option.
- **A modified `.dnax`** is untested.

