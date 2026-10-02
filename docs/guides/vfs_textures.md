# CrabeLoader V2 — Virtual File System (VFS) & Loose Textures

Technical specification and developer guide for **Virtual File System (VFS)** loose texture interception in **Disney Infinity 3.0: Gold Edition**.

---

## 1. Background & Motivation

In the retail PC release of *Disney Infinity 3.0*, textures are distributed as `.tbody` files (DDS containers) packed inside 256 two-character encrypted or unencrypted `.zip` archives under `assets/textures/<xx>.zip` (e.g. `a5.zip`, `b4.zip`).

### The Legacy Problem:
* Distributing a single custom texture required modders to repack and distribute an entire 50MB+ `.zip` archive matching the first two hex characters of the texture hash.
* If two independent mods introduced textures starting with `a5`, installing both caused a fatal file overwrite collision: one mod would overwrite the other's `.zip`.

### The CrabeLoader V2 Solution:
CrabeLoader hooks the engine's internal file resolution layer (`VfsOverrideManager`), intercepting requests for texture assets before the engine consults archive files on disk. If any active mod supplies the matching texture as a loose `.tbody` file, CrabeLoader redirects the file handle directly to the mod's loose file in memory.

---

## 2. Directory Structure & Canonical Resolution

To provide custom textures, place `.tbody` files inside your mod's `textures/` folder:

```text
mods/<mod_id>/
├── mod.json
├── characters/
│   └── sor_sora/
└── textures/
    ├── a5a0000050a40001.tbody       <- Loose diffuse atlas
    └── a5a0000050a40002.tbody       <- Loose normal map
```

### Canonical Matching Rules:
The VFS manager normalizes all incoming file queries and supports multiple canonical aliases:
1. **Raw Hash**: `<hash>.tbody` (e.g. `a5a0000050a40001.tbody`)
2. **Prefixed**: `textures/<hash>.tbody`
3. **Subfolder**: `textures/<xx>/<hash>.tbody`

All lookups are **case-insensitive** and O(1) in memory via `std::unordered_map`.

---

## 3. Runtime Hot-Reloading (`F4`) & Texture Cache Invalidation

When you edit a texture in external authoring software (e.g. Photoshop, GIMP, Blender) and save the `.tbody` file:

1. Press **`F4`** in-game.
2. CrabeLoader re-scans the `mods/` directory, updating the internal VFS lookup map.
3. CrabeLoader automatically purges the engine's internal `g_MaterialCache` and flushes Direct3D 11 texture resource views.
4. The game reloads the texture instantly on the live 3D character model without restarting `DisneyInfinity3.exe`.

---

## 4. Technical Specifications

| Property | Value |
| :--- | :--- |
| **Container Format** | `.tbody` (Standard DDS file with optional 16-byte DI3 header) |
| **Supported Pixel Formats** | DXT1 (BC1), DXT5 (BC3), RGBA8 |
| **Mipmap Requirements** | Complete mipmap pyramid down to 4x4 recommended |
| **Hash Length** | 16 hexadecimal characters (64-bit integer, e.g. `a5a0000050a40001`) |
| **Material Linkage** | Referenced by 8-byte hash inside `.mtb` material bundles |

---

## 5. Frequently Asked Questions (FAQ)

### Can I replace specific in-game objects, world assets, or props (e.g. a Star Wars wall)?
**Yes, absolutely.** The Virtual File System (VFS) intercepts file access at the Win32 API level (`CreateFile`, `GetFileAttributes`, `FindFirstFile`), which means **any** file requested by the game under `assets/` (textures, sounds, models, UI, etc.) can be overridden or added without modifying the original game files.

### Why is there no Lua API like `replaceTexture("wall", "new_texture")`?
In *Disney Infinity 3.0* (built on Avalanche Software's **Octane** engine):
* Textures are not identified at runtime by human-readable names like `"star_wars_wall.png"`.
* Each texture is compiled into a `.tbody` file (a Direct3D DDS container with a 16-byte header) and referenced by a **64-bit hexadecimal hash** (e.g., `e4b1000089a10002.tbody`).
* 3D models and material definitions (`.mtb` bundles) link directly to these 64-bit texture hashes.
* Because this linkage is compiled into assets and handled directly by the engine's renderer, there is no need for a dedicated runtime Lua API. Overriding is **100% zero-code and drop-in**:

### Step-by-Step Workflow:
1. **Find the Asset / Texture Hash:** Use community modding tools (such as DI Model Viewer, QuickBMS with Octane/DI scripts, or RenderDoc) to inspect the object's model or material and retrieve its 16-character texture hash.
2. **Author the Replacement:** Export your texture in DDS format (DXT1/BC1 or DXT5/BC3 with mipmaps) and convert it to `.tbody`.
3. **Drop it into your Mod:** Place the file in your mod's directory:
   ```text
   mods/<mod_name>/textures/<hash>.tbody
   ```
4. **Instant Reload:** Start the game or press **`F4`** in-game to flush the material cache and see your texture update live without restarting `DisneyInfinity3.exe`.

