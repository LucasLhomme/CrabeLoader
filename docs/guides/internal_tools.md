# Repository Tools & Build Utilities

A complete developer reference for the internal tools, code generators, asset pipeline scripts, and CI linters **tracked directly inside the CrabeLoader repository** (`tools/` and `src/cli/`).

---

## 1. Tooling Inventory (Tracked in Repository)

The CrabeLoader repository contains a suite of Python, C++, and CMake tools designed to automate character creation, figure registry cryptography, API embedding, and code quality enforcement:

| Tool Path | Language | Primary Purpose |
| :--- | :--- | :--- |
| [`tools/character_pipeline/build_character.py`](file:///E:/Dev/DIM2/CrabeLoader/tools/character_pipeline/build_character.py) | Python 3.9+ | End-to-end CLI to clone, retarget, and generate standalone playable character mods. |
| [`tools/character_pipeline/make_icon.py`](file:///E:/Dev/DIM2/CrabeLoader/tools/character_pipeline/make_icon.py) | Python 3.9+ | Converts square PNG portraits into Octane `.tbody` DDS textures and 80-byte `.mtb` descriptors. |
| [`tools/di3_gateway/di3_gateway.py`](file:///E:/Dev/DIM2/CrabeLoader/tools/di3_gateway/di3_gateway.py) | Python 3.8+ | Reads, decrypts, and edits encrypted AES-128-CBC figure registry (gateway) files. |
| [`tools/embed_api.py`](file:///E:/Dev/DIM2/CrabeLoader/tools/embed_api.py) | Python 3.8+ | Compiles `src/api/*.lua` into `include/application/embedded_api.hpp` byte arrays. |
| [`tools/embed_api_def.py`](file:///E:/Dev/DIM2/CrabeLoader/tools/embed_api_def.py) | Python 3.8+ | Generates EmmyLua definitions and C++ headers for IDE autocomplete and CLI validation. |
| [`tools/generate_choreography_catalog.py`](file:///E:/Dev/DIM2/CrabeLoader/tools/generate_choreography_catalog.py) | Python 3.8+ | Scans shipped choreography archives and generates `src/api/30_choreographies.lua`. |
| [`tools/check_headers.py`](file:///E:/Dev/DIM2/CrabeLoader/tools/check_headers.py) | Python 3.8+ | CI linter enforcing the strict 3-line file description header format across all source files. |
| [`tools/build_stamp.cmake`](file:///E:/Dev/DIM2/CrabeLoader/tools/build_stamp.cmake) | CMake | Injects git commit hash, build timestamp, and compiler flags into the binary. |
| [`src/cli/` (`crabe-cli`)](file:///E:/Dev/DIM2/CrabeLoader/docs/guides/cli.md) | C++23 | The official CrabeLoader CLI tool for mod validation, scaffolding, and dependency resolution. |

---

## 2. Character Creation Pipeline: `build_character.py` & `make_icon.py`

Located under `tools/character_pipeline/`, these tools automate the entire workflow of turning a vanilla actor into a standalone custom character mod.

### 2.1. Fast-Track CLI Usage
```powershell
python tools/character_pipeline/build_character.py `
    --base EMP_Luke `
    --name SOR_Sora `
    --mod sora `
    --icon path/to/icon.png `
    --deploy
```

### 2.2. Pipeline Execution Steps:
1. **Extraction**: Unpacks the base actor's model from `assets/characters/<base>/<base>.zip` and `.dnax` from `assets/characters/characters.zip`.
2. **Binary Byte-Preserving Retargeting**:
   * Clones `.bent`, `.oct`, `.mtb`, `.vbuf`, `.ibuf`, and `.dnax`.
   * Strictly enforces that `--name` and `--base` have the **exact same string length** (e.g. `SOR_Sora` and `EMP_Luke` are both 8 characters) to allow direct in-place binary patching without corrupting Octane scene graph offsets.
   * Modifies body material name so the clone receives its own unique 64-bit texture hash.
3. **HUD Icon Generation (`make_icon.py`)**:
   * Reads a 256×256 or 512×512 PNG image.
   * Compresses it into DirectDraw Surface (DDS DXT5) with mipmaps.
   * Prepends the 16-byte Octane `.tbody` header.
   * Generates the companion 80-byte `.mtb` descriptor.
4. **Manifest Creation**: Generates `mod.json` with the required `character` block.
5. **Deployment**: Deploys the completed mod to `mods/<mod>/`.

---

## 3. Gateway & Figure Registry Tool: `di3_gateway.py`

The gateway is the game's **encrypted figure registry** (`assets/characters/gateway*.zip`). It maps a `sku_id` into a recognized Disney Infinity figure. Without a valid registry entry, the game refuses to play the character, displaying *"Figurine Disney Infinity manquante"*.

Each slot is encrypted with **AES-128-CBC** (`KEY = 000102030405060708090a0b0c0d0e0f`, `IV = 0123456789012345`) and PKCS7 padding.

### 3.1. Available Commands

```powershell
# 1. Verify cryptography reproduces existing gateway files byte-for-byte
python tools/di3_gateway/di3_gateway.py check --game "D:\SteamLibrary\...\Disney Infinity 3.0 Gold Edition"

# 2. List all registered AVATAR slots across all installed playsets
python tools/di3_gateway/di3_gateway.py list --game "D:\SteamLibrary\...\Disney Infinity 3.0 Gold Edition"

# 3. Decrypt and dump every slot into readable text
python tools/di3_gateway/di3_gateway.py dump --game "D:\SteamLibrary\...\Disney Infinity 3.0 Gold Edition"

# 4. Find unused SKU IDs in the safe range
python tools/di3_gateway/di3_gateway.py freesku --game "D:\SteamLibrary\...\Disney Infinity 3.0 Gold Edition"

# 5. Add a new character slot by cloning an existing base slot
python tools/di3_gateway/di3_gateway.py add --game "D:\SteamLibrary\...\Disney Infinity 3.0 Gold Edition" `
    --sku 1000340 --name SOR_Sora --clone-from 1000201 --write
```

---

## 4. API Code Generator: `embed_api.py` & `embed_api_def.py`

CrabeLoader embeds its entire Lua runtime API directly into the binary DLL/ASI so the loader has **zero external disk dependencies** at startup.

### 4.1. How `embed_api.py` Works:
1. Iterates through all scripts in `src/api/*.lua` in alphabetical order.
2. Strips UTF-8 Byte Order Marks (BOMs).
3. Sanitizes file names into valid C++ variable identifiers (`00_core.lua` -> `mod_00_core_lua`).
4. Generates `include/application/embedded_api.hpp` containing `inline constexpr unsigned char` byte arrays.

```powershell
python tools/embed_api.py
```
During compilation, CMake automatically runs `embed_api.py` whenever any file in `src/api/` is modified.

### 4.2. API Definitions Generator: `embed_api_def.py`
Generates `include/cli/embedded_api_def.hpp` and EmmyLua definition tables used by `crabe-cli` to validate mod scripts for typos and nonexistent API functions before launching the game.

---

## 5. Choreography Catalog Generator: `generate_choreography_catalog.py`

In vanilla Disney Infinity 3.0, the game contains thousands of animation choreographies (emotes, victory dances, cutscenes) stored in `assets/choreographies/*.zip`.

Historically, mods attempted to scan this directory at runtime using `io.popen("dir ...")`, which blocked the game thread for several seconds and caused stutter.

### Solution:
`generate_choreography_catalog.py` scans the game directory once and pre-compiles all choreography names into a single indexed string inside `src/api/30_choreographies.lua`:

```powershell
python tools/generate_choreography_catalog.py "D:\SteamLibrary\...\Disney Infinity 3.0 Gold Edition"
```
At runtime, `Game.ListChoreographies()` and `Game.PlayChoreography(name)` provide O(1) instantaneous access with zero disk I/O.

---

## 6. Header Format Linter: `check_headers.py`

To maintain documentation integrity across the codebase, CrabeLoader enforces a strict 3-line description header on every source file.

### Required Header Format:
```cpp
/*
** CrabeLoader
** File description:
** <Line 1: exact responsibility of this file>
** <Line 2: constraint or architectural mechanism the reader must know>
** <Line 3: what this file deliberately does not do, or defers to>
**
** Authors: @LucasLhomme
*/
```

### Running the Linter:
```powershell
# Checks all files; returns exit code 1 if any file fails
python tools/check_headers.py

# Lists every source file currently in scope
python tools/check_headers.py --list
```
This script runs in GitHub Actions CI to guarantee that no un-commented or improperly formatted files are merged.

---

## 7. Official Command-Line Interface: `crabe-cli`

Compiled from `src/cli/`, `crabe-cli` is the command-line utility for modders and automated pipelines:

```powershell
# Scaffold a new mod directory with mod.json and main.lua
crabe-cli new my_mod --template basic

# Validate mod manifest, SemVer dependencies, and directory structure
crabe-cli validate mods/my_mod

# Check Lua files for deprecated or misspelled Crabe/Game API calls
crabe-cli check mods/my_mod/main.lua
```
