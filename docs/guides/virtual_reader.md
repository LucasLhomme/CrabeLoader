# Virtual Reader & Dynamic Figure Management

A technical reference and developer guide for **CrabeLoader's Virtual Reader subsystem** (`Crabe.VirtualReader`). Learn how the engine resolves RFID tags into 3D avatars, how to expose hidden or custom characters, and how to swap characters dynamically at runtime without a physical portal.

---

## 1. Overview & Architecture

In *Disney Infinity 3.0*, the physical USB portal communicates with three distinct RFID antennas:
* **Antenna 0 (Slot 0)**: Player 1 Avatar figure (or hexagon power disc stack).
* **Antenna 1 (Slot 1)**: Player 2 Avatar figure (or hexagon power disc stack).
* **Antenna 2 (Slot 2)**: Play Set piece, Toy Box Game disc, or Magic Band.

When running on PC, the game features an internal **Virtual Reader** (`VirtualReaderPC_Data`), allowing players without a physical portal to select figures from an on-screen grid.

### The Character Resolution Pipeline:
When a figure is placed or selected from the grid:
```text
[SKU ID] ──▶ [VirtualReaderPC_Data.AvatarData] ──▶ [Name (e.g. "SOR_Sora")]
                                                         │
                                                         ▼
[assets/characters/<actor>.dnax] ◀── [DNAFile] ◀── [ActorList (Name:lower())]
```
1. The **SKU ID** is looked up in the `AvatarData` catalog to find the character's `Name`.
2. The lowercase `Name:lower()` must exist in the engine's dynamic `ActorList` with `Type = "Avatar"`.
3. The `ActorList` entry points to the `DNAFile` (compiled `.dnax`), which binds the 3D model, animation overrides, and skill tree.

---

## 2. Exposing & Adding Characters

`Crabe.VirtualReader` provides two complementary operations:

### 2.1. `exposeCharacter(name, baseCharacter, displayName)`
Used to surface characters that exist in the game archives or mod folders but lack a tile in the official character selection grid:

```lua
Crabe = Crabe or {}

Crabe.VirtualReader.exposeCharacter({
    Name = "SOR_Sora",
    baseCharacter = "EMP_Luke",
    displayName = "Sora",
    Icon = "SOR_Sora"
})
```

### 2.2. Deterministic SKU Allocation (`Crabe.VirtualReader.skuForName`)
To avoid collisions between independent mods, CrabeLoader allocates SKUs deterministically:
* A polynomial rolling string hash (`h = (h * 31 + byte)`) maps the character's actor name into the safe modding range **`1000340`–`1000999`**.
* The C++ gateway and Lua runtime mirror this calculation identically, ensuring the in-memory figure registry slot always matches the Lua catalog entry.

```lua
local sku = Crabe.VirtualReader.skuForName("SOR_Sora")
print("Allocated SKU for Sora:", sku) -- e.g. "1000412"
```

---

## 3. Dynamic In-Game Swapping (`Game.SetCharacter`)

While `VirtualReader` controls the menu catalog, **`Game.SetCharacter`** (`src/api/11_avatar.lua`) swaps the active avatar **instantly in the live world**:

```lua
-- Swap Player 1 to Darth Vader using SKU or Name
Game.SetCharacter("SW_DarthVader")

-- Or swap to a custom modded hero
Game.SetCharacter("SOR_Sora")

-- Swap for Player 2
Game.SetCharacter("SW_LukeSkywalker", 1)
```

### Preserving Character State on Swap:
When `Game.SetCharacter` executes:
1. The engine checks current player coordinates and facing angles.
2. The current avatar entity is despawned.
3. The new character entity is loaded and spawned at the exact same coordinates.
4. Level, progression, and equipped skill tree are restored.

---

## 4. Querying Characters

### Listing All Characters in the Roster:
```lua
local allCharacters = Crabe.VirtualReader.listCharacters()
print(string.format("Total roster: %d figures", #allCharacters))
```

### Listing Modded Characters:
```lua
local modded = Crabe.VirtualReader.getModdedCharacters()
for _, char in ipairs(modded) do
    print(string.format("Custom Hero: %s (SKU: %s)", char.Name, tostring(char.sku_id)))
end
```
