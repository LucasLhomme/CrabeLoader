# CrabeLoader V2 — Mod Development Guide

Welcome to the **CrabeLoader V2** Mod Development Guide.

CrabeLoader V2 is designed from the ground up on modern **SOLID architecture principles**:
* **CrabeLoader (C++23):** Acts as a high-performance, stable engine platform. It handles DirectX 11 hooks, hardware crash shields via Structured Exception Handling (SEH), Lua state virtualization, sandboxing, and generic memory primitives.
* **Mods (Lua 5.1):** Carry 100% of game-specific logic, menus, camera manipulation, custom tools, and gameplay systems.

---

## 1. Quick Start

### Directory Layout
Mods reside in the `mods/` directory located in the root of your Disney Infinity 3.0 installation:

```text
Disney Infinity 3.0 Gold Edition/
├── bink2w32.dll              <- CrabeLoader V2 proxy DLL (includes embedded API)
├── bink2w32_orig.dll         <- Original game Bink DLL
├── mods/                     <- Mod directory
│   ├── simple_mod.lua        <- Single-file standalone mod
│   └── my_complex_mod/       <- Folder-based modular mod
│       ├── mod.json          <- Mod manifest
│       ├── main.lua          <- Primary entry script
│       └── modules/          <- Internal scripts and assets
```

You can write either a single `.lua` file dropped directly into `mods/`, or a multi-file folder containing a `mod.json` manifest.

---

## 2. Mod Manifest (`mod.json`)

For multi-file projects, create a `mod.json` file in your mod folder:

```json
{
  "name": "My Custom Mod",
  "id": "my_custom_mod",
  "version": "1.0.0",
  "minLoaderVersion": "0.2.0",
  "entry": "main.lua",
  "author": "YourName",
  "description": "Custom gameplay features and ImGui overlay for Disney Infinity 3.0"
}
```

* `id`: Unique identifier for your mod (alphanumeric and underscores).
* `minLoaderVersion`: Enforces the minimum CrabeLoader version required to run your mod.
* `entry`: The entry Lua script to execute on startup (defaults to `main.lua`).

---

## 3. Mod Lifecycle (`Crabe.Mod.register`)

Every modern mod registers its lifecycle callbacks with `Crabe.Mod.register`:

```lua
Crabe.Mod.register({
    id = "my_custom_mod",
    name = "My Custom Mod",

    -- 1. Called once when the game's Lua VM is ready
    onInit = function()
        Crabe.write("My Custom Mod initialized successfully!")
    end,

    -- 2. Called every frame (~60 Hz / 16ms) on the game's Lua thread
    onUpdate = function(dt)
        -- Perform periodic game checks or timers
    end,

    -- 3. Called during the DirectX 11 Present loop to render ImGui UI
    onDraw = function()
        -- Render Dear ImGui windows and overlays safely
    end,

    -- 4. Called when the mod is unloaded or reloaded (F4)
    onShutdown = function()
        -- Clean up hooks, state, or temporary memory patches
    end
})
```

---

## 4. Deep-Frozen Sandbox Protection

In CrabeLoader V2, every mod runs within an **isolated sandbox environment**:
* **Variable Isolation:** Global variables declared in your mod (`myVar = 10`) remain strictly private to your mod's environment. Other mods cannot see or pollute them.
* **Read-Only Standard Tables:** Critical runtime tables (`Game`, `table`, `string`, `math`, `coroutine`, `os`, `debug`, `_G`) are protected by deep read-only proxy metatables.
* **Tamper Prevention:** Attempting to overwrite engine tables (e.g. `Game.SetPlayerHealth = nil`) raises a trapped error and logs a warning with the offending mod's ID, leaving the game and other mods intact.

### Sharing APIs Between Mods (`Crabe.Sandbox.export`)
To share functions or state across mods safely without polluting globals:

```lua
-- In Mod A (Producer):
Crabe.Sandbox.export("MySharedService", {
    sayHello = function(name)
        return "Hello, " .. tostring(name)
    end
})

-- In Mod B (Consumer):
local service = Crabe.Sandbox.import("MySharedService")
if service then
    Crabe.write(service.sayHello("Player 1"))
end
```

---

## 5. Designing In-Game User Interfaces (Dear ImGui)

CrabeLoader V2 includes **Dear ImGui** bindings directly in Lua, executed on the DirectX 11 render pipeline. All UI rendering must take place inside the `onDraw` lifecycle callback:

```lua
local isMenuOpen = true
local customSpeed = 1.0

Crabe.Mod.register({
    id = "ui_demo",
    name = "UI Demo",

    onDraw = function()
        if not isMenuOpen then return end

        if Crabe.ImGui.begin("My Mod Window", true) then
            Crabe.ImGui.text("Welcome to Disney Infinity 3.0 Modding!")
            Crabe.ImGui.separator()

            -- Toggle Button
            if Crabe.ImGui.button("Heal Player") then
                local player = Game.GetLocalPlayer and Game.GetLocalPlayer()
                if player and Game.SetPlayerHealth then
                    Game.SetPlayerHealth(player, 1000.0)
                end
            end

            -- Slider
            local changed, newSpeed = Crabe.ImGui.sliderFloat("Speed Multiplier", customSpeed, 1.0, 10.0)
            if changed then
                customSpeed = newSpeed
            end

            Crabe.ImGui.endWindow()
        end
    end
})
```

---

## 6. Memory Manipulation & Hooks

CrabeLoader V2 provides clean Lua primitives for interacting directly with the game's Win32 memory space.

### Memory Patching (`Crabe.Memory.patchBytes`)
Temporarily or permanently overwrite binary instructions (e.g., removing level restrictions):

```lua
-- Unlock Toy Box Editor everywhere (Nop out conditional jump at 0x00A8FB32)
local ok, err = Crabe.Memory.patchBytes(0x00A8FB32, { 0x90, 0x90 })
if ok then
    Crabe.write("Toy Box Editor unlocked everywhere!")
else
    Crabe.write("Patch failed: " .. tostring(err))
end
```

### Pattern Scanning (`Crabe.Memory.patternScan`)
Locate dynamic functions or offsets using AOB byte signatures:

```lua
local address = Crabe.Memory.patternScan("55 8B EC 83 EC 10 53 56 57 8B F9")
if address ~= 0 then
    Crabe.write(string.format("Found target function at: 0x%08X", address))
end
```

### Installing Code Caves (`Crabe.Hooks.installCodeCave`)
Divert execution at any instruction with **dynamic instruction boundary analysis** (using the internal HDE32 disassembler to ensure instructions are never split):

```lua
-- Install a code cave that automatically steals the required full instructions (>= 5 bytes)
local caveAddr = Crabe.Hooks.installCodeCave(0x00B1A2C0, {
    0x90, 0x90, 0x90  -- Your custom shellcode / hook logic
})
```

---

## 7. Event System & Keybindings

Listen for keyboard events without polling:

```lua
-- Subscribe to keydown events
Crabe.Events.on("keyDown", function(vk)
    -- VK_F5 = 0x74
    if vk == 0x74 then
        isMenuOpen = not isMenuOpen
        Crabe.write("Menu toggled: " .. tostring(isMenuOpen))
    end
end)
```

---

## 8. Development & Live Hot-Reloading

You do not need to restart the game to test your code changes!

1. Edit your Lua script in VS Code or any text editor.
2. Save the file.
3. Switch back to *Disney Infinity 3.0* and press **`F4`**.
4. CrabeLoader cleanly invokes `onShutdown` on all active mods, flushes the registry, and re-initializes all mods from disk in less than 50 milliseconds.
5. Press **`Insert`** at any time to open the developer debug console and test live Lua snippets.
