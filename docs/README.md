# CrabeLoader V2 Documentation

Welcome to the documentation repository for **CrabeLoader V2** — the decoupled, high-performance modloader and engine platform for *Disney Infinity 3.0: Gold Edition (PC)*.

---

## 📖 Table of Contents

### 1. In-Depth Modding Guides (`docs/guides/`)
A dedicated suite of structured guides in plain English explaining every modding subsystem:
* **[Mod Development Guide](guides/mods.md)** — Lifecycle hooks (`onInit`, `onUpdate`, `onDraw`, `onShutdown`), sandboxing, Dear ImGui UI creation, memory patching (`Crabe.Memory.patchBytes`, `Crabe.Hooks.installCodeCave`), and live hot-reloading (`F4`).
* **[Skill Tree Modding Guide](guides/skilltrees.md)** — How character progression works in the engine, writing `.patch` chunk modifications and `.lua` full source overrides, structure of `SkillTree.Nodes` and `CharacterStats`.
* **[Character & Figurine Guide](guides/characters.md)** — The 5-stage actor resolution pipeline (`SKU -> AvatarData -> Name -> ActorList -> DNA`), adding cosmetic variants (`addCharacter`), exposing standalone unreleased heroes (`exposeCharacter`, e.g. Mace Windu, Thanos), and SKU numbering ranges.
* **[Guides Overview](guides/README.md)** — Summary index of all modding guides.

### 2. Getting Started
* **[Quickstart Guide](modding.md)** — Minimal "Hello World" mod walkthrough to get your first script running in under two minutes.

### 3. Engine & API References
* **[Native Function Database](nativedb.md)** — Comprehensive catalog of 1,200+ native C++ engine functions exposed to Lua.
* **[Lua API Type Definitions (`crabe_api.def.lua`)](crabe_api.def.lua)** — EmmyLua type definitions for code completion and static analysis in VS Code / IDEs.

### 4. Architectural Invariants & Blueprint
* **[Architecture Blueprint (Long-Term Reference)](ARCHITECTURE_BLUEPRINT.md)** — Invariant rules, the 5-layer stack, decision flowchart ("where does my code belong?"), and the Definition of Done (DoD).
