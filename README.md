# CrabeLoader

An open-source modloader for **Disney Infinity 3.0 (PC)**. It injects into
the game as a proxy `bink2w32.dll`, hooks the game's own Lua VM, and exposes
a stable Lua API (`Crabe.*`/`Game.*`) so mods never need to touch the game's
memory through anything but that API.

CrabeLoader itself ships no gameplay mods — it's the platform. See
[docs/modding.md](docs/modding.md) to write one, and
[docs/nativedb.md](docs/nativedb.md) for the game's native function catalog.
[CrabeLoader-Splitscreen](https://github.com/LucasLhomme/CrabeLoader-Splitscreen) (not working for now)
is an example real mod built on top of it.

## Building

Requirements: CMake ≥ 3.21, Visual Studio 2022 (MSVC, C++23), Windows (the
game is Win32-only).

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -DCRABELOADER_AS_SHARED=ON
cmake --build build --config Release
```

`CRABELOADER_AS_SHARED=ON` is required to produce the loader DLL
(`build/Release/bink2w32.dll`); without it, CMake builds a standalone `.exe`
useful only for quick local testing of non-injected code paths.

Options (`cmake -D<OPTION>=ON/OFF`):

| Option                      | Default | What it does                                                         |
| --------------------------- | ------- | -------------------------------------------------------------------- |
| `CRABELOADER_AS_SHARED`   | `OFF` | Build as a DLL (needed for real injection) instead of an`.exe`.    |
| `CRABELOADER_BUILD_IMGUI` | `ON`  | Build the bundled Dear ImGui overlay (console,`Insert` to toggle). |

## Installing

Disney Infinity 3.0 loads `bink2w32.dll` from its own game folder — that's
the hook point, no separate injector needed.

1. In the game folder, rename the existing `bink2w32.dll` to
   `bink2w32_orig.dll` (CrabeLoader forwards every real Bink export to it —
   video playback still works normally).
2. Copy the built `bink2w32.dll` (from `build/Release/`) into the game
   folder.
3. Copy this repo's `src/api/` folder to the game folder as `api/` (the
   loader reads it from disk at runtime, relative to the game's working
   directory — it is not compiled into the DLL).
4. Create a `mods/` folder next to it (or drop mod `.lua` files there — see
   [docs/modding.md](docs/modding.md)).
5. Launch the game. `loader.log` appears next to the DLL; press `Insert`
   in-game for the console/overlay.

## Writing mods

Short version: **never** hardcode a game memory address in a mod — the
loader already exposes everything needed (call any function by address,
read/write memory, trace messages, hotkeys, per-frame ticks) as a stable
API. Full guide: [docs/modding.md](docs/modding.md).

## Herald

im using [Herald](https://herald.codes/) for review the codebase of this repo. thanks to [Graven](https://github.com/GravenilvecTV) for his tools !

## License

CrabeLoader is licensed under the [GNU General Public License v3.0](LICENSE).
This means any modified or redistributed version of CrabeLoader must also stay
open source under the same license — it cannot be turned into a closed-source
fork.
