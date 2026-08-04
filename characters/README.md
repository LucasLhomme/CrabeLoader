# characters/

CrabeLoader's own folder for VirtualReader roster changes — sibling to
`api/`, `mods/` and `skilltrees/`.

## Why this is separate from mods/

`Crabe.VirtualReader.addCharacter`/`editCharacter` (`src/api/12_virtualreader.lua`)
work on `VirtualReaderPC_Data.AvatarData`, a plain Lua table the game itself
builds — so in principle a mod calling them from `Game.onTick` should be
enough. Confirmed live that it is not: a mod-time `table.insert` produces a
tile that displays correctly (name/icon/description) but the game still
treats it as unowned (upsell/purchase screen instead of selecting it) —
some native-side catalog build already ran by the time a mod gets to run,
same kind of boot-order problem as the gamedb data (see
`skilltrees/README.md`), just for the roster instead of ability data.

## Why this is separate from skilltrees/

`gamedb/core/*.lua` loads anonymously (`name = NULL`), so `skilltrees/`
has to match on content. `Presentation/VirtualReaderPC_Data.lua` — the file
that builds `VirtualReaderPC_Data.AvatarData` — actually gets a real name at
load time (confirmed live). `characters/` uses that name directly
(`Loader::loadCharactersFromDisk`, hardcoded target
`"Presentation/VirtualReaderPC_Data.lua"`), so there is no arbitrary anchor
string to pick — just write normal Lua.

## Format

Every `characters/*.lua` file is concatenated (each wrapped in its own
`do...end`, so one file's mistakes/locals can't break another) into a single
patch that runs right after `VirtualReaderPC_Data.AvatarData` is built —
`Crabe.VirtualReader` is already loaded by then (confirmed live: mods/API
load before this file does), so just call the normal API:

```lua
-- characters/CRABE_Thanos.lua
Crabe.VirtualReader.addCharacter({
    Name = "Thanos",
    baseCharacter = "AVG_Thor", -- borrows Thor's real sku_id/model
    Icon = "HUD_PlayerIcons_Thor",
    Description = "New identity on Thor's model",
    MetaData = "StarWars,Franchise_EMP",
})
```

```lua
-- characters/AVG_Thor_StarWars.lua
Crabe.VirtualReader.editCharacter("AVG_Thor", {
    MetaData = "Marvel,Franchise_AVG,StarWars,Franchise_EMP",
})
```

Filenames are just for organization — nothing reads them.

## Selectability — handled automatically, no extra step needed

A synthetic `addCharacter` row draws a correct-looking tile immediately, but
the grid still shows it as locked (upsell/purchase screen on click) by
default. This is **not** controlled by any row field — `IsLocked`,
`IsTrialPlayable`, `SteamDLCAppId`/`PCSKU`/`WINRTSKU`, and calling
`VirtualReaderPC_SetCurrentCharacter` directly were all tried and confirmed
live to have no effect. The actual gate is plain Lua in the grid *screen*
itself (`presentation/virtualreaderpc_gridcharacter.lua`'s
`DoSelectGridItem`/`GetGridButtonState`), found by diffing that file's
vanilla decompile against the shipped "Breeze" DI3 mod's patched copy of the
same file — Breeze hardcodes the lock check to `false` instead of reading it
back from the grid's list data.

`Crabe.VirtualReader.installGridUnlock()` ports that exact patch as a
runtime monkey-patch (`src/api/12_virtualreader.lua`, same technique
`mods/window_mode.lua` uses on `SettingsVideo:BuildList`) and runs
unconditionally as part of the API — every `characters/*.lua` entry is
selectable with no extra mod, toggle, or file to remember.

See [CUSTOM_MODELS.md](CUSTOM_MODELS.md) for the (paused) research trail on
whether a synthetic entry could get its own 3D model instead of always
rendering as `baseCharacter` — concrete findings so far, and exactly what's
still missing.

## Hard limits (still true, no way around these from Lua)

- **No new 3D model.** `baseCharacter`/`sku_id` must name a real, existing
  character — the model, animations and voice are always exactly that
  character's. "Adding Thanos" today means an existing character (Thor,
  Hulk, whoever) under a new name/description, not a new purple giant.
  Real new geometry is an asset pipeline problem (new `.dnax`/`.mtb`/
  textures), well outside this API — see how `tools/breeze` restores
  Disney's own left-out characters for what that actually involves.
- **Icon must be a real, existing `HUD_PlayerIcons_*` asset.** Reuse the
  base character's own icon, or another real character's — a genuinely new
  icon image means dropping a new texture into the game's UI assets, not
  something `addCharacter` can do by itself.
- **Own gameplay is still possible**: give the new identity its own
  `ProgressionTree` (see `skilltrees/README.md`) for different abilities,
  even while it shares a base character's model.
