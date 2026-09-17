# CrabeLoader V2 — Mod Development Guide

Welcome to the **CrabeLoader V2** Mod Development Guide.

CrabeLoader V2 introduces a decoupled, industry-standard **SOLID architecture**:
* **CrabeLoader (C++23):** Acts purely as a stable, neutral infrastructure platform. It hooks DirectX 11, captures hardware faults via Structured Exception Handling (SEH), provides an isolated Lua runtime, and exposes low-level memory and graphics primitives.
* **Mods (Lua 5.1):** Carry 100% of game-specific logic, cheats, camera controls, custom mechanics, and user interfaces.

---

## 1. Quick Start

### Mod Directory Layout
Mods live in the `mods/` directory located in the root of your Disney Infinity 3.0 installation:
```text
Disney Infinity 3.0 Gold Edition/
├── bink2w32.dll              <- CrabeLoader V2 proxy DLL
├── bink2w32_orig.dll         <- Original game Bink DLL
├── api/                      <- CrabeLoader runtime Lua library
├── mods/                     <- Mods directory
│   ├── simple_mod.lua        <- Standalone single-file mod
│   └── my_complex_mod/       <- Folder-based mod
│       ├── mod.json          <- Mod manifest
│       ├── main.lua          <- Entry script
│       └── modules/          <- Internal scripts
```

A mod can be either a single `.lua` file dropped directly into `mods/`, or a dedicated subfolder with a `mod.json` manifest.

---

## 2. Mod Manifest (`mod.json`)

For multi-file mods, create a `mod.json` file in your mod folder:

```json
{
  "name": "My Custom Mod",
  "id": "my_custom_mod",
  "version": "1.0.0",
  "minLoaderVersion": "0.2.0",
  "entry": "main.lua",
  "author": "YourName",
  "description": "Custom gameplay enhancements and UI for Disney Infinity 3.0"
}
```

* `minLoaderVersion`: Ensures your mod is only loaded if the installed CrabeLoader is compatible.
* `entry`: Specifies the primary Lua script to execute on startup (defaults to `main.lua`).

---

## 3. Mod Lifecycle (`Crabe.Mod.register`)

Every modern mod in V2 registers its lifecycle callbacks with the mod manager:

```lua
Crabe.Mod.register({
    id = "my_custom_mod",
    name = "My Custom Mod",

    -- Called once when the game's Lua runtime is initialized
    onInit = function()
        Crabe.write("My Custom Mod initialized successfully!")
    end,

    -- Called periodically (~60 Hz / 16ms) on the game's script thread
    onUpdate = function(dt)
        -- Perform per-frame gameplay logic
    end,

    -- Called on the DirectX 11 render thread between ImGui::NewFrame and ImGui::Render
    onDraw = function()
        -- Render ImGui windows and overlays
    end,

    -- Called when the mod is unloaded or when hot-reloading (F4)
    onShutdown = function()
        -- Clean up resources, hooks, and temporary state
    end
})
```

---

## 4. Deep-Frozen Sandbox Protection

In CrabeLoader V2, every mod runs within an **isolated sandbox environment**:
* **Variable Isolation:** Global assignments like `myVar = 42` only affect your mod's local environment table (`_ENV`).
* **Read-Only Standard Tables:** Critical tables (`Game`, `table`, `string`, `math`, `coroutine`, `os`, `debug`, `_G`) are protected by read-only proxy metatables.
* **Tamper Prevention:** If a mod attempts to overwrite engine functions (e.g. `Game.UnlockCatalogItem = nil`), CrabeLoader traps the write, logs a security violation with the mod's ID, and raises an error without corrupting other mods or the game engine.

### Inter-Mod Communication (`Crabe.Sandbox.export`)
To share an API or object with other mods safely:

```lua
-- In Mod A:
Crabe.Sandbox.export("MySharedService", {
    sayHello = function(name)
        return "Hello, " .. tostring(name)
    end
})

-- In Mod B:
local service = Crabe.Exports.MySharedService
if service then
    print(service.sayHello("World"))
end
```

---

## 5. In-Game UI Creation (Dear ImGui)

CrabeLoader V2 exposes Dear ImGui primitives directly to Lua. To create an interactive UI, place your code in the `onDraw` callback:

```lua
local isWindowVisible = true

Crabe.Mod.register({
    id = "sample_ui",
    name = "Sample UI",
    onDraw = function()
        if not isWindowVisible or not ImGui then return end

        ImGui.SetNextWindowSize(380, 240, 4) -- ImGuiCond_FirstUseEver
        if ImGui.Begin("My Mod Window", true) then
            ImGui.TextColored(0.2, 0.8, 1.0, 1.0, "Welcome to Disney Infinity 3.0 Modding!")
            ImGui.Separator()

            if ImGui.Button("Heal Player", 120, 30) then
                if Game and Game.HealAvatar then
                    Game.HealAvatar()
                end
            end

            ImGui.SameLine()
            if ImGui.Button("Level Up", 120, 30) then
                if Game and Game.LevelUpAvatar then
                    Game.LevelUpAvatar()
                end
            end

            ImGui.End()
        end
    end
})
```

---

## 6. Low-Level Memory & Reverse Engineering Toolkit

When game natives are insufficient, CrabeLoader V2 provides powerful, SEH-guarded memory manipulation primitives under `Crabe.Memory`:

| Function | Description |
| :--- | :--- |
| `Crabe.Memory.patternScan(pattern)` | Scans process memory for an IDA-style AOB string (e.g. `"55 8B EC ?? 8B 45"`). |
| `Crabe.Memory.patchBytes(addr, hex)` | Writes raw bytes to executable memory (e.g. `"90 90"`). Unprotects and flushes cache automatically. |
| `Crabe.Memory.readFloat(addr)` | Reads a 32-bit floating-point number from a memory address. |
| `Crabe.Memory.writeFloat(addr, val)` | Writes a 32-bit floating-point number to a memory address. |
| `Crabe.Memory.readU32(addr)` | Reads an unsigned 32-bit integer from a memory address. |
| `Crabe.Memory.writeU32(addr, val)` | Writes an unsigned 32-bit integer to a memory address. |
| `Crabe.Memory.installCodeCave(addr, hexBody, stolenLen)` | Installs a midpoint x86 code cave detour with dynamic instruction boundary detection via **HDE32**. |

### Example: Unlocking the Toy Box Editor Everywhere
```lua
-- 2-byte NOP patch to bypass story mission editor restrictions
local addr = Crabe.Memory.patternScan("0F B6 42 20 85 C0 74")
if addr then
    local success = Crabe.Memory.patchBytes(addr + 6, "90 90")
    if success then
        Crabe.write("[Mod] Toy Box Editor unlocked everywhere!")
    end
end
```

---

## 7. Event Bus & Keybindings

Listen for keystrokes or broadcast custom events across mods:

```lua
-- 1. Direct keybinding helper
Crabe.Input.bindKey(0x74, function() -- VK_F5
    isWindowVisible = not isWindowVisible
end)

-- 2. Generic Event Bus
Crabe.Events.on("keyDown", function(virtualKey)
    if virtualKey == 0x75 then -- VK_F6
        Crabe.write("F6 pressed!")
    end
end)

-- 3. Custom cross-mod events
Crabe.Events.emit("onPlayerCustomAction", { power = 9000 })
```

---

## 8. Hot-Reloading (`F4`)

While the game is running, you do **not** need to restart `DisneyInfinity3.exe` to test script changes:
1. Edit and save your `.lua` files in `mods/`.
2. Press **`F4`** in-game.
3. CrabeLoader executes `onShutdown` for all active mods, resets the sandbox registries, and re-loads all scripts fresh from disk.
4. Check `loader.log` or press **`Insert`** to review any compilation errors in the live developer console.
