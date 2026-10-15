# CrabeLoader Tools & Reverse Engineering Utilities

A comprehensive guide to the internal reverse engineering, asset extraction, decompilation, and pipeline tools bundled inside the **CrabeLoader repository** (`tools/`).

---

## 1. Tooling Inventory & Overview

The `tools/` directory contains custom and community utilities for analyzing and modding *Disney Infinity 3.0: Gold Edition*:

| Tool / Folder | Primary Language | Function & Purpose |
| :--- | :--- | :--- |
| `tools/character_pipeline/` | Python 3.9+ | Automated character extraction, cloning, retargeting, and HUD generation. |
| `tools/QuickBMS/` | C / Binary | Decrypts and unpacks proprietary AES-128-CTR `.zip` game archives using `disney_infinity_new.bms`. |
| `tools/cheatengine-mcp-bridge/` | Python 3.10+ | Live process memory bridge over Named Pipes (`\\.\pipe\CE_MCP_Bridge_v99`). |
| `tools/unluac/` | Java | Decompiles shipped pre-compiled Lua 5.1 bytecode (`.luac`) into human-readable source code. |
| `tools/.MTB Texture Extractor/` | Python / C# | Parses binary material descriptors (`.mtb`) and extracts referenced 64-bit texture hashes. |
| `tools/Save Game Editor/` | Python / C# | Inspects and edits Toy Box save files and player profile progression. |
| `tools/check_headers.py` | Python | CI linter ensuring consistent file headers and coding standards across C++ and Lua files. |

---

## 2. Unpacking Game Archives: QuickBMS

In retail *Disney Infinity 3.0*, game archives in `assets/` (e.g. `characters/*.zip`, `worlds/*.zip`, `textures/*.zip`) are protected by AES-128-CTR encryption.

### Unpacking an Archive:
```powershell
tools/QuickBMS/quickbms.exe `
    tools/QuickBMS/disney_infinity_new.bms `
    "D:/SteamLibrary/.../assets/characters/emp_luke/emp_luke.zip" `
    "extracted/emp_luke/"
```
This extracts the raw `.bent`, `.oct`, `.mtb`, `.vbuf`, and `.ibuf` stream files into the target folder.

---

## 3. Automated Character Pipeline: `build_character.py`

Located at `tools/character_pipeline/build_character.py`, this is the official CLI for creating standalone character clones in one command:

```powershell
python tools/character_pipeline/build_character.py `
    --base EMP_Luke `
    --name SOR_Sora `
    --mod sora `
    --icon path/to/icon.png `
    --deploy
```

### What It Does:
1. **Unpacks** the base actor (`EMP_Luke`) from `assets/characters/`.
2. **Clones & Retargets**: Rewrites names inside `.bent`, `.oct`, `.mtb`, and `.dnax` while strictly preserving binary length.
3. **Generates HUD Icon**: Converts your PNG image into a `.tbody` DDS texture and builds the companion 80-byte `.mtb` descriptor.
4. **Builds Manifest**: Writes a clean `mod.json` and deployment script.

---

## 4. Decompiling Lua Bytecode: `unluac`

Many game scripts in `assets/` are shipped as compiled Lua 5.1 bytecode (`.luac` or `.dnax` files).

### Decompiling Bytecode:
```powershell
java -jar tools/unluac/unluac.jar assets/scripts/gameplay.luac > gameplay_decompiled.lua
```
This produces clean, readable Lua 5.1 code that you can inspect to discover internal game mechanics, state machines, and quest flows.

---

## 5. Live Memory Inspection: Cheat Engine MCP Bridge

Located in `tools/cheatengine-mcp-bridge/ce.py`, this tool allows AI agents and scripts to inspect and manipulate live memory in `DisneyInfinity3.exe` via Named Pipes without touching the graphical user interface.

### Common Commands:
```powershell
# Open connection to running game
python tools/cheatengine-mcp-bridge/ce.py open_process '{"process_id_or_name":"DisneyInfinity3.exe"}'

# Scan for an AOB pattern
python tools/cheatengine-mcp-bridge/ce.py aob_scan '{"pattern":"8B 0D ?? ?? ?? ??"}'

# Read memory value
python tools/cheatengine-mcp-bridge/ce.py read_memory '{"address":"0x00A45B20", "type":"float"}'
```

---

## 6. Parsing Materials & Textures: `.MTB Texture Extractor`

Materials in Disney Infinity link 3D meshes to 64-bit texture hashes. The `.MTB` parser reads the binary descriptor blocks:
1. Opens `<actor_name>.mtb`.
2. Locates the shader binding offsets.
3. Extracts the 16-hexadecimal character texture hashes (e.g. `a5a0000050a40001`).
4. Allows modders to identify exactly which texture files in `mods/<mod>/textures/` correspond to the diffuse, normal, and specular maps of an object.
