# CrabeLoader v0.2.0

[![CI - Build & Test](https://github.com/LucasLhomme/CrabeLoader/actions/workflows/ci.yml/badge.svg)](https://github.com/LucasLhomme/CrabeLoader/actions/workflows/ci.yml)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Win32%20(x86)-lightgrey.svg)]()

An open-source, high-performance mod loader and runtime extensibility platform for **Disney Infinity 3.0: Gold Edition (PC)**.

CrabeLoader attaches as a proxy `bink2w32.dll`, intercepting DirectX 11 presentation calls and the internal Lua 5.1 virtual machine to provide a resilient, modern environment for community modifications.

---

## Architecture Overview

CrabeLoader v0.2.0 follows a strict **decoupled, layered architecture**:

* **Core Platform (C++23):** Operates strictly as a neutral infrastructure layer. It contains **zero gameplay logic, zero cheats, and zero hardcoded menus**. Its responsibilities are limited to DirectX 11 hooks, Structured Exception Handling (SEH) crash guards, a multi-buffer UI command pipeline, and generic memory primitives.
* **Modding Layer (Lua 5.1):** All gameplay mechanics, user interfaces (such as [CrabeMenu](https://github.com/LucasLhomme/CrabeMenu)), camera modifications, custom characters, and skill trees execute as sandboxed Lua modules.

---

## Key Capabilities

* **Embedded Runtime:** Core API modules are embedded directly within `bink2w32.dll`, removing disk dependencies in the game root.
* **Thread-Safe Decoupled UI (`DrawBuffer`):** Mod UI logic executes safely on the script thread, publishing commands that are rendered natively on the DirectX 11 Present loop without thread contention or VM state corruption.
* **Hardware Exception Armor (SEH):** Unhandled native exceptions and invalid memory operations are trapped and logged to `loader.log`, preventing silent crashes to desktop.
* **Isolated Environment Sandboxes:** Each mod operates in an isolated environment with read-only proxy protection over critical engine namespaces (`Game`, `table`, `string`, `math`, `_G`).
* **Runtime Hot-Reloading (`F4`):** Scripts and modular packages in `mods/` can be reloaded on the fly in under 50ms without restarting the executable.
* **Dynamic Instruction-Boundary Code Caves:** Integrated HDE32 disassembler guarantees safe hook installation without truncating multi-byte x86 instructions.
* **Display & Focus Management:** Integrated Borderless Windowed mode toggle with automatic cursor liberation during overlay interactions.

---

## Modular Mod Layout

All user modifications reside strictly inside the `mods/` directory:

```text
Disney Infinity 3.0 Gold Edition/
├── bink2w32.dll              <- CrabeLoader proxy DLL
├── bink2w32_orig.dll         <- Original game Bink DLL
└── mods/                     <- Target mod directory
    ├── standalone_mod.lua    <- Single-file mod
    └── hero_expansion/       <- Modular mod package
        ├── mod.json          <- Manifest metadata
        ├── main.lua          <- Entry script
        ├── characters/       <- Bundled character definitions
        └── skilltrees/       <- Bundled progression patches
```

---

## Documentation Suite

Comprehensive technical guides are available in the [`docs/`](docs/) directory:

| Document | Description |
| :--- | :--- |
| **[Mod Development Guide](docs/guides/mods.md)** | Lifecycle hooks (`onInit`, `onUpdate`, `onDraw`, `onShutdown`), Dear ImGui widgets, and event dispatching. |
| **[Skill Tree Guide](docs/guides/skilltrees.md)** | Progression tree overrides (`.lua`) and binary chunk patches (`.patch`). |
| **[Character & Figurine Guide](docs/guides/characters.md)** | Figurine resolution pipeline, custom character registration, and costume variants. |
| **[API Type Definitions](docs/crabe_api.def.lua)** | EmmyLua annotations providing full IntelliSense and autocomplete for IDEs. |
| **[Architecture Blueprint](docs/ARCHITECTURE_BLUEPRINT.md)** | Core architectural invariants, threading rules, and layer definitions. |

---

## Installation

1. In your game installation folder (e.g., `D:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition`), rename the original `bink2w32.dll` to `bink2w32_orig.dll`.
2. Copy the release `bink2w32.dll` into the game root directory.
3. Place mods or packages into the `mods/` folder.
4. Launch `DisneyInfinity3.exe`.
   * **`Insert`**: Toggle developer debug overlay and log viewer.
   * **`F4`**: Hot-reload all active mods.
   * **`F5`**: Toggle menu interface (when using CrabeMenu).

---

## Building from Source

### Prerequisites
* CMake ≥ 3.21
* Visual Studio 2022 (MSVC with C++23 support)
* Target Architecture: **Win32 (x86)**

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -DCRABELOADER_AS_SHARED=ON
cmake --build build --config Release
```

The compiled binary will be generated at `build/Release/bink2w32.dll`.

---

## License

CrabeLoader is distributed under the terms of the **[GNU General Public License v3.0](LICENSE)**.
