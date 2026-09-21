# Writing a CrabeLoader V2 Mod

> **CrabeLoader V2 Architecture:** The modloader is a generic, neutral C++23 platform, not a per-mod feature factory. Everything a mod needs — calling any engine function, reading/patching memory, hooking inputs, event dispatching, Dear ImGui drawing, and ticks — is exposed as clean primitives.

For dedicated, comprehensive topic guides:
* [Mod Development Guide (Lifecycle, Sandbox, UI, Memory)](guides/mods.md)
* [Skill Tree Modding (Overrides & Patches)](guides/skilltrees.md)
* [Custom Characters & Figurines (Roster Expansion)](guides/characters.md)
* [crabe-cli (Offline Scaffolding, Validation & Typo Checking)](guides/cli.md)
* [Guides Index Overview](guides/README.md)
* [Engine Native Database](nativedb.md)

---

## 1. Where Mods Live (`mods/`)

In CrabeLoader V2, **all user content lives strictly inside the `mods/` directory**. There are no loose asset or script folders polluting the root game directory:

```text
Disney Infinity 3.0 Gold Edition/
├── bink2w32.dll              <- CrabeLoader V2 proxy DLL (embedded API)
├── bink2w32_orig.dll         <- Original game Bink DLL
└── mods/                     <- Everything lives inside mods/
    ├── hello.lua             <- Standalone single-file mod
    └── hero_overhaul/        <- Modular mod package
        ├── mod.json          <- Mod metadata manifest
        ├── main.lua          <- Primary entry script
        ├── characters/       <- Mod-bundled character declarations
        │   ├── CRABE_MaceWindu.lua
        │   └── CRABE_Thanos.lua
        └── skilltrees/       <- Mod-bundled skill tree patches/overrides
            ├── HULK_BASEHEALTH.patch
            └── tcw_macewindu.lua
```

No manual injector or archive repacking is needed: CrabeLoader scans `mods/` automatically:
* **Standalone Scripts (`mods/*.lua`):** Executed directly in their own isolated sandboxes.
* **Modular Packages (`mods/<folder>/`):** Read `mod.json`, execute their entry script, register custom characters (`characters/`), and arm skill tree patches (`skilltrees/`).



```lua
-- mods/hello.lua
Crabe.Mod.register({
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
