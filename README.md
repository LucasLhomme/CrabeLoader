# CrabeLoader v1.0.0

[![CI - Build & Test](https://github.com/LucasLhomme/CrabeLoader/actions/workflows/ci.yml/badge.svg)](https://github.com/LucasLhomme/CrabeLoader/actions/workflows/ci.yml)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Win32%20(x86)-lightgrey.svg)]()

An open-source, high-performance mod loader and runtime extensibility platform for **Disney Infinity 3.0: Gold Edition (PC)**.

CrabeLoader attaches as a proxy `bink2w32.dll`, intercepting DirectX 11 presentation calls and the internal Lua 5.1 virtual machine to provide a resilient, modern environment for community modifications.

---

## Architecture Overview

CrabeLoader v1.0.0 follows a strict **decoupled, layered architecture**:

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

All user modifications reside strictly inside the `mods/` directory, while runtime configuration and diagnostic logs are stored under `Crabe/`:

```text
Disney Infinity 3.0 Gold Edition/
├── bink2w32.dll              <- CrabeLoader proxy DLL
├── bink2w32_orig.dll         <- Original game Bink DLL
├── Crabe/
│   ├── crabe.toml            <- Main configuration file
│   └── loader.log            <- Diagnostics, logs, and minidumps
└── mods/                     <- Target mod directory
    ├── standalone_mod.lua    <- Single-file mod
    └── hero_expansion/       <- Modular mod package
        ├── mod.json          <- Manifest metadata
        ├── main.lua          <- Entry script
        ├── characters/       <- Bundled character definitions
        └── skilltrees/       <- Bundled progression patches
```

---

## Configuration (`crabe.toml`)

CrabeLoader is configured via a standard TOML file located at `<GameRoot>/Crabe/crabe.toml`. It is generated automatically on the first launch with sensible defaults. If the file contains syntax errors, CrabeLoader logs the exact line and column to `loader.log`, ignores the file in favour of defaults without overwriting it, and keeps your modifications safe.

```toml
# CrabeLoader configuration (<GameRoot>/Crabe/crabe.toml)

[general]
language  = "en"       # UI language (informational)
logLevel  = "info"     # debug | info | warning | error
profile   = "default"  # Active mod profile from [profiles.*]

[display]
windowMode = "borderless" # "borderless" | "windowed" (hot-toggle with Alt+Enter)

[keybinds]
hotReload  = "F4"      # Reloads every active mod from disk
devOverlay = "Insert"  # Toggles the ImGui developer overlay / console

[updates]
check = true           # Checks GitHub at launch for new stable releases (non-blocking)

# Mod profiles allow managing multiple mod loadouts.
# Use "*" to load all discovered mods, or list specific mod IDs.
[profiles.default]
enabled = ["*"]

# Optional profile example (switch by setting [general].profile = "speedrun")
#[profiles.speedrun]
#enabled = ["com.example.crabemenu"]

# [quarantine] is managed automatically by the runtime fault-isolation engine.
# Misbehaving mod callbacks that throw repeatedly are quarantined here without crashing the game.
```

### Configuration Options Reference

* **`[general]`**
  * `language`: UI localization string (`"en"`).
  * `logLevel`: Logging verbosity (`"debug"`, `"info"`, `"warning"`, `"error"`). Logs are written to `Crabe/loader.log`.
  * `profile`: Active profile key (`"default"`). References a matching `[profiles.<name>]` section.
* **`[display]`**
  * `windowMode`: Window presentation mode (`"borderless"` or `"windowed"`). Pressing `Alt + Enter` in-game toggles between them and persists the choice.
* **`[keybinds]`**
  * `hotReload`: Virtual key name for reloading all scripts and VFS overrides in real-time (default: `"F4"`).
  * `devOverlay`: Virtual key name for toggling the native ImGui developer console (default: `"Insert"`).
* **`[updates]`**
  * `check`: When `true`, CrabeLoader queries the GitHub Releases API in the background on startup. If a newer stable release is found, a non-blocking prompt is shown. Set to `false` to disable network access entirely.
* **`[profiles.<name>]`**
  * `enabled`: Array of strings specifying which mods to load. Use `["*"]` to load all valid mods found in `mods/`, or specify explicit mod identifiers (e.g. `["local.mymod", "com.author.mod"]`).

---

## Documentation Suite

Comprehensive technical guides are available in the [`docs/`](docs/) directory:

| Document | Description |
| :--- | :--- |
| **[Mod Development Guide](docs/guides/mods.md)** | Lifecycle hooks (`onInit`, `onUpdate`, `onDraw`, `onShutdown`), Dear ImGui widgets, and event dispatching. |
| **[Skill Tree Guide](docs/guides/skilltrees.md)** | Progression tree overrides (`.lua`) and binary chunk patches (`.patch`). |
| **[Character & Figurine Guide](docs/guides/characters.md)** | Figurine resolution pipeline, custom character registration, and costume variants. |
| **[Update Check & Installer](docs/guides/updates_and_installer.md)** | The launch-time update prompt (and how to disable it), `CrabeInstaller.exe`, and what each sends or changes. |
| **[API Type Definitions](docs/crabe_api.def.lua)** | EmmyLua annotations providing full IntelliSense and autocomplete for IDEs. |
| **[Architecture Blueprint](docs/ARCHITECTURE_BLUEPRINT.md)** | Core architectural invariants, threading rules, and layer definitions. |

---

## Installation

Each [release](https://github.com/LucasLhomme/CrabeLoader/releases/latest) offers two ways to install. Both end with the same result, and the release also lists SHA-256 checksums (`SHA256SUMS.txt`).

### Option A: `CrabeInstaller.exe` (recommended)

Run it and follow the prompts. It finds the game in your Steam libraries (or lets you pick the folder), installs the proxy, keeps the game's own `bink2w32.dll` as `bink2w32_orig.dll`, and creates the `mods/` folder with a short readme. Running it again updates CrabeLoader and leaves your mods and settings alone. It never overwrites a `bink2w32.dll` it does not recognise; see the [installer guide](docs/guides/updates_and_installer.md).

### Option B: manual

1. In your game installation folder (e.g., `<steam-library>\steamapps\common\Disney Infinity 3.0 Gold Edition`), rename the original `bink2w32.dll` to `bink2w32_orig.dll`.
2. Copy the release `bink2w32.dll` into the game root directory.
3. Create a `mods/` directory next to it.

### Then

1. Place your mods or packages into the `mods/` directory (such as **[CrabeMenu](https://github.com/LucasLhomme/CrabeMenu)** for an interactive in-game menu).
2. Launch `DisneyInfinity3.exe`.
   * **`Insert`**: Toggle developer debug overlay and log console.
   * **`F4`**: Hot-reload all active mods.
   * **`F5`**: Toggle in-game mod menu (optional, requires **[CrabeMenu](https://github.com/LucasLhomme/CrabeMenu)**).

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

The compiled binary will be generated at `build/Release/bink2w32.dll`. Add `-DCRABELOADER_BUILD_INSTALLER=ON` to also build `build/Release/CrabeInstaller.exe` with that DLL embedded.

## Contributing

Contributions are welcome! Please read **[CONTRIBUTING.md](CONTRIBUTING.md)** for detailed guidelines on our architecture invariants, C++23 standards, Git workflow, and submission checklist.

---

## License

CrabeLoader is distributed under the terms of the **[GNU General Public License v3.0](LICENSE)**.
