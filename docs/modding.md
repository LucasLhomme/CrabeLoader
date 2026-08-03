# Writing a CrabeLoader mod

**The modloader is a generic platform, not a per-mod feature factory.** Everything
a mod needs — calling any engine function, reading/patching memory, tracing
messages, hooking input, hotkeys, ticks — is already exposed. If you find
yourself wanting a new C++ native just for *your* mod's one feature, stop:
99% of the time the toolkit below already covers it, entirely from Lua. New
C++ in the loader is reserved for genuinely generic platform capabilities
(a new kind of hook nothing else needs), reviewed as such — not shipped
alongside a mod to make its one feature more convenient.

**No raw memory address ever appears in a `mods/*.lua` file, full stop.**
If your mod needs one (an offset, an RVA, a vtable literal), it goes in a
named function in a dedicated `src/api/*.lua` module instead, behind a name
like `Crabe.YourFeature.someInternal()` — the mod calls that, never the
literal. This still isn't C++ — it's the same Lua toolkit, just given a
name and moved one file over so the mod itself stays address-free and
readable.

## Where a mod lives

Drop a `.lua` file in `mods/`. No manifest, no registration call. The loader
runs every file in that folder once the game's Lua state is up and the
`Crabe`/`Game` API has been injected (`Loader::onLoadmods`,
`src/Loader.cpp`) — by the time your file's top level runs, everything below
is already available.

```lua
-- mods/hello.lua
print("hello mod loaded")
```

## Two layers of API

### 1. The game's own natives — call them directly

The engine registers ~900 plain C functions as globals (`UI_GetSparks`,
`Players_ForceAvatar`, ...). Call them like any Lua function. Full catalog:
[nativedb.md](nativedb.md), regenerated with `=Crabe.dump.all()` in the
overlay console.

**Some of these hard-crash the whole process on a wrong argument count
instead of raising a Lua error — `pcall` does not catch that.** Confirm a
native's arity in-game, one call at a time, before it goes anywhere near a
loop or `Game.onTick`. `docs/nativedb.md`'s "To verify" tables show what
this looks like in practice.

### 2. `Crabe`/`Game` — CrabeLoader's own layer

`Game.*` (`src/api/10_game.lua`) wraps common natives with defaults and a
real Lua `error()` on failure. Use what's there; add your own local
wrappers in your mod for whatever it needs beyond that — a mod-specific
helper belongs in the mod, not as a new global.

`Crabe.*` is the platform surface. This is the part that means you never
need new C++:

| Function | What it does |
| --- | --- |
| `Game.onTick(fn)` | Per-frame callback — the *only* event primitive the engine gives you. Build anything edge-triggered (death, state changes, ...) by polling a native here and remembering the previous value; `Game.onDeath` (`src/api/20_hooks.lua`) is the reference example. |
| `OnKeyF1()` … `OnKeyF12()` | Define one of these as a global function in your mod and it fires on that key press — no native needed (`Loader::registerDefaultKeybinds`). |
| `Crabe.inspect.native/rva/bytes/calls/xrefs/strings/dumpNative` | Resolve any native's address, read its raw bytes, list what it calls, find code referencing a string. Ergonomic wrapper over the raw natives below (`src/api/40_inspect.lua`). |
| `Crabe._callThis(fn, this, a1, a2, a3)` / `_callThis1(fn, this, a1)` / `_callThis0(fn, this)` | Call **any** function in the game binary by address, `__thiscall`. Pick the wrapper matching the target's real `ret N` (0/1/3 stack args) — a mismatch corrupts the caller's frame and crashes somewhere unrelated, which the SEH guard cannot catch. A genuine fault (not an arity mismatch) is caught and returns `nil`. |
| `Crabe._readBytes(addr, count)` / `_writeBytes(addr, "AA BB ...")` | Read (≤512 bytes) or write (≤64 bytes) raw game memory. SEH-guarded. Writing pokes at structures whose layout is inferred from disassembly, never documented — a wrong offset is a normal step, not a bug. |
| `Crabe._findString` / `_scanPattern` / `_findCalls` / `_findCallSites` / `_findPointers` / `_moduleBase` | The rest of the RE toolkit: find a string, IDA-style byte-pattern scan, list/find `call` sites, find live heap objects by their vtable pointer, get the image base for RVA math. |
| `Crabe._messageWatch(substring)` / `_messageReport()` / `_messageClear()` | Trace the engine's named-message bus — see what a UI action, native, or your own code actually posts. |
| `Crabe._inputReport()` | Which XInput slots the game has polled and seen connected. |
| `Crabe.SetWindowMode(mode)` / `GetWindowMode()` | The one capability with no game native behind it at all (nothing exposes window/fullscreen state) — necessarily C++, but already generic and available to every mod. |

Together these let a mod call arbitrary engine functions, read and patch
memory, watch messages, and introspect the binary — all without touching
`src/*.cpp`. The splitscreen mod (its own repo, depends on CrabeLoader) is
built entirely this way: every address lookup, every raw call, every memory
read runs from this toolkit alone.

## When C++ actually is the answer

Rare, and it looks specific: something that must run **synchronously inside
a call already in progress on the game's own stack** — an inline hook. Lua
runs on the game thread but can never interpose *inside* a function call the
way a detour can. `MessageHook`/`InputHook`/`RenderHook`/`AvatarRelayHook`
are existing examples, each added to test something no amount of
`_callThis` could reach. If you think you're in this situation, you're
asking for a new **platform** hook, not a mod feature — treat it as a
modloader change to discuss, not something to add quietly alongside your mod.

## Safety notes

- **Arity mismatches are silent killers.** `_callThis`/`_callThis1`/`_callThis0`
  pushing the wrong stack-arg count doesn't fault where you'd expect — the
  caller returns into a shifted frame and dies somewhere unrelated. Read the
  target's disassembly (`Crabe.inspect.dumpNative`) before calling it.
- **Test unconfirmed natives one at a time**, from the console
  (`=Game.X(...)`), not batched inside a mod — a wrong argument can crash
  the process outright, and batching just makes the crash harder to
  attribute.
- **`Game.onTick` runs every frame.** Anything expensive (a scan, a string
  build) belongs behind a cheap early-out, not on the hot path — read a
  cheap flag most frames, and only call a native when something actually
  needs re-asserting.

## A minimal real mod

```lua
-- mods/give_sparks.lua: OnKeyF6 grants 1000 Sparks.

function OnKeyF6()
    Game.AddToInventory("Items.money", 1000)
    print("give_sparks: +1000 Sparks")
end
```

```lua
-- mods/watch_dropin.lua: traces what the engine says about drop-in requests.

Crabe._messageWatch("DropIn")
Crabe._messageWatch("MissingAvatar")

Game.onTick(function()
    local report = Crabe._messageReport()
    if report ~= "" then
        print("watch_dropin: " .. report)
        Crabe._messageClear()
    end
end)
```

Both are complete mods. Neither needed a single line of C++.
