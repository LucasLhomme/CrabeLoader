# Technical Guide: Freecam & Unlocking All Characters in Play Sets

This guide details the complete technical procedure for:
1. **Implementing a Freecam (free camera)** controllable from the CrabeLoader Mod Menu.
2. **Unlocking ALL characters** (Disney, Marvel, Star Wars cross-era) in story-driven adventure packs (**Play Sets** such as *Rise Against the Empire*, *Twilight of the Republic*, etc.).

---

## PART 1: Freecam Implementation Procedure

### 1. Why Freecam is impossible in pure Lua?
In Disney Infinity 3.0, the game camera is managed directly in C++ by the Avalanche engine:
- Every frame (60 times/sec), the engine computes the camera's $(X, Y, Z)$ position and view matrix based on the player.
- If a Lua script tries to write a position, the C++ engine overwrites it immediately on the next frame.
- Keyboard inputs (WASD) are directly routed to the avatar.

**The solution:** A clean Freecam consists of a C++ hook in CrabeLoader, driven by a toggle in the Lua Mod Menu.

---

### 2. Freecam Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                 CrabeLoader (bink2w32.dll)                  │
│                                                             │
│   [ Lua Menu (15_menu.lua) ]                                │
│          │ Crabe.Freecam.toggle()                           │
│          ▼                                                  │
│   [ Freecam Module (C++) ]                                  │
│          │                                                  │
│          ├─► 1. InputHook: Blocks keys from reaching avatar │
│          │                 Reads WASD / Mouse for camera    │
│          │                                                  │
│          └─► 2. MinHook: Intercepts UpdateCamera()          │
│                          Prevents the game from overwriting │
│                          the camera; injects new coords     │
└─────────────────────────────────────────────────────────────┘
```

---

### 3. The 4 Implementation Steps

#### Step A: Find the camera function in memory (Cheat Engine)
1. Launch the game with Cheat Engine.
2. Search for the camera coordinates (type `float`) by observing values that change when rotating the camera with the mouse.
3. Set a write breakpoint (*"Find out what writes to this address"*).
4. Note the assembly function address (e.g. `Camera::UpdateViewMatrix` or the instruction `movups [esi+0x20], xmm0`).

#### Step B: Create the C++ Hook in CrabeLoader
Create a class following SOLID standards (`include/loader/freecam.hpp` and `src/freecam.cpp`):

```cpp
class Freecam
{
public:
    static Freecam& get();

    void setEnabled(bool enabled);
    bool isEnabled() const;
    void setSpeed(float speed);

    // Called every render frame (in RenderHook or InputHook)
    void update(float deltaTime);

private:
    bool _enabled = false;
    float _speed = 10.0f;
    Vector3 _cameraPos;
    Vector3 _cameraRot; // Pitch, Yaw
};
```

In the MinHook hook:
- If `_enabled == false`: call the game's original function.
- If `_enabled == true`: do **not** call the original function, but write `Freecam` coordinates directly into the game's camera structure.

#### Step C: Handle flight controls
In `src/input_hook.cpp`, when `Freecam::get().isEnabled()` is true:
* **Block movement key input from reaching the avatar** (so the character does not move).
* **Update the camera:**
  - `W` / `S`: Move forward / backward in the camera's look direction.
  - `A` / `D`: Strafe left / right.
  - `Space` / `Ctrl`: Move up / down vertically.
  - `Shift`: Speed multiplier (fast mode).
  - `Mouse movement`: Update Pitch and Yaw angles.

#### Step D: Expose controls to Lua & the Mod Menu
In `src/lua_runtime.cpp`, register functions for Lua:
```cpp
// Crabe.Freecam.set(bool)
// Crabe.Freecam.setSpeed(float)
```

Then in your Lua mod menu:
```lua
Crabe.Menu.registerInCategory("CAMERA", {
    label = "Freecam",
    toggle = true,
    state = false,
    onToggle = function(enabled)
        Crabe.Freecam.set(enabled)
        return enabled and "Freecam ACTIVE (WASD + Mouse)" or "Freecam DISABLED"
    end
})

Crabe.Menu.registerInCategory("CAMERA", {
    label = "Freecam Speed",
    cycle = { "Slow (5)", "Normal (15)", "Fast (40)" },
    index = 2,
    onCycle = function(label, index)
        local speeds = { 5.0, 15.0, 40.0 }
        Crabe.Freecam.setSpeed(speeds[index])
        return "Speed: " .. label
    end
})
```

---

## PART 2: Unlocking All Characters in Play Sets (Star Wars / Rise Against the Empire)

In Disney Infinity, story-driven adventure packs (Play Sets) enforce a strict restriction called **"Franchise Lock"**:
* In *Rise Against the Empire* or *Twilight of the Republic*, the game rejects all non-Star Wars characters (Mickey, Elsa, Iron Man, etc.).
* Even within Star Wars eras, the game historically required "Champion Coins" to allow characters like Luke in the prequel era.

---

### 1. How does the game enforce this lock?

The lock operates at two levels:
1. **UI Level (Lua Interface / Character Selection Grid):**
   The file `virtualreaderpc_gridcharacter.lua` filters displayed characters or grays them with an "invalid" state by calling `VirtualReaderPC_GetBrandFromCurrentPlaySet()` and `VirtualReaderPC_PlaysetCheckInGame()`.
2. **Engine Level (C++):**
   When loading the avatar (`VirtualReaderPC_SetCurrentCharacter`), the C++ engine checks whether the character ID belongs to the list of allowed characters for the current pack. If not allowed, the spawn is aborted.

---

### 2. Procedure to unlock everything via CrabeLoader

Unlike older mods that modified `.exe` files on disk, CrabeLoader can lift the restriction **dynamically in memory**:

#### Step 1: Unlock the selection interface (Lua)
The file `src/api/12_virtualreader.lua` in CrabeLoader already contains a `patchGridCharacter()` function that forces the `available` status:

```lua
-- In src/api/12_virtualreader.lua
function cls:GetGridButtonState(info)
    local state = "available"
    if info.IsDead then
        state = "dead"
    end
    return state
end
```

To make **all** characters appear in the grid even in a Play Set:
- In `src/api/12_virtualreader.lua`, patch the grid filtering method (`cls.FilterByPlayset` or `cls.PopulateGrid`) so it doesn't remove any character from the `VirtualReaderPC_Data.AvatarData` table.

#### Step 2: Bypass the native C++ check (`PlaysetCheck`)
The C++ engine calls a franchise validation function (similar to `FindPlaysetForCharacter` discovered during reverse engineering research).

In CrabeLoader:
1. **Hook the validation native:**
   Intercept the C++ function behind `VirtualReaderPC_PlaysetCheckInGame` via MinHook.
2. **Force a positive return:**
   Make this function always return `true` (character allowed), regardless of the current Play Set:
   ```cpp
   // Hook pseudo-code
   BOOL __cdecl Hooked_PlaysetCheckInGame(int characterId, int playsetId)
   {
       // Instead of checking for Star Wars membership:
       return TRUE; // Allows ALL characters (Mickey, Marvel, etc.)
   }
   ```
3. **Immediate result:**
   As soon as you select any figure in the menu, the engine loads the 3D model and animations into the Play Set without an error message.

---

### 3. Mod Menu Integration

To offer the player full control:
```lua
Crabe.Menu.registerInCategory("GAMEPLAY", {
    label = "Universal Play Set Crossover",
    toggle = true,
    state = true,
    onToggle = function(enabled)
        Crabe.SetPlaysetCrossoverUnlock(enabled)
        return enabled and "All characters allowed in Play Sets!" or "Normal restrictions"
    end
})
```

---

## Summary

1. **Freecam:** Requires intercepting the camera update function in C++ via MinHook to detach coordinates from the player and drive them via keyboard/mouse.
2. **All characters in Play Sets:** Requires lifting the filter in `12_virtualreader.lua` and forcing the C++ validation function to always return `true`.
