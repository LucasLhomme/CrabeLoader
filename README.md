# CrabeLoader V2

An open-source, high-performance modloader and engine platform for **Disney Infinity 3.0: Gold Edition (PC)**. 

CrabeLoader injects into the game process as a proxy `bink2w32.dll`, hooks the game's internal Lua 5.1 virtual machine and DirectX 11 pipeline, and exposes a decoupled, stable Lua API (`Crabe.*` / `Game.*`).

---

## 🏛️ Architecture: CrabeLoader V2

CrabeLoader V2 strictly adheres to modern **SOLID architecture principles**:
* **CrabeLoader (C++23):** Acts strictly as a neutral, resilient infrastructure platform. It contains **zero gameplay logic, zero cheats, and zero hardcoded menus**. Instead, it manages DirectX 11 hooks, Structured Exception Handling (SEH) crash guards, an isolated Lua sandbox with read-only proxy protection, and generic memory/hook primitives.
* **Mods (Lua 5.1):** 100% of game-specific logic, mod menus (like **[CrabeMenu](https://github.com/LucasLhomme)**), camera manipulation, custom skill trees, and gameplay cheats run as isolated Lua mods.

---

## 🚀 Key Features

* **Embedded API:** All 24 core runtime modules are compiled directly into `bink2w32.dll`. No loose `api/` scripts are needed on disk.
* **SEH Hardware Crash Armor:** Mod errors and rendering bugs are intercepted gracefully. They are logged to `loader.log` without crashing the game to desktop.
* **Deep Mod Sandboxing:** Every mod runs in an isolated environment. Engine tables (`Game`, `table`, `string`, `math`, `_G`) are protected by deep read-only proxy metatables.
* **Live Hot-Reloading (`F4`):** Edit scripts in VS Code, switch to the game, and press **`F4`** — all mods reload in under 50ms without restarting the game.
* **DirectX 11 Dear ImGui Overlay:** Mods can render responsive, hardware-accelerated user interfaces directly inside the game loop.
* **Dynamic Instruction Boundary Code Caves:** Code caves disassemble target instructions on-the-fly using an internal HDE32 disassembler to guarantee instructions are never split or corrupted.

---

## 📚 Documentation & Guides

Complete, structured guides in plain English are available in the **[`docs/`](docs/)** directory:

| Guide | Description |
| :--- | :--- |
| **[Mod Development Guide](docs/guides/mods.md)** | Learn how to build mods, handle lifecycle hooks (`onInit`, `onUpdate`, `onDraw`, `onShutdown`), design Dear ImGui interfaces, apply memory patches, and use the event bus. |
| **[Skill Tree Modding Guide](docs/guides/skilltrees.md)** | Deep dive into how Disney Infinity 3.0 compiles progression trees. Learn how to write chunk patches (`.patch`) and full source overrides (`.lua`). |
| **[Character & Figurine Guide](docs/guides/characters.md)** | Learn the 5-step figurine resolution pipeline (`SKU -> AvatarData -> Name -> ActorList -> DNA`), how to expose unreleased heroes (Mace Windu, Thanos), and create costume variants. |
| **[Native Function Database](docs/nativedb.md)** | Catalog of 1,200+ native C++ engine functions exposed to Lua. |
| **[Lua API Type Definitions](docs/crabe_api.def.lua)** | EmmyLua autocomplete and static typing definitions for IDEs. |
| **[Architecture Blueprint](docs/ARCHITECTURE_BLUEPRINT.md)** | Long-term architectural invariants, 5-layer stack, decision flowchart, and Definition of Done. |

---

## 🛠️ Building from Source

Requirements: CMake ≥ 3.21, Visual Studio 2022 (MSVC, C++23), Windows (Win32 x86 target).

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -DCRABELOADER_AS_SHARED=ON
cmake --build build --config Release
```

The compiled proxy DLL will be located at `build/Release/bink2w32.dll`.

---

## 🎮 Installation

1. In your game installation directory (e.g. `D:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition`), rename the original `bink2w32.dll` to `bink2w32_orig.dll`.
2. Copy the compiled `bink2w32.dll` into the game folder.
3. Create a `mods/` folder next to the game executable and place your mods inside (such as `crabemenu.lua`).
4. Launch the game!
   * Press **`F5`** to toggle the mod menu (if using CrabeMenu).
   * Press **`Insert`** to open the developer debug console.
   * Press **`F4`** to hot-reload all mods instantly.

---

## 🔍 Codebase Quality & Review

We use **[Herald](https://herald.codes/)** to audit and enforce strict C++23 and SOLID architecture standards across the repository.

---

## 📜 License

CrabeLoader is licensed under the **[GNU General Public License v3.0](LICENSE)**. Any derivative works or redistributions must also remain open-source under GPL v3.0.
