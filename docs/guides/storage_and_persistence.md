# Persistent Mod Storage & Settings

A developer guide for saving and loading persistent data, user settings, quest states, and custom progression across game sessions using **CrabeLoader's Storage API** (`Crabe.Storage`).

---

## 1. Overview & Motivation

In the retail PC version of *Disney Infinity 3.0*:
* The embedded Lua 5.1 runtime has **no JSON serializer**, no `string.pack`, and no native interface for mods to save persistent files.
* Vanilla game saves only store player stats, Toy Box saves, and official unlock bits.

**CrabeLoader solves this** by providing a lightweight, RFC-8259 compliant JSON parser and disk serialization engine directly inside `src/api/04_storage.lua`.

### Key Capabilities:
* **Per-Mod Scoped Storage**: Mods can isolate their save files cleanly under `mods/<mod_name>/` using `Crabe.Storage.forMod`.
* **Zero Dependency**: Runs out-of-the-box without requiring external C libraries or DLL dependencies.
* **Session Persistence**: Data survives game exits, crashes, and hot-reloads (**`F4`**).

---

## 2. Quick Start: Scoped Mod Storage

The recommended way to persist data is using `Crabe.Storage.forMod(modName)`:

```lua
Crabe = Crabe or {}

-- 1. Obtain a scoped storage handle for your mod
local Storage = Crabe.Storage.forMod("my_combat_mod")

-- Default configuration table
local config = {
    godmode = false,
    damage_multiplier = 2.5,
    favorite_weapon = "Tools.GreenLightsaber",
    kill_count = 0
}

-- 2. Load existing settings or fall back to defaults
local loaded, err = Storage.load("settings.json")
if loaded then
    for k, v in pairs(loaded) do
        config[k] = v
    end
    print("[MyCombatMod] Configuration loaded successfully.")
else
    print("[MyCombatMod] No existing config found, using defaults.")
end

-- 3. Modify values during gameplay
config.kill_count = config.kill_count + 1

-- 4. Save back to disk
local success, saveErr = Storage.save("settings.json", config)
if not success then
    print("! Failed to save settings: " .. tostring(saveErr))
end
```
This writes directly to:
```text
mods/my_combat_mod/settings.json
```

---

## 3. Storage API Reference

### 3.1. `Crabe.Storage.forMod(modName)`
Creates a scoped storage helper bound to `mods/<modName>/`.

```lua
local myStorage = Crabe.Storage.forMod("super_hero_mod")
```
Returns an object with two methods:
* `myStorage.load(filename)`: Reads and decodes JSON from `mods/<modName>/<filename>`.
* `myStorage.save(filename, dataTable)`: Encodes and writes `dataTable` to `mods/<modName>/<filename>`.

### 3.2. Global File Persistence

If you need to read or write files at custom relative paths:

#### `Crabe.Storage.save(filename, dataTable)`
* **Parameters**:
  * `filename` (*string*): Target file path relative to the game directory.
  * `dataTable` (*table*): Lua table containing strings, numbers, booleans, arrays, or nested tables.
* **Returns**: `boolean success`, `string? errorMessage`

```lua
local ok, err = Crabe.Storage.save("Crabe/custom_stats.json", { highscore = 42000 })
```

#### `Crabe.Storage.load(filename)`
* **Parameters**:
  * `filename` (*string*): File path to read.
* **Returns**: `table? decodedData`, `string? errorMessage`

```lua
local data, err = Crabe.Storage.load("Crabe/custom_stats.json")
if data then
    print("High score:", data.highscore)
end
```

---

## 4. In-Memory JSON Encoding & Decoding

You can also use CrabeLoader's JSON parser directly in memory without writing to disk:

### `Crabe.Storage.encode(dataTable)`
Converts a Lua table into a valid JSON string.
```lua
local jsonText = Crabe.Storage.encode({
    hero = "Sora",
    hp = 100,
    inventory = { "Keyblade", "Potion" }
})
-- Result: '{"hero":"Sora","hp":100,"inventory":["Keyblade","Potion"]}'
```

### `Crabe.Storage.decode(jsonString)`
Parses a JSON string into a structured Lua table.
```lua
local luaTable, err = Crabe.Storage.decode('{"sparks": 500, "active": true}')
if luaTable then
    print(luaTable.sparks) -- 500
end
```

---

## 5. Best Practices & Performance

1. **Do NOT save every frame in `onUpdate`**:
   Writing to disk every frame (~60 times per second) causes micro-stuttering and disk wear.
2. **Save on Mod Shutdown**:
   Use `onShutdown` in `Crabe.Mod.register` to flush pending changes when the world changes, when the user reloads (**`F4`**), or when the game exits:
   ```lua
   Crabe.Mod.register({
       name = "MyMod",
       onShutdown = function()
           Storage.save("settings.json", myModData)
       end
   })
   ```
3. **Debounce Frequent Updates**:
   If tracking values that update frequently (e.g. coordinates or kill counts), maintain them in memory and save only every 30 to 60 seconds using `Crabe.wait` or a frame accumulator.
