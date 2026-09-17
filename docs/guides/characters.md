# CrabeLoader V2 — Character & Figurine Modding Guide

Welcome to the **CrabeLoader V2** Character & Figurine Modding Guide.

In *Disney Infinity 3.0 (PC)*, playable characters were historically tied to physical RFID figurine bases. On PC, the game emulates this hardware via a virtual reader table (`Presentation/VirtualReaderPC_Data.lua`). CrabeLoader V2 provides a clean Lua API (`Crabe.VirtualReader`) that allows you to inject unreleased characters, create custom skins, and expand the playable roster without patching encrypted game archives.

---

## 1. How Character Resolution Works

To successfully add or expose a character, you must understand the engine's 4-step resolution chain:

```mermaid
flowchart LR
    SKU["1. SKU ID (e.g. 1000129)"] --> AVATAR["2. VirtualReader AvatarData"]
    AVATAR --> NAME["3. Character Name (e.g. AL_Jasmine)"]
    NAME --> ACTOR["4. Engine ActorList (Type = 'Avatar')"]
    ACTOR --> DNA["5. 3D Model & Rigs (.dnax / .oct)"]

    style SKU fill:#e3f2fd,stroke:#1565c0,stroke-width:2px;
    style DNA fill:#e8f5e9,stroke:#2e7d32,stroke-width:2px;
```

### The Rule of Identity
* The true primary key is the **`Name` string**, not the numeric `sku_id`.
* The `Name` attribute (case-insensitive) **must exist** in the game's internal `ActorList` with `Type = "Avatar"`.
* If you assign a valid SKU but provide a `Name` that doesn't exist in the engine's `ActorList`, the character selection tile will appear in the UI, but spawning will either fail or spawn an invisible avatar falling through the floor.

---

## 2. The Two Modding Approaches

| Method | Purpose | Difficulty |
| :--- | :--- | :--- |
| **`addCharacter` (Cosmetic Variant)** | Creates a new identity or costume preset on top of an existing hero's model, sharing their base `sku_id`. | Beginner (100% stable) |
| **`exposeCharacter` (Standalone Hero)** | Surfaces an unreleased or hidden character whose 3D mesh, animations, and skill tree already exist inside game files, but lacks an official catalog row. | Intermediate (Requires unused SKU) |

---

## 3. Directory Placement

In CrabeLoader V2, all custom character declarations live strictly inside a mod folder under `mods/`:

```text
Disney Infinity 3.0 Gold Edition/
└── mods/
    └── my_hero_expansion/
        ├── mod.json
        ├── main.lua
        └── characters/
            ├── CRABE_MaceWindu.lua <- Mod-bundled character declaration
            └── CRABE_Thanos.lua    <- Mod-bundled character declaration
```

CrabeLoader scans `<GameRoot>/mods/*/characters/` on startup. You can also declare characters programmatically inside any mod's `main.lua` or `onInit` callback using `Crabe.VirtualReader.exposeCharacter()`.

---

## 4. Declaring Characters with `Crabe.VirtualReader`

### Example 1: Exposing Mace Windu (`characters/CRABE_MaceWindu.lua`)

Master Mace Windu has complete combat animations, purple lightsaber effects, and audio files in the game archives, but was never officially activated as a retail PC SKU. We can expose him using `exposeCharacter`:

```lua
-- characters/CRABE_MaceWindu.lua

Crabe.VirtualReader.exposeCharacter({
    Name = "TCW_MaceWindu",
    DisplayName = "Mace Windu",
    Franchise = "StarWars",
    Icon = "HUD_PlayerIcons_MaceWindu",
    Description = "Jedi High Council Master and creator of Vaapad lightsaber combat.",
    ProgressionTree = "tcw_macewindu",
    MetaData = "StarWars,Franchise_SW,Jedi,Lightsaber",
})
```

### Example 2: Exposing Thanos (`characters/CRABE_Thanos.lua`)

```lua
-- characters/CRABE_Thanos.lua

Crabe.VirtualReader.exposeCharacter({
    Name = "MV_Thanos",
    DisplayName = "Thanos",
    Franchise = "Marvel",
    Icon = "HUD_PlayerIcons_Thanos",
    Description = "The Mad Titan wielding cosmic strength and devastating heavy strikes.",
    ProgressionTree = "mv_thanos",
    MetaData = "Marvel,Franchise_MV,Villain,Boss",
})
```

---

## 5. SKU Allocation & Numbering Ranges

Disney Infinity figurines occupy specific SKU ranges based on release era:

| Era | SKU Range | Examples |
| :--- | :--- | :--- |
| **Disney Infinity 1.0** | `1000001` – `1000038` | Jack Sparrow, Sulley, Mr. Incredible |
| **Disney Infinity 2.0** | `1000100` – `1000140` | Iron Man, Thor, Spider-Man, Maleficent |
| **Disney Infinity 3.0** | `1000200` – `1000339` | Ahsoka, Anakin, Luke, Hulkbuster, Rey |
| **Community Mods (Safe Range)** | `2000000`+ | Custom characters created by players |

When using `Crabe.VirtualReader.exposeCharacter()`, if you do not specify a `sku_id`, CrabeLoader automatically assigns a free ID from the safe range `2000000`+, avoiding conflicts with vanilla figurines.

---

## 6. How Character Swapping Works in Mods

In mod menus such as **CrabeMenu**, characters can be spawned or swapped using two engine routes:

1. **`loadout` Route (Recommended):** Calls the game's internal `Game.SetPlayerCharacter(playerId, skuId)` or `Avatar_ChangeAvatar(skuId)`. This cleans up active buffs, reloads the weapon loadout, and rebuilds the skill tree properly.
2. **`legacy` Route:** Directly invokes the low-level actor spawn routines `Player_SpawnAvatar(name, x, y, z)`. Used when spawning AI companions or boss entities.

---

## 7. Troubleshooting

* **Missing 3D Mesh (Falling through the world):**  
  Verify that the `Name` attribute matches the engine's internal actor ID. Case sensitivity matters in internal hash tables. Use `search_index.py` or the SQLite database to check `ActorList.lua`.
* **Missing Icons in Selection Grid:**  
  If the `Icon` string is invalid, the engine falls back to a default silhouette icon.
* **Skill Tree Not Loading:**  
  Make sure `ProgressionTree` matches the stem of the character's skill tree file (e.g. `tcw_macewindu`).
