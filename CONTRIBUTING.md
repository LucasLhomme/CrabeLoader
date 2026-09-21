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

### File headers

Every non-vendored, non-generated file under `src/` and `include/` opens with this
header, and `python tools/check_headers.py` fails the build if one is missing,
malformed, or says nothing:

```cpp
/*
** CrabeLoader
** File description:
** <line 1 - what this file is responsible for>
** <line 2 - the constraint or mechanism a reader must know>
** <line 3 - what it deliberately does NOT do, or what it defers to>
**
** Authors: @LucasLhomme
*/
```

Lua files in `src/api/` carry the same content with `--` comments.

**The brief is exactly three lines and each has to earn its place.** Bad, because
it restates the filename and a reader learns nothing:

```cpp
** ModManager implementation
```

Good, because none of it can be read off the code:

```cpp
** Discovers mods, filters by profile, assigns ids, resolves order, then runs each chunk.
** Mod names come from unpacked archives, so both Lua chunks are constants fed values as arguments.
** Decides no order itself -- the rules are in src/domain/dependency_resolver.cpp.
```

Line 3 is the one people skip and the one that saves the most time; prefer naming
the file that owns the adjacent concern. Where a file encodes reverse-engineering
knowledge, line 2 carries the empirical fact rather than restating the mechanism —
`src/infrastructure/lua_symbols.cpp` is the model.

Three *physical* lines, each at most 100 characters including the `** ` prefix.
A wrapping brief cannot be told from a four-line one by a script, so the line is
the unit and concision is the price.

**The `Authors:` line is append-only.** Comma-separated GitHub handles, in the
order people first touched the file. Add the handle of the human directing the
work if it is absent; never remove, reorder or rewrite a name, and never add an
agent or a model. Git stays the source of truth for authorship — resolve any real
question with `git log --follow` and `git blame`, not with this line.

Handle mapping for this repository: the history carries both `Crabe` and
`Lucas Lhomme` as commit-author names, and both are **@LucasLhomme**.

### Comment placement

Comments belong **above** the declaration they describe, and at most three lines.
Do not put comments inside a function body.

> **Known gap:** the existing tree does not follow this yet — roughly a thousand
> comment lines sit inside function bodies, many of them added deliberately to
> record reverse-engineering findings. Bringing them into line is a separate
> sweep and has not been done.

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
- [ ] `python tools/check_headers.py` passes, and any file you created carries the header.
- [ ] Commits follow Conventional Commits formatting (`feat:`, `fix:`, `refactor:`, `docs:`, `chore:`).
- [ ] Any new feature is documented in the corresponding [`docs/guides/`](docs/guides/) file.

---

## 💬 Community & Questions

Need help or want to discuss reverse engineering ideas?
* Open an **[Issue](https://github.com/LucasLhomme/CrabeLoader/issues)** or a Discussion thread.
* Check existing reverse-engineering references and documentation in [`docs/`](docs/).
