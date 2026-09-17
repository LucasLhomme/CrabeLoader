# Writing a CrabeLoader V2 Mod

> **CrabeLoader V2 Architecture:** The modloader is a generic, neutral C++23 platform, not a per-mod feature factory. Everything a mod needs — calling any engine function, reading/patching memory, hooking inputs, event dispatching, Dear ImGui drawing, and ticks — is exposed as clean primitives.

For dedicated, comprehensive topic guides:
* [Mod Development Guide (Lifecycle, Sandbox, UI, Memory)](guides/mods.md)
* [Skill Tree Modding (Overrides & Patches)](guides/skilltrees.md)
* [Custom Characters & Figurines (Roster Expansion)](guides/characters.md)
* [Guides Index Overview](guides/README.md)
* [Engine Native Database](nativedb.md)

---

## 1. Where Mods, Characters & Skill Trees Live

CrabeLoader discovers content in both global directories and modular mod folders:

```text
Disney Infinity 3.0 Gold Edition/
├── bink2w32.dll              <- CrabeLoader V2 proxy DLL (embedded API)
├── bink2w32_orig.dll         <- Original game Bink DLL
├── mods/                     <- Mods directory (scripts & modular packages)
│   ├── hello.lua             <- Standalone single-file mod
│   └── hero_overhaul/        <- Modular mod folder with manifest
│       ├── mod.json          <- Mod metadata manifest
│       ├── main.lua          <- Primary entry script
│       ├── characters/       <- Mod-bundled character declarations
│       │   └── CustomHero.lua
│       └── skilltrees/       <- Mod-bundled skill tree patches/overrides
│           └── CombatBuff.patch
├── characters/               <- Global character declarations
│   ├── CRABE_MaceWindu.lua   <- Standalone unreleased hero
│   └── CRABE_Thanos.lua      <- Standalone hero
└── skilltrees/               <- Global skill tree modifications
    ├── HULK_BASEHEALTH.patch <- Chunk patch (runs after matching chunk)
    └── tcw_macewindu.lua     <- Source override (replaces chunk before compilation)
```

No manual injector or archive repacking is needed: CrabeLoader scans and activates all three content types automatically:
* **Mods (`mods/`):** Standalone `.lua` scripts and modular folders with `mod.json` execute as soon as the Lua VM is initialized.
* **Characters (`characters/` & `mods/*/characters/`):** Registered into the game's `VirtualReader` catalog, unlocking heroes in the selection grid.
* **Skill Trees (`skilltrees/` & `mods/*/skilltrees/`):** Intercepted on-the-fly during engine bytecode compilation via `luaL_loadbuffer`.


```lua
-- mods/hello.lua
Crabe.Mod.register({
    id = "hello_world",
    name = "Hello World",
    onInit = function()
        Crabe.write("Hello from CrabeLoader V2!")
    end
})
```

---

## 2. API Layers

### Layer 1: The Game's Native Functions (`nativedb.md`)
The engine registers ~900 plain C functions as globals (`UI_GetSparks`, `Players_ForceAvatar`, `Place_SetEditorState`, etc.). You can call them directly from Lua. Refer to [nativedb.md](nativedb.md) for the full catalog.

### Layer 2: `Game.*`
`Game.*` wraps common game functions with parameter safety and descriptive Lua errors on failure.

### Layer 3: `Crabe.*` (V2 Platform Surface)

| Function | What it does |
| :--- | :--- |
| `Crabe.Mod.register(def)` | Registers mod lifecycle callbacks (`onInit`, `onUpdate`, `onDraw`, `onShutdown`). |
| `Crabe.Input.bindKey(vk, callback)` | Binds any virtual key (e.g. `0x74` for F5) to execute a Lua handler. |
| `Crabe.Events.on(event, callback)` | Listens for system events (`"keyDown"`, `"reload"`) or custom cross-mod events. |
| `Crabe.Memory.patternScan(pattern)` | Finds IDA-style byte patterns in memory dynamically. |
| `Crabe.Memory.patchBytes(addr, hex)` | Writes raw executable byte patches with automatic `VirtualProtect` and cache flush. |
| `Crabe.Memory.readFloat(addr)` / `writeFloat(addr, val)` | Reads or writes floating point values in live RAM. |
| `Crabe.Memory.readU32(addr)` / `writeU32(addr, val)` | Reads or writes unsigned 32-bit integers in live RAM. |
| `Crabe.Memory.installCodeCave(addr, hex, [len])` | Installs an x86 code cave with automatic instruction boundary calculation via **HDE32**. |
| `Crabe.Hooks.patchChunk(match, luaCode)` | Runs a Lua patch immediately after a matching game chunk is executed. |
| `Crabe.Hooks.overrideChunk(match, luaCode)` | Replaces an engine chunk source completely before compilation. |
| `Crabe.SetWindowMode(mode)` / `GetWindowMode()` | Switches between Borderless Windowed and Windowed display modes. |
| `ImGui.*` | Native Dear ImGui immediate-mode UI rendering primitives. |

---

## 3. Hot-Reloading (`F4`)

Press **`F4`** in-game at any time to hot-reload all scripts from disk without restarting `DisneyInfinity3.exe`. Press **`Insert`** to view the live developer console and log stream.
