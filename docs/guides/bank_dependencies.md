# Additive sound bank dependencies (local preview)

Put `bank_dependencies.json` at the root of an unpacked mod directory:

```json
{
  "version": 1,
  "banks": {
    "Example.bnk": ["shared_body.bnk", "Example__dialog_2d.bnk"]
  }
}
```

The target must already exist in the effective `assets/sound/bankloads.bdp` table
and have an installed sound bank. Dependencies must be existing sound filenames
ending in `.bnk`, `.dbnk` or `.dlg`; paths are not accepted. Localized `.dbnk`
and `.dlg` files may also live under `assets/sound/dialog/<language>/`. At least
one installed language must contain the file; this does not guarantee that the
currently selected language has that recording.

During startup and VFS rescans the loader reads the effective table selected by
normal VFS precedence, including an override supplied by another enabled mod.
It merges patch files in shared dependency-resolved mod order, appends dependencies once
(case insensitive), retains original records and attributes, encrypts a generated
copy, and redirects `sound/bankloads.bdp` to that copy. It never rewrites the
source table or installed banks. Profile-disabled folders are excluded from VFS
scans. VFS and Lua use the same accepted mod plan and load order.

Generated snapshots live under `mods/.crabeloader-cache/bank-dependencies/` and
are named by SHA-256 content. The cache is excluded from mod discovery; identical
results reuse a verified snapshot. Close the game before manually removing old
cache snapshots. Removing the patch and restarting restores normal resolution.

A malformed patch, unknown target, missing dependency, unsafe filename or new
dependency cycle rejects the whole overlay generation. The effective unpatched
table remains selected and `BankDependencies:` in `loader.log` explains the
failure. Read diagnostics also show bankloads requests observed by the file hooks.
`Crabe.Vfs.supportsBankDependencyPatches` exposes loader support to Lua mods.
No partial merge is published. If everything requested is already
present, no generated override is needed.

Restart the game for sound tests. A VFS rescan can regenerate the file but does
not unload banks or guarantee the audio engine rereads its startup table.
This feature adds dependency loading only. Character audio IDs, event routing,
bank content and recordings may still need separate fixes. The local preview
has offline validation; in-game sound recovery remains unverified.

## Offline checks

Build `test_bank_dependencies`, then run (Python needs `pycryptodome`):

```powershell
python tests/python/test_bank_dependencies.py build/Release/test_bank_dependencies.exe
```

The suite encrypts fixtures with an independent AES implementation, decrypts
production output and checks it with Python's XML parser. It also exercises VFS
precedence, disabled folders, failure fallback and removal of stale mappings.
