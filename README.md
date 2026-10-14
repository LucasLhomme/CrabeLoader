# CrabeLoader v1.0.0

[![CI - Build & Test](https://github.com/LucasLhomme/CrabeLoader/actions/workflows/ci.yml/badge.svg)](https://github.com/LucasLhomme/CrabeLoader/actions/workflows/ci.yml)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Win32%20(x86)-lightgrey.svg)]()

An open-source, high-performance mod loader and runtime extensibility platform for **Disney Infinity 3.0: Gold Edition (PC)**.

CrabeLoader is a native plugin, `CrabeLoader.asi`, started by a small bootstrap `dinput8.dll`; both sit next to the game executable (no game file is modified). It intercepts DirectX 11 presentation calls and the internal Lua 5.1 virtual machine to provide a resilient, modern environment for community modifications.

---

## Architecture Overview

CrabeLoader v1.0.0 follows a strict **decoupled, layered architecture**:

* **Core Platform (C++23):** Operates strictly as a neutral infrastructure layer. It contains **zero gameplay logic, zero cheats, and zero hardcoded menus**. Its responsibilities are limited to DirectX 11 hooks, Structured Exception Handling (SEH) crash guards, a multi-buffer UI command pipeline, and generic memory primitives.
* **Modding Layer (Lua 5.1):** All gameplay mechanics, user interfaces (such as [CrabeMenu](https://github.com/LucasLhomme/CrabeMenu)), camera modifications, custom characters, and skill trees execute as sandboxed Lua modules.

---

## Key Capabilities

* **Embedded Runtime:** Core API modules are embedded directly within `CrabeLoader.asi`, removing disk dependencies in the game root.
* **Two-File Install:** A small `dinput8.dll` bootstrap forwards DirectInput to the system's copy and starts `CrabeLoader.asi` at the game's entry point, before any game code runs. If `CrabeLoader.asi` is missing or fails to start, the bootstrap writes a warning to `loader.log` and the game starts without CrabeLoader.
* **Thread-Safe Decoupled UI (`DrawBuffer`):** Mod UI logic executes safely on the script thread, publishing commands that are rendered natively on the DirectX 11 Present loop without thread contention or VM state corruption.
* **Hardware Exception Armor (SEH):** Unhandled native exceptions and invalid memory operations are trapped and logged to `loader.log`, preventing silent crashes to desktop.
* **Isolated Environment Sandboxes:** Each mod operates in an isolated environment with read-only proxy protection over critical engine namespaces (`Game`, `table`, `string`, `math`, `_G`).
* **Runtime Hot-Reloading (`F4`):** Scripts and modular packages in `mods/` can be reloaded on the fly in under 50ms without restarting the executable.
* **Dynamic Instruction-Boundary Code Caves:** Integrated HDE32 disassembler guarantees safe hook installation without truncating multi-byte x86 instructions.
* **Display & Focus Management:** Integrated Borderless Windowed mode toggle with automatic cursor liberation during overlay interactions.
* **Native `.asi` Plugins:** 32-bit `.asi` plugins load from the game folder, `scripts/`, `plugins/` or a mod's folder before the game's own code runs, with each file checked first and a crashing plugin skipped instead of ending the game. An optional C plugin API (`include/sdk/crabe_plugin.h`) lets a plugin log, hook through the hook registry, run Lua, and share values with Lua mods. CrabeLoader is itself an `.asi`, so a player who already has another ASI loader (such as Ultimate ASI Loader as `dinput8.dll`) can copy `CrabeLoader.asi` alone next to the game and let that loader start it.

---

## Modular Mod Layout

All user modifications reside strictly inside the `mods/` directory, while runtime configuration and diagnostic logs are stored under `Crabe/`:

```text
Disney Infinity 3.0 Gold Edition/
├── DisneyInfinity3.exe       <- Game executable (untouched)
├── dinput8.dll               <- Bootstrap, starts CrabeLoader.asi
├── CrabeLoader.asi           <- The loader
├── bink2w32.dll              <- Game's own Bink DLL (untouched)
├── loader.log                <- Diagnostics and logs
├── Crabe/
│   ├── crabe.toml            <- Main configuration file
│   └── crash-*.txt, *.dmp    <- Crash reports and minidumps (only after a crash)
├── scripts/, plugins/        <- Optional: native .asi plugins (also accepted next to the exe)
└── mods/                     <- Target mod directory
    ├── standalone_mod.lua    <- Single-file mod
    └── hero_expansion/       <- Modular mod package
        ├── mod.json          <- Manifest metadata
        ├── main.lua          <- Entry script
        ├── hero_native.asi   <- Optional native half, loaded only while the mod is enabled
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

[plugins]
asi  = true            # Loads native .asi plugins before the game starts
skip = []              # File names never to load, e.g. ["example.asi"]

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
  * `logLevel`: Logging verbosity (`"debug"`, `"info"`, `"warning"`, `"error"`). Logs are written to `loader.log`, next to `DisneyInfinity3.exe`.
  * `profile`: Active profile key (`"default"`). References a matching `[profiles.<name>]` section.
* **`[display]`**
  * `windowMode`: Window presentation mode (`"borderless"` or `"windowed"`). Pressing `Alt + Enter` in-game toggles between them and persists the choice.
* **`[keybinds]`**
  * `hotReload`: Virtual key name for reloading all scripts and VFS overrides in real-time (default: `"F4"`).
  * `devOverlay`: Virtual key name for toggling the native ImGui developer console (default: `"Insert"`).
* **`[updates]`**
  * `check`: When `true`, CrabeLoader queries the GitHub Releases API in the background on startup. If a newer stable release is found, a non-blocking prompt is shown. Set to `false` to disable network access entirely.
* **`[plugins]`**
  * `asi`: When `false`, no `.asi` plugin is loaded; each one found is still listed in `loader.log` as skipped.
  * `skip`: Array of `.asi` file names never to load, compared without regard to case or folder. A `crabe.toml` without a `[plugins]` section loads every plugin.
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
| **[Native `.asi` Plugins](docs/guides/asi_plugins.md)** | Where `.asi` plugins load from and when, `[plugins]`, `Crabe.Plugins`, and the C plugin API (`include/sdk/crabe_plugin.h`). |
| **[Update Check & Installer](docs/guides/updates_and_installer.md)** | The launch-time update prompt (and how to disable it), `CrabeInstaller.exe` and the two files it installs, and what each sends or changes. |
| **[API Type Definitions](docs/crabe_api.def.lua)** | EmmyLua annotations providing full IntelliSense and autocomplete for IDEs. |
| **[Architecture Blueprint](docs/ARCHITECTURE_BLUEPRINT.md)** | Core architectural invariants, threading rules, and layer definitions. |

---

## Installation

Each [release](https://github.com/LucasLhomme/CrabeLoader/releases/latest) offers two ways to install. Both end with the same result, and the release also lists SHA-256 checksums (`SHA256SUMS.txt`).

### Option A: `CrabeInstaller.exe` (recommended)

Run it and follow the prompts. It finds the game in your Steam libraries (or lets you pick the folder), installs `dinput8.dll` and `CrabeLoader.asi` without renaming or modifying any game file, and creates the `mods/` folder with a short readme. Running it again updates CrabeLoader and leaves your mods and settings alone. It also moves a 1.0 install (`bink2w32.dll` + `bink2w32_orig.dll`) over, putting the game's own `bink2w32.dll` back. It never overwrites a `dinput8.dll` or a `CrabeLoader.asi` that is not CrabeLoader's (such as an `.asi` plugin loader), and if anything fails part-way it puts every file back as it was; see the [installer guide](docs/guides/updates_and_installer.md).

### Option B: manual

1. Copy the release `dinput8.dll` and `CrabeLoader.asi` into your game installation folder (e.g., `<steam-library>\steamapps\common\Disney Infinity 3.0 Gold Edition`), next to `DisneyInfinity3.exe`.
2. Create a `mods/` directory next to them.
3. Coming from 1.0: delete the old `bink2w32.dll`, then rename `bink2w32_orig.dll` back to `bink2w32.dll`.

Already have an ASI loader installed as `dinput8.dll` (such as Ultimate ASI Loader)? Keep it, and copy only `CrabeLoader.asi` next to `DisneyInfinity3.exe`: your ASI loader starts CrabeLoader. In that case the `.asi` plugins in the game folder, `scripts/` and `plugins/` are loaded by your ASI loader, CrabeLoader loads only the ones inside mod folders, and CrabeLoader's start time depends on your ASI loader. See [Native `.asi` Plugins](docs/guides/asi_plugins.md#other-asi-loaders).

### Uninstalling

Delete `dinput8.dll` and `CrabeLoader.asi`. No game file was changed by the install.

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

The loader is built as `build/Release/CrabeLoader.asi` and the bootstrap as `build/Release/dinput8.dll`. Add `-DCRABELOADER_BUILD_INSTALLER=ON` to also build `build/Release/CrabeInstaller.exe` with both files embedded.

## Contributing

Contributions are welcome! Please read **[CONTRIBUTING.md](CONTRIBUTING.md)** for detailed guidelines on our architecture invariants, C++23 standards, Git workflow, and submission checklist.

---

## License

CrabeLoader is distributed under the terms of the **[GNU General Public License v3.0](LICENSE)**.
