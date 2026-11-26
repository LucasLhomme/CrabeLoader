# VFS mounts, shared order and conflicts

VFS and Lua now consume the same immutable dependency-resolved mod plan. Profile
filtering, duplicate-id assignment, loader compatibility, required dependencies,
conflicts and cycles are resolved before assets are indexed. Later accepted mods
in that resolved order win file conflicts, independently of folder names. An
accepted mod whose Lua subsequently fails remains in the plan; a Lua failure does
not roll back mounts that the engine may already have read.

Startup publishes the plan and index together. A hot reload builds one new plan
for all Lua states; each state uses it when it catches up. Filesystem scan failures
retain the previous index and plan. Malformed manifests are rejected, rather than
silently being treated as legacy mods.

## Explicit asset mounts

A v0 or v1 `mod.json` may declare `assetMounts`. For example:

```json
{
  "manifestVersion": 1,
  "id": "local.example-art",
  "version": "1.0.0",
  "assetMounts": [
    { "source": "assets", "target": "" },
    { "source": "replacement_portraits", "target": "ui/icons" }
  ]
}
```

`source` is a directory inside the mod, and `target` is a virtual game asset
prefix. Thus `replacement_portraits/example.mtb` becomes `ui/icons/example.mtb`.
Mounts are applied in declaration order; a later mount in the same mod wins.
Declaring an empty array mounts no asset folders. Missing source directories are
allowed; an existing source that is not a directory aborts the scan. Traversal,
absolute paths, mod-root mounts and paths escaping a mod through links are rejected.

Without `assetMounts`, conventional locations remain supported:

| Mod directory | Virtual prefix |
|---|---|
| `assets/` | empty |
| `textures/` | `textures/` |
| `ui/` | `ui/` |
| `sound/` | `sound/` |
| `gamedb/` | `gamedb/` |
| `characters/` | `characters/` |

They apply in the order shown. Root entry scripts, documentation and `skilltrees/`
patches are not VFS assets. Loose character definition `.lua` and `.patch` files
under a direct `characters/ -> characters/` mount are also excluded. Game scripts
intentionally shipped inside `assets/` can still be mounted. Native executables,
DLLs and ASI plugins are excluded from every asset mount. Existing texture hash
aliases are retained; within one mod and mount an exact path beats an alias.

An asset-only mod does not need an empty `main.lua` to be recognized as content.
Bank dependency patches remain explicit root-level `bank_dependencies.json` files,
independent of asset mounts, and merge in the same resolved mod order.

## Conflict diagnostics

In the CrabeLoader Lua console:

```lua
Crabe.Vfs.printExplanation("sound/bankloads.bdp")
Crabe.Vfs.printExplanation("ui/icons/hud_playericons_luigi.mtb")
```

For author tools:

```lua
local detail = Crabe.Vfs.explain("ui/icons/example.mtb")
-- detail.path, detail.reason, detail.providers, detail.winner
-- provider: mod (stable ID), folder, physicalPath, order (zero-based), mount, alias, reason
local conflicts = Crabe.Vfs.conflicts("ui/")
```

Explanations include shadowed providers and the selected winner. Generated bank
overlays identify the underlying table provider and the IDs of contributing
patch mods. These queries inspect the published index without reading files or
changing hit counters. They describe precedence, not proof that an engine subsystem
has requested or reloaded the asset. `Crabe.Vfs.list()` retains folder names for
compatibility with existing callers.

Startup/rescan logs summarize conflicts and show up to 64 selected winners.
The API retains the complete conflict list. Identical texture aliases from the
same file and mount are not counted as competing providers.

## Verification

Build and run `test_vfs_framework`, `test_vfs_override`, `crabe_fate` and the Lua
API tests. The encrypted bank integration suite remains under
`tests/python/test_bank_dependencies.py` (requires `pycryptodome`). Restart the
game for startup assets such as sound bank tables; this work does not make every
asset safe to reload while in use.
