# Creating Custom Characters in Disney Infinity 3.0: From Zero to Hero

A comprehensive, step-by-step developer tutorial for creating custom standalone characters in **Disney Infinity 3.0: Gold Edition** using **CrabeLoader V2**.

---

## 1. Overview & Architecture

With **CrabeLoader V2**, custom character creation is **100% Zero-Touch**:
* **No modification of game installation files**: You never edit or repack vanilla `.zip` archives.
* **Automatic SKU Allocation**: No manual SKU picking or registry collision chores; SKUs are derived deterministically via 32-bit FNV-1a.
* **Declarative Manifest**: Character definitions can be declared cleanly inside `mod.json` or via `Crabe.VirtualReader.exposeCharacter`.
* **Dynamic Gateway Injection**: AES-128 figure registry slots, `ActorList` rows, and `DataMap` dependencies are injected dynamically in memory.
* **Loose VFS Texture Interception**: Textures are served directly as loose `.tbody` files without huge 50MB+ archive repacking.

---

## 2. Character Anatomy

A custom standalone character consists of:
```text
mods/<mod_id>/
├── mod.json                     <- Declarative manifest with character configuration
├── characters/
│   └── <actor_name>/
│       ├── <actor_name>.bent    <- Skeleton & bone hierarchy
│       ├── <actor_name>.oct     <- Octane scene graph & submesh bounds
│       ├── <actor_name>.mtb     <- Material & texture bindings
│       ├── <actor_name>_0.vbuf  <- Vertex stream buffer (positions, normals, UVs, weights)
│       ├── <actor_name>_0.ibuf  <- Triangle index buffer
│       └── <actor_name>.dnax    <- Animation & moveset binding
├── textures/
│   ├── <diffuse_hash>.tbody     <- Diffuse atlas texture (DDS DXT1/DXT5)
│   └── <normal_hash>.tbody      <- Normal map texture (DDS DXT5)
└── deploy.ps1                   <- One-click deployment script
```

---

## 3. Fast-Track: Building a Character in 5 Minutes

We provide an automated CLI tool under `tools/character_pipeline/build_character.py` that handles extraction, model retargeting, binary byte swapping, and manifest generation in a single command.

### Step 1: Run the Character Pipeline

```powershell
python tools/character_pipeline/build_character.py `
    --base EMP_Luke `
    --name SOR_Sora `
    --mod sora `
    --icon path/to/icon.png `
    --deploy
```

### What Happens Automatically:
1. **Extraction**: Unpacks base actor assets (`EMP_Luke`) from `assets/characters/emp_luke/emp_luke.zip` into a clean workspace.
2. **Actor Retargeting**: Clones `.bent`, `.oct`, `.mtb`, `.vbuf`, `.ibuf`, and `.dnax`, renaming all internal strings to `SOR_Sora` with byte-exact consistency.
3. **HUD Icon Generation**: Converts your PNG image into an Octane `.tbody` DDS texture and creates the 80-byte companion `.mtb` descriptor.
4. **Manifest Generation**: Generates `mod.json` with the required `character` block.
5. **Deployment**: Installs the mod directly into your game's `mods/sora/` folder.

---

## 4. Declarative `mod.json` Reference

Here is the standard `mod.json` format for a custom character:

```json
{
  "id": "sora",
  "name": "Sora (Kingdom Hearts)",
  "version": "1.0.0",
  "description": "Custom standalone character Sora with Kingdom Hearts animations and Keyblade weapon.",
  "character": {
    "name": "SOR_Sora",
    "baseCharacter": "EMP_Luke",
    "displayName": "Sora",
    "icon": "SOR_Sora"
  }
}
```

### Field Descriptions:
* **`name`** *(required)*: The unique actor name of the character (e.g. `SOR_Sora`). Must match the actor folder `characters/<name_lowercase>/`.
* **`baseCharacter`** *(recommended)*: An existing vanilla character (e.g. `EMP_Luke`, `TCW_Anakin`, `AVG_IronMan`) whose combat logic, voice routing, and DNA configuration are inherited.
* **`displayName`** *(optional)*: Human-readable name displayed in menus and HUD popups.
* **`icon`** *(optional)*: The HUD icon identifier.

---

## 5. In-Game Testing & Verification

1. Launch `DisneyInfinity3.exe`.
2. Open the CrabeMenu overlay by pressing **`F5`** (or gamepad `Back` / `View`).
3. Navigate to **`Heroes`** -> **`Swap Character Model`**.
4. Select **`★ Custom / Modded Characters`**.
5. Select your custom character (e.g. `SOR_Sora`).
6. Your character spawns immediately into the world!
