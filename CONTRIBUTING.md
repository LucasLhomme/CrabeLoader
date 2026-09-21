# Contributing to CrabeLoader

Thank you for your interest in contributing to **CrabeLoader**! 

CrabeLoader is an open-source, high-performance mod loader and runtime platform for *Disney Infinity 3.0: Gold Edition (PC)*. We welcome contributions from developers, reverse engineers, and modders of all experience levels.

---

## 🏛️ Core Architectural Invariants

Before writing any code, it is vital to understand our core architectural rule:

> **The C++ Loader is strictly an infrastructure platform.**  
> It must contain **zero gameplay logic, zero cheats, and zero hardcoded menus**.

* **What belongs in C++ (`CrabeLoader`):** Direct3D 11 hooks, Win32 window procedures, Lua VM interception, memory scanning / patching primitives, dynamic instruction disassembly (HDE32), hardware SEH crash guards, and the multi-buffer UI pipeline (`DrawBuffer`).
* **What belongs in Lua (`mods/` / `CrabeMenu`):** Cheats (god mode, speedhacks, teleports), freecam, custom menus, character definitions, and progression trees.

For full architectural details, consult the **[Architecture Blueprint](docs/ARCHITECTURE_BLUEPRINT.md)**.

---

## 🛠️ Development Setup

### Prerequisites
* **Operating System:** Windows 10 or 11 (64-bit host)
* **Target Architecture:** **Win32 (x86, 32-bit)** (Disney Infinity 3.0 is a 32-bit binary)
* **Compiler:** Visual Studio 2022 (MSVC v143) with **C++23** support
* **Build System:** CMake ≥ 3.21
* **Scripting:** Python 3.10+ (used by tooling to embed the runtime API)
* **Git:** With submodules enabled

### Clone & Configure

```powershell
git clone --recurse-submodules https://github.com/LucasLhomme/CrabeLoader.git
cd CrabeLoader

# Generate Visual Studio 2022 solution targeting Win32
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -DCRABELOADER_AS_SHARED=ON

# Make `git blame` skip the repository-wide mechanical rewrites
git config blame.ignoreRevsFile .git-blame-ignore-revs
```

### Build

```powershell
cmake --build build --config Release
```

The compiled proxy DLL will be located at:
```text
build/Release/bink2w32.dll
```

---

## 🔄 Git Workflow & Branching Strategy

We follow a structured GitFlow-inspired model:

* **`main`**: Production-ready, stable releases only (e.g. `v0.2.0`). **Do not open PRs directly against `main`**.
* **`dev`**: Active development integration branch. **All Pull Requests must target `dev`**.
* **`feature/<name>`** or **`fix/<name>`**: Individual feature or bugfix branches created off `dev`.

### Step-by-Step:
1. **Fork** the repository on GitHub.
2. **Branch** off `dev`:
   ```bash
   git checkout dev
   git pull origin dev
   git checkout -b feature/my-cool-improvement
   ```
3. Make your changes and verify that the project builds cleanly.
4. **Commit** using [Conventional Commits](https://www.conventionalcommits.org/):
   ```bash
   git commit -m "feat(render): add scissor rect support to DrawBuffer"
   ```
5. **Push** to your fork and open a Pull Request targeting **`dev`**.

---

## 💻 Coding Standards

### Modern C++ (C++23)

1. **Strict RAII:** Raw `new` and `delete` are strictly forbidden. Use `std::unique_ptr` for exclusive ownership, and `std::shared_ptr` only when shared lifetime is genuinely required.
2. **Rule of Zero:** Do not define custom destructors, copy, or move operations unless managing a raw low-level Win32/DirectX resource.
3. **Encapsulation & Safety:**
   * Class member variables must be `private` or `protected` with `m_` or `_` prefix (remain consistent with the file).
   * Mark non-modifying member functions `const` and `noexcept` where applicable.
   * Mark single-argument constructors `explicit`.
   * Mark virtual overrides `override` and leaf classes/methods `final`.
   * Pass non-trivial parameters by `const &`.
4. **Strict Thread Separation (DirectX vs. Script Thread):**
   * Direct3D 11 API calls are **only** permitted on the render thread inside `RenderHook::hkPresent`.
   * Lua VM calls are **only** permitted on the engine script thread inside `Loader::runTicks`.
   * Never invoke the Lua VM directly from `Present` — use the decoupled `DrawBuffer` command pipeline.

### Lua Runtime & Embedded API (`src/api/`)

* The engine embeds **Lua 5.1**. Do not use LuaJIT extensions, Lua 5.2+ bitwise operators (`&`, `|`), or `goto`.
* If you modify any script in `src/api/*.lua`, you **must re-generate the embedded bytecode header** before committing:
  ```powershell
  python tools/embed_api.py
  ```
  *(This automatically generates `include/application/embedded_api.hpp`).*

---

## 🧪 Testing Your Changes

1. **Deploy Locally:** Copy your built `build/Release/bink2w32.dll` to your game directory (backed up with the original `bink2w32.dll` renamed to `bink2w32_orig.dll`).
2. **Verify Logs:** Inspect `loader.log` generated in the game root directory. Verify there are no hook errors, syntax warnings, or unexpected exceptions.
3. **Hot-Reload Testing:** If you are testing Lua scripts, press **`F4`** in-game to verify that your modifications reload cleanly without leaking state.
4. **Automated CI:** Every PR triggers our GitHub Actions pipeline (`CI - Build & Test`), building the solution on `windows-latest` MSVC and validating artifact production.

---

## 📋 Pull Request Checklist

Before submitting your PR, ensure:

- [ ] Target branch is set to **`dev`** (not `main`).
- [ ] Code builds without warnings under MSVC (`/W4` or equivalent).
- [ ] No raw `new`/`delete` or unmanaged Win32 memory handles.
- [ ] If `src/api/` was edited, `python tools/embed_api.py` was executed and `embedded_api.hpp` was updated.
- [ ] Commits follow Conventional Commits formatting (`feat:`, `fix:`, `refactor:`, `docs:`, `chore:`).
- [ ] Any new feature is documented in the corresponding [`docs/guides/`](docs/guides/) file.

---

## 💬 Community & Questions

Need help or want to discuss reverse engineering ideas?
* Open an **[Issue](https://github.com/LucasLhomme/CrabeLoader/issues)** or a Discussion thread.
* Check existing reverse-engineering references and documentation in [`docs/`](docs/).
