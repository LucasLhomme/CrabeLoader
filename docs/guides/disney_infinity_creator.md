# Disney Infinity Creator

**Disney Infinity Creator** is a desktop application (Python, Windows) bundled in
[`tools/creator`](https://github.com/LucasLhomme/CrabeLoader/tree/dev/tools/creator). It turns any 3D model into a
**new playable figure** for Disney Infinity 3.0 Gold Edition and installs it as a CrabeLoader mod. Where
[Creating a character](character_creation.md) explains the files of a character by hand, Creator produces all of
them from a model in a few clicks.

Full documentation: [English](https://github.com/LucasLhomme/CrabeLoader/blob/dev/tools/creator/README.en.md) ·
[Français](https://github.com/LucasLhomme/CrabeLoader/blob/dev/tools/creator/README.md).

---

## What it does

1. **Import** an `.obj`, `.glb`/`.gltf`, `.fbx`, `.dae` or a `.zip` holding the model and its textures. Blender (run in
   the background) can clean the model first: merged pieces, scale, orientation, lost textures.
2. **Pick a base figure** among the 99 playable characters. The new figure uses the base's skeleton, animations,
   moveset and abilities; nothing of the original figure is replaced.
3. **Fit the skeleton** automatically (corrected by models calibrated on the game's own figures), then refine each
   joint with sliders, in T-pose or directly in the game pose.
4. **Rig and preview**: skin weights (Blender bone heat or distance-based), the game's twist bones, test poses and a
   deformation score.
5. **Install**: Creator writes the mesh (`.oct`, `.vbuf`, `.ibuf`), materials (`.mtb`), actor definition (`.bent`,
   `.dnax`), database patches, the entry in the character grid, in-game icons and a relief normal map, then checks
   every file it wrote.

## Requirements

| What | Notes |
| :--- | :--- |
| Windows 10 or 11, Disney Infinity 3.0 Gold Edition (PC) | Your own copy |
| CrabeLoader, with CrabeMenu recommended | Loads the character; `F5` → Heroes to switch to it |
| Python 3.12+ | "Add python.exe to PATH" when installing |
| Blender 4.x or 5.x (recommended) | Model cleanup (required for `.fbx`) and better weights |

Start it with **`tools/creator/Launch_Creator.bat`**: the first launch creates a virtual environment and installs
the dependencies.

## Mods it produces

Each character is a folder `mods/<name>/` with its own `mod.json` (a `character` block, see
[Characters and figures](characters.md)). The game reads textures and UI pictures only from its archives, so the
character keeps only its source files and a single shared mod, **`mods/zz_creator_shared`**, carries the rebuilt
archives (`textures/17.zip`, `ui/icons/icons.zip`, `ui/virtualreadercharacters/virtualreadercharacters.zip`) for
every Creator character at once. The game's own entries are copied as they are; only the characters' files are
added.

`tools/creator/Manage_Mods.bat` lists, disables (`_` prefix, skipped by the loader) and deletes Creator characters,
and rebuilds the shared archives so a removed character leaves nothing behind.

## Game data

Creator reads the base figures from `tools/creator/src/assets/`, extracted from the user's own game install by the
scripts in `tools/creator/tools/` (`build_roster.py`, `build_shader_table.py`, `build_base_pack.py`). Game data and
third-party binaries (QuickBMS, `offsetting.exe`) are never committed.
