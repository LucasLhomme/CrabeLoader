# Iterator compatibility for restricted patch states (local test build)

Before each injected patch, the loader supplies `pairs` and `ipairs` only when
those globals are nil in that patch's target Lua state. Existing functions and
non-nil overrides are preserved. The bootstrap creates state-local closures
using the game's own zero-upvalue `next` and `rawget` wrappers; no Lua object is
transferred between VMs. The incoming Lua stack top is restored after success,
compilation failure and runtime failure.

For PC DI 3.0 Gold profile `di3-gold-steam-1.0`, the expected wrapper RVAs are
`luaB_next = 0x00F20380` and `luaB_rawget = 0x00F20200`. Resolution also requires
an exact registered-function match. An unknown or unmatched address is refused;
the normal patch path remains available with a compatibility warning.

This ports the older local compatibility candidate onto the current fork while
preserving its newer upstream code, loose UI support, bank overlays, shared mod
plan, asset mounts and conflict diagnostics. It does not change CrabeMenu's S9
`MissingAvatarQuitBypass` entry. Keep the user's disabled S9 setting for testing.

The older native test installed iterators and removed their missing-function
errors, while a Toy Box hang persisted. The user subsequently reported that
disabling S9 stopped the hang. Treat these as separate observations; this helper
is not presented as a general Toy Box loading fix.

## Offline validation

Build `CrabeLoader` and `test_patch_iterators` (Win32 Release), then run:

```powershell
ctest --test-dir build -C Release -R '^test_patch_iterators$' --output-on-failure
```

The regression test runs against real independent bare Lua 5.1 states and covers
23 assertions, including sparse arrays, false values, existing definitions,
invalid arguments, repeated initialization and failure stack cleanup.

## Native test

Restart the game with S9 still disabled. Exercise the same Cars/Toy Box sequence
that previously produced `DataMap +` iterator errors. Look for:

- `LuaCall: patch iterators installed in state ... (pairs=..., ipairs=...)`
- Successful patch application without the old missing `pairs`/`ipairs` errors.
- Continued world loading and character selection with S9 disabled.

No installation line is expected when a target state already has both globals.
An installation message proves the bootstrap ran, not that a world loaded.
No VM address cache is used because states may be destroyed, reset or reused.
The small bootstrap is compiled per patch invocation; native loading/performance
and the current combination of mods still need testing.
