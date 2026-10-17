# Disney Infinity Creator

[Français](README.md) · **English**

Turn any 3D model into a **playable character** for **Disney Infinity 3.0 Gold Edition (PC)**.

You import a model (Sketchfab, The Models Resource, your own Blender work…), pick a figure of the game as its base,
and Creator fits a skeleton, computes the skinning, lets you preview the result in motion, then installs it as a mod.
The character shows up in the game as a **brand-new figure**: it uses the base figure's animations, attacks and
abilities, and no original character is replaced.

> Fan-made tool, not affiliated with Disney, Avalanche Software or any rights holder. You need your own copy of the game.

---

## Contents

- [Features](#features)
- [Requirements](#requirements)
- [Installation and launch](#installation-and-launch)
- [Creating a character step by step](#creating-a-character-step-by-step)
- [Managing your characters](#managing-your-characters)
- [Understanding the deformation score](#understanding-the-deformation-score)
- [Command-line tools](#command-line-tools)
- [Playing your character](#playing-your-character)
- [Tips for a good result](#tips-for-a-good-result)
- [How it works](#how-it-works)
- [Known limitations](#known-limitations)
- [Troubleshooting](#troubleshooting)
- [For developers](#for-developers)
- [Credits](#credits)

---

## Features

- **99 base figures**: every playable character of Disney Infinity 3.0 (Marvel, Star Wars, Disney, Pixar, including
  those from 1.0 and 2.0), with search and a textured 3D preview.
- **Nothing is overwritten**: each creation is a new figure with its own tile in the character grid. Luke, Thor or
  Elsa stay untouched, and you can install as many characters as you like.
- **Built-in cleanup**: on import, Blender (in the background) merges the pieces, fixes scale and orientation,
  recovers lost textures (missing `.mtl`, images next to the model) and freezes any skeleton. The result is a
  temporary file deleted on exit: nothing is left in your folders. `.fbx` files always go through this cleanup.
- **Manual placement**: move and rotate the model (sliders, 1 cm buttons, fine rotation, "Recentre") when the
  automatic fit is wrong.
- **3-view layout** (front, side, top) to check the skeleton alignment at a glance.
- **Automatic rigging**: shoulders, elbows, wrists, neck, hips, knees and ankles are detected, with one slider per
  joint to refine them.
- **Quality skin weights** through Blender when installed. If it fails (open meshes, many pieces) or is missing,
  Creator falls back to a distance-to-bone method with per-zone rules.
- **Motion preview**: in-game pose, arms up, walk, with an amplitude slider ("Pose amount").
- **Deformation score** out of 100, with over-stretched edges highlighted in red, and detection of a "collapsed"
  model when the weights are broken.
- **Base suggestions**: ranks the base figures by how well they match your model.
- **Piece locking**: Ctrl+click a piece (hair, cape, weapon…), "Grow" to include the small neighbouring pieces,
  "Lock" to make it follow a single bone.
- **Texture atlas preview** with checks (image too dark, not square…), and import warnings (material without
  texture, repeated texture).
- **In-game icons**: HUD portrait and character-grid figure, captured automatically from your model.
- **Relief**: a normal map derived from the texture gives the "sculpted toy" look of the game's figures.
- **Lightweight shared mod**: textures and icons of all your characters live in a single mod (`zz_creator_shared`)
  instead of a ~13 MB archive per character; `Manage_Mods.bat` lists, disables and cleanly deletes your characters.
- **Error log** (`build/creator.log`) to understand a crash.
- **Verified installation**: every install is checked (buffers, weights, file links) and the previous version of the
  mod is backed up to `backups/`.

## Requirements

| What | Notes |
| :--- | :--- |
| Windows 10 or 11 | |
| Disney Infinity 3.0 Gold Edition (PC) | Your own installed copy |
| [CrabeLoader](https://github.com/LucasLhomme/CrabeLoader) and [CrabeMenu](https://github.com/LucasLhomme/CrabeMenu) | The mod loader that loads your character. CrabeMenu (`F5` menu in game) is recommended to switch to your character |
| Python 3.12 or newer | Tick "Add python.exe to PATH" during installation |
| Blender 4.x or 5.x | Recommended: model cleanup (required for `.fbx`) and better weights. Creator runs it in the background, you never have to open it |

## Installation and launch

1. Download or clone this repository.
2. Double-click **`Launch_Creator.bat`**. On first launch it creates the `.venv` virtual environment and installs the
   dependencies (`customtkinter`, `moderngl`, `numpy`, `Pillow`, `trimesh`, `scipy`, `cryptography`), then opens
   Creator. After that, it starts right away.
3. If the app stops, the window stays open and points to `build\creator.log`.

> `run.ps1` (PowerShell) does the same and reinstalls the dependencies on every launch: useful after updating
> Creator.

The Blender and game paths are set in the **Rig & Export** tab; they are remembered.

## Creating a character step by step

The left panel has five tabs; go through them in order.

### 0. Base: pick the figure

Choose the figure whose skeleton, animations and moveset your character will use.

| Mark | Meaning |
| :---: | :--- |
| ● | Standard skeleton: every required joint exists |
| ◐ | Adapted skeleton: a few bones stand in for others (e.g. the ball of the foot for the ankle). Works, with small approximations |
| ✕ | Not humanoid (Olaf, Randall…): cannot take a humanoid mesh |

**Pick a base with a similar build.** A slim hero fits Spider-Man better than Hulk, a long dress fits Elsa better than
Captain America. The **"Suggest the best bases for my model"** button (Model tab) ranks the figures for you: it
compares your model's silhouette with each figure's (in T-pose), keeps different body types (not five clones), then
builds the real rig of the best ones and measures the stretch in the test poses and the deformation when your T-pose
is fitted onto the game's pose. Each row shows these three measures.

Since the moveset comes with the figure, **the whole list (~90 figures) stays visible**: the top 12 are tested right
away, the others are ranked by silhouette ("not tested"). **"Test the next 12 figures"** tests the next batch, and the
filter field searches by name or franchise (Avengers, Star Wars…).

**Fingers (experimental, off by default).** The base figures have finger bones (Thumb1-3, Index1-2, Middle1-2). With
**"Finger weights"** ticked, your model's hand follows these bones: each vertex beyond the finger joints is shared
between the wrist and the nearest finger chain. As long as the model's fingers do not line up with the base's (4
fingers vs 3 chains, bigger hands), the game's grips (saber, fist) crumple the hand, so a rigid hand is the default.
A hand of fewer than 60 vertices always stays rigid. The **Fist** pose in the pose selector shows the result.

**`.dae` format (COLLADA).** Creator reads it itself, without Blender: geometry, UVs, textures (looked up next to the
file), unit and up axis. A skinned model is read in its rest pose. With the cleanup box ticked, Creator converts it
to `.glb` (identical colours) before handing it to Blender, which has had no Collada importer since version 5.

**Game look (texture filter).** Model tab, "Game look" slider (0 = original texture). Fan models are darker, duller
and full of painted shadows; the game's characters are bright, saturated and smooth. The filter softens noise and
baked shadows (edge-preserving guided filter), lifts the shadows and adds saturation. It only works inside the UV
islands: no colour bleeds in from the background. The result shows live in the 3D view and in "Preview the texture
atlas" (fast 512 px version while the slider moves, full resolution on release), and it is what gets exported to the
game. The second slider, "Match the base figure's brightness", brings the average brightness closer to the chosen
base's texture (0 = off).

**Relief.** "Relief" box (Rig & Export tab, on by default): Creator derives a normal map from the texture (seams,
folds and colour edges slightly carved) in the format of the game's maps, like the official figures. Untick it to go
back to a flat map.

**Character icons.** "Character icons" box (Model tab): on install, Creator captures the model (texture with the
filter, game pose once rigged) and writes the pictures in the format of the game's files: the 128 px HUD portrait
(`ui/icons/hud_playericons_<actor>.mtb` + texture, referenced in `mod.json` by
`"icon": "HUD_PlayerIcons_<Actor>"`) and the Virtual Reader figure (`ui/virtualreadercharacters/<actor>.mtb`, `_sm`,
and the `_s` silhouette). `<actor>_icon.png` stays at the root of the mod. The "Preview" button shows the three
pictures. The game only reads these pictures from its archives: Creator adds them to the shared mod's archives (see
[Managing your characters](#managing-your-characters)).

**Animation audit (separate tool, outside Creator).** Drop a model onto `Audit_Animations.bat` (default base
TCW_Anakin; another one as 2nd argument). The tool builds the rig, plays every animation on it and writes to
`build\audit\<model>\`: `report.txt` (static score + one line per animation: share of edges deformed more than 2×,
worst frame, affected zones), `report.json` and pictures of the worst frame of the problematic animations (stretched
edges in red). The clips are Creator's procedural motions, not the game's original animations. Fingers follow the
hand in these clips.

**Proportions (head, torso, arms, forearms, legs).** Skeleton tab, five sliders: "Head" (×0.8 to ×1.5, the head grows
from the jaw, the chin stays in place, the neck thickens by half), "Torso", "Arms", "Forearms" and "Legs" (×0.8 to
×1.8, inflated around their axis without changing length, blended with the weights). Live preview, applied to the rig.
- **Disney Infinity style**: moves the model towards the average proportions of 30 humanoid characters of the game
  (measured in game: head = 21 % of the height, forearms thicker than upper arms, etc.). Never smaller than the
  model; caps: head ×1.4, torso ×1.4, limbs ×1.6.
- **Match the base**: copies the chosen base's thickness (head untouched).
- **Reset**: back to the model's proportions.
A loose garment (baggy trousers) counts as thick: the legs then stay at ×1.
- **V-shape** (0 to 1): in the game pose, the arms hang from the base's (wide) shoulders; a narrow-chested model then
  leaves a gap under the arms. V-shape widens the chest sideways, slice by slice, up to the inner side of the lowered
  arm (minus 2 % of the height): full effect from chest to armpits, nothing at the waist or above the armpits (neck,
  collar), and a slice that is already wide enough (flared jacket) does not move. Both buttons set it to 1.

**Game pose while adjusting the skeleton.** Skeleton tab, "Preview in game pose" box: the model is shown arms down,
as the game will display it, and follows every skeleton slider (preview computed in a few milliseconds with
approximate weights). If the rig is already built, its real game pose is shown.

**Shoulder roundness.** Rig & Export tab, "Shoulder roundness" slider, active once the rig is built. The arms turn by
about 80° to go from the T-pose to the game pose. At 0 (default), the chest/arm junction is folded towards the joint:
suits jackets and wide sleeves (Sora). At 1, it keeps its volume (dual quaternions + smoothing): round shoulders, suits
bare arms (Xehanort). The blend shows live and is what gets exported.

**Skeleton badges.** Green ✓ = close to the base's proportions (including after a manual tweak); blue ✎ = set by hand
but still far from the base; ⚠ = estimated or too different.

**Accessories (cape, tail, wings, weapon).** A cape hanging behind the pelvis skews the measurements (torso depth,
knees, crotch) and gets torn by the legs. In the Model tab, "Accessories" box:

- **Find** looks for (1) everything further behind the body than the set depth (slider, 0.20 × the height by
  default), even if the cape is welded to the body, and (2) separate pieces that stick out. The result is highlighted
  in magenta. You can also select a piece with Ctrl+click.
- **Ignore**: the part stays in the model but is no longer seen by the auto-fit or the base ranking, and it follows
  the torso (hips, chest) instead of the legs and arms.
- **Remove**: the part is removed from the model.
- **✕** next to the line undoes either. "Lock" (rigid piece on one bone) remains available for separate pieces.

**Skeleton tab.** Each slider has a short note (what to line the green dot up with) and an **input box** on the right
to type the exact value (Enter applies). A small icon to the right of each dot: ✓ found on the mesh, yellow ⚠
estimated by the auto-fit, orange ⚠ far from the base's proportions, ✎ set by you; hover it to read why. Sliders in
yellow "⚠ check" are the ones the auto-fit only estimated. The **Elbow position** slider places the elbow. Below the
summary, ⚠ warnings show where your proportions differ from the base by 20 % or more (arms, shoulders, legs) and which
slider to check.

There are six test poses: arms up, walk, **arms overhead, arms forward, elbows bent and torso twist** (40 % of the
stretch penalty). They are also available in the pose selector to see where the shoulders break.

The **"in T-pose"** box (under "Show the base figure alongside") puts the comparison figure and its cyan skeleton back
in T-pose, to line them up with your model.

### 1. Model: import

Click **Import a 3D model** and choose an `.obj`, `.glb`, `.gltf`, `.fbx` (or `.dae`, `.ply`, `.stl`, `.off`), or a
`.zip` holding it with its textures.

- **"Clean with Blender while importing"** is ticked by default. It runs the cleanup described above. Without a valid
  Blender path, `.glb`/`.obj`/`.zip` files are imported as they are; `.fbx` requires Blender.
- The model must be in **T-pose** (arms out); an A-pose works too.
- Creator detects the orientation. Otherwise: **↻ 90°**, **↻ 180°**, **Stand up**, **⇅ Flip**.
- **Model height** is preset to the base's size. Tick **Show … alongside** to compare side by side.
- **Position**: left/right, back/front, down/up sliders, fine rotation; ◄ 1 cm ►, ▼ back, ▲ forward buttons;
  **Recentre** goes back to automatic centring.
- **Pieces**: Ctrl+click a piece in the view to select it, **Grow** to extend to the small neighbouring pieces, pick the
  bone and **Lock**: the piece will follow that bone rigidly (e.g. hair strands on the head).
- **Preview the texture atlas** shows the texture the game will receive, with warnings.
- Import warnings (material without texture, repeated texture…) are shown in yellow under the model's name.

### 2. Skeleton: check the joints

Yellow dots = joints, green lines = bones. Creator places them on its own; the text above the sliders says what was
**detected** and what is only **estimated** (check those first). Adjust with the sliders (arm height, shoulder width,
wrists, neck, head, hips, leg spread, knees, ankles). **Auto-Fit** starts again from the automatic placement. Turn on
**"3 views"** (bar under the viewport) to see front, side and top.

### 3. Abilities & Moveset

The character's abilities and moveset settings (inherited from the base).

### 4. Rig & Export: build and install

1. **Character name**: name shown in the menus (e.g. `Mario`); it also names the mod folder.
2. **Internal actor name**: the engine's internal name (e.g. `MAR_Mari`), 3 to 32 characters, letters/digits/`_`,
   starting with a letter; prefilled.
3. **Game folder**: your Disney Infinity 3.0 folder.
4. **Blender**: path to `blender.exe` (optional but recommended).
5. **1. Build the rig**, then check **In-game pose**, **Arms up**, **Walk** and the **deformation score**. If something
   looks wrong, go back to tab 1 or 2, adjust, rebuild.
6. **2. Install into the game**: the log lists the checks, all of them must show `OK`.

The mod is written to `<game>/mods/<character name>/`. Reinstalling under the same name replaces the mod and moves
the previous version to `backups/`. Creator then updates the shared mod `zz_creator_shared` (textures and icons, a few
seconds). **Close the game before installing**: it keeps its archives open.

### Option: robust weight transfer

"Robust weight transfer" box (Model tab). Creator builds the rig twice (Blender weights, then weights copied from the
base figure's body) and keeps the one that deforms less. Mostly useful for skirts, capes and long coats. No extra
module to install (NumPy and SciPy are enough).

- Method: "Robust Skin Weights Transfer via Weight Inpainting" (Abdrashitov, Raichstat, Monsen, Hill, SIGGRAPH Asia
  2023), reimplemented without compiled dependencies. Inspired by the free Blender add-on
  [Robust Weight Transfer](https://github.com/sentfromspacevr/robust-weight-transfer) by SentFromSpaceVR and the
  reference code [RobustSkinWeightsTransferCode](https://github.com/rin-23/RobustSkinWeightsTransferCode) (MIT).

## Managing your characters

The game reads textures and icons only from its own archives (`assets/textures/17.zip`,
`assets/ui/icons/icons.zip`…), which a mod has to ship whole. To avoid repeating these archives in every character:

- each character (`mods/<name>/`) only keeps its source files (`textures/*.tbody`, `ui/…`);
- the **`mods/zz_creator_shared`** mod holds the archives rebuilt once for all enabled characters (the game's files are
  copied as they are, only your files are added). Do not edit it by hand.

**`Manage_Mods.bat`** (separate tool, game closed; its menu is in French):

| Choice | Effect |
| :--- | :--- |
| 1. Supprimer (delete) | Sends the character to the recycle bin and rebuilds the archives: nothing of it is left in the game |
| 2. Désactiver / réactiver (disable / enable) | `_` prefix on the folder (skipped by CrabeLoader), then rebuilds the archives |
| 3. Nettoyer (clean up) | Tidies characters made with an older version (one 13 MB archive each), offers to send the `backups/` copies to the recycle bin |
| 4. Reconstruire (rebuild) | Forces a rebuild of the shared archives |

When to run it: to delete or disable a character (rather than deleting its folder), after deleting a folder by hand,
or when the install reports "shared archives NOT rebuilt" (game was open). The list says "Archives partagées à jour"
(up to date) or what needs rebuilding. Command line: `--list`, `--rebuild`, `--clean`.

## Understanding the deformation score

After "Build the rig", Creator measures how much the mesh stretches or squashes in the test poses (arms raised to
70°, walk at 35°; the "Pose amount" slider changes the amplitude).

The score (0 to 100) blends three measures: the average stretch of all edges (50 %), the share of edges deformed more
than 2× (30 %) and the share of edges torn beyond 3× (20 %).

| Score | Level |
| :---: | :--- |
| 75 and above | Good |
| 50 to 74 | Acceptable |
| below 50 | Heavy deformation |

- Edges stretched more than 3× are drawn in **red** ("Show stretched edges in red" box).
- If the model is **squashed or flattened** (broken weights), the score shows "FAILED - the model is collapsed".
- This score measures geometry, **not appearance**: a model can be "good" and badly rigged (hair following the arms).
  Always check visually, and lock troublesome pieces to the right bone.

## Command-line tools

Without opening the interface (run with `.\.venv\Scripts\python.exe`):

| Command | Purpose |
| :--- | :--- |
| `tools\batch_check.py folder [--base ACTOR \| --suggest] [--blender path]` | Places and rigs every model of a folder and prints the score; with `--suggest`, tries every figure and gives the best |
| `tools\capture.py model.glb [--base ACTOR] [--amount 1.0] [--out folder]` | Builds a model's rig and saves 3-view pictures (rest, arms up, walk, with stretched edges in red) and `report.txt` to `build\captures\` |
| `Capture_Model.bat` | Same, by dropping a model onto the file |
| `Audit_Animations.bat` | Animation audit of a model (report + pictures), outside Creator |
| `Manage_Mods.bat` / `tools\manage_mods.py` | Manage the installed characters (see above) |

## Playing your character

1. **Restart the game**: new characters are only loaded at startup.
2. In game, either **F5** (CrabeMenu) → **Heroes › ★ Modded Characters**, or the **My collection › Characters** grid,
   where your character has its own tile.

## Tips for a good result

- **Vertex count**: under about 65,000 (engine limit, 16-bit indices). The cleanup decimates if needed.
- **A clean mesh**: small pieces (eyes, buttons, accessories) are attached to the nearest part, but a model cut into
  dozens of pieces rigs less well.
- **A true T-pose**: straight horizontal arms, legs slightly apart, facing front.
- **Install Blender**: softer bends at shoulders, elbows and knees when the heat computation succeeds.
- **Proportions close to the base**: limbs are stretched to match the base's bone lengths (within limits). Very
  different proportions give a stretched character in game.
- **Textures**: import a `.zip` with the images next to the model, or a `.glb` with embedded textures.

## How it works

Each playable figure is an **actor**: a skeleton, a skinned mesh, materials and textures, and the animations and
abilities that drive it. Creator makes a **cloned actor**: a copy of the base figure whose mesh is replaced by yours,
under a new name.

1. **Skeleton fitting**: horizontal slices of the model give the arm line, neck, crotch, knees and ankles; the
   standard joints are placed on them, then corrected by small models calibrated on the game's own figures. Centring
   ignores hair and wings so the skeleton is not shifted.
2. **Bone mapping**: each joint is linked to the closest bone the base actually has; a missing position is rebuilt
   from the proportions of a reference figure.
3. **Skinning**: Blender's bone-heat weights, or else distance to the bones. Blender fails on open meshes or meshes
   with many pieces: if fewer than 70 % of the vertices get a weight, Creator uses the distance method. Then: rigid
   head (the eyes do not slide), pieces below the knees bound to an ankle, pieces bridging arms and legs follow the
   spine, smoothing of the panels between the legs (skirts, dresses), the game's twist bones on arms and thighs, and
   finally the hand-locked pieces.
4. **Baking into the rest pose**: the mesh is not rigged onto a new skeleton. Each body part is rotated and stretched
   from your model's joint to the matching joint of the base, so the mesh fits the base's rest pose exactly. The base's
   skeleton and matrices do not change: **every animation of the game applies as is**.
5. **Writing the files**: Creator reads and writes the game's formats directly: Octane scenes (`.oct`, `.bent`),
   compiled Lua (`.dnax`), material bundles (`.mtb`), UI bundles and the encrypted archives, losslessly (rewriting an
   untouched file gives back the same bytes).

### Contents of a character mod

| File | Role |
| :--- | :--- |
| `characters/<actor>/<actor>.oct`, `_0.vbuf`, `_0.ibuf` | The mesh: vertices, weights, triangles, bounds |
| `characters/<actor>/<actor>.mtb` | The materials, with textures |
| `characters/<actor>/<actor>.dnax`, `.bent`, `.animtreeoverrides` | The actor definition, pointing to the new mesh |
| `textures/<id>.tbody` | The character's textures (colour and relief), sources of the shared archive |
| `ui/icons/…`, `ui/virtualreadercharacters/…`, `<actor>_icon.png` | Icons (sources of the shared archive) |
| `skilltrees/ActorList.patch`, `skilltrees/DataMap.patch` | Register the actor in the game's databases |
| `characters/<Actor>.lua` | Adds the figure to the character grid |
| `<name>.lua`, `mod.json` | CrabeLoader entry point and CrabeMenu entry |

## Known limitations

- **Moveset**: the character uses its base's moves, voice, abilities and skill tree; no custom animations.
- **Textures**: only the model's base colour is used (the relief is derived from that colour, not from a real
  normal map of the model). One texture per character (atlas), capped at 2048 px.
- **Non-humanoid figures** (Olaf, Randall, Maleficent, Elastigirl): unusable as a base.
- **Adapted skeletons** (◐): a few joints rebuilt approximately.
- **Capes, skirts, hair**: without successful bone heat, distance skinning is rougher; the score does not see
  everything, check the poses and lock pieces if needed.
- **Automatic skeleton placement**: the main source of error. Run back through Creator, the game's figures drift by
  about 3.5 % from their original shape with the auto-fit, against 1.6 % with their real joints. Check the shoulders,
  elbows and pelvis in the Skeleton tab first.
- **Maturity**: tested in game on the Luke, Anakin and Gamora bases; the other bases use the same pipeline but have
  been played less.

## Troubleshooting

| Problem | Try |
| :--- | :--- |
| The app closes on launch | Run `Launch_Creator.bat` (the window stays open) and read `build\creator.log` |
| Black textures or "IN" icon in game | Close the game, `Manage_Mods.bat` → 4 (rebuild); the icon only exists if "Character icons" was ticked at install |
| The character does not show up in game | Restart the game; check CrabeLoader; read `loader.log` in the game folder |
| "model too detailed" | Reduce below ~65,000 vertices (the cleanup does it) |
| Model lying down or facing away | Tab 1: **Stand up**, **↻ 90°** / **↻ 180°**, **⇅ Flip** |
| Skeleton offset from the body | Tab 1: position sliders or **Recentre**; "3 views" to check |
| The model turns into a needle / "collapsed" | Blender's weights failed; Creator normally falls back to distance. Rebuild the rig and read the log |
| Grey model on import | Read the yellow warning: texture not linked. Put the images next to the model (in the same `.zip`) |
| Elbows or knees bending badly | Tab 2: move the matching slider, then rebuild |
| Hair or cape following the arms | Ctrl+click the piece, Grow, pick the bone (e.g. Head), Lock, then rebuild |
| Stiff or blocky bends | Set the Blender path (Rig & Export tab) |
| The character slides without moving its legs | The base's animations are missing from the install: reinstall or verify the files through Steam |
| "already a figure of the game" | Choose another internal actor name |

## For developers

```
src/
  core/       pure logic, no UI
    octane_scene.py, luac.py, material_bundle.py        lossless read/write of .oct/.bent, .dnax, .mtb
    base_character.py, base_pack.py, roster.py          base figures
    skeleton_map.py                                      Creator joints -> bones of each figure
    model.py, skeleton.py, rig.py, rigging.py            imported model, joints, skinning
    quality.py, ranking.py                               deformation score, base ranking
    cleaner.py, errorlog.py                              built-in Blender cleanup, error log
    octane.py, exporter.py, mod_writer.py, textures.py   mod output
    icon.py, normalmap.py, twist.py                      icons, relief, twist bones
    shared.py, octane_zip.py                             shared mod, the game's encrypted archives
  render/     off-screen OpenGL viewport (also used by tools/capture.py)
  ui/         customtkinter interface
  scripts/    background Blender steps (cleanup, bone-heat weights, freezing a rigged .glb)
  assets/     bases/ (figure pack), roster.json, shader_layouts.json, templates/
  tools/      QuickBMS and the archive re-encrypter (offsetting.exe)
tools/        batch_check.py, capture.py, audit.py, manage_mods.py, scripts that rebuild src/assets
              from a game install, measuring tools (roundtrip.py, fit_calibration.py, twist_profiles.py)
tests/        unit tests (pytest)
```

Tests: `.\.venv\Scripts\python.exe -m pytest tests`.

`tools/roundtrip.py <ACTOR>` runs an official figure through Creator and compares it with the original (shape,
weights, skeleton); `--true-skeleton` and `--official-weights` isolate each source of error. `Sync_Claude.bat` packs
`src/assets` into one archive for a remote development session.

### Game resources

`src/assets/` and `src/tools/` (extracted from the game, third-party binaries) are **not** versioned here. They are
rebuilt from a game install:

| Script | Produces |
| :--- | :--- |
| `tools/build_roster.py <virtualreaderpc_data.lua> <toybox_actors.lua>` | `roster.json`: every playable figure |
| `tools/build_shader_table.py <decrypted game root>` | `shader_layouts.json`: texture slots per shader |
| `tools/build_base_pack.py <game folder>` | `bases/`: model, `.dnax`, animations and preview textures of each figure |

QuickBMS (with the Disney Infinity BMS script) and `offsetting.exe` go in `src/tools/`.

### Conventions

- Main branch: `main`. Commits `type(scope): summary` with a body that says why; each version is described in
  `CHANGELOG.md` (in French). Releasing: update `VERSION` and the changelog, GitHub Actions creates the tag and the
  release.
- `build/`, `backups/`, `.venv/`, `src/assets/`, `src/tools/` and `src/config/settings.json` (personal paths) are not
  tracked, nor are 3D models (`*.glb`, `*.fbx`, `*.dae`…).

## Credits

- **QuickBMS** by Luigi Auriemma and the community's Disney Infinity BMS script.
- The Disney Infinity modding community (archive re-encrypter, format research).
- [trimesh](https://trimesh.org), [ModernGL](https://github.com/moderngl/moderngl),
  [CustomTkinter](https://github.com/TomSchimansky/CustomTkinter), NumPy, SciPy, Pillow,
  [cryptography](https://cryptography.io) and [Blender](https://www.blender.org).

Disney Infinity and all related characters are trademarks of their respective owners. This project is intended for
owners of the game.
