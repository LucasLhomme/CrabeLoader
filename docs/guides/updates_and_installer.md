# Update Check & CrabeInstaller

Two small, optional pieces make CrabeLoader easy to install and keep current: a **launch-time update check** inside `CrabeLoader.asi`, and **`CrabeInstaller.exe`**, a one-click installer.

---

## 1. The update check

At every game launch, CrabeLoader quietly asks GitHub for the latest published release. Nothing happens unless that release is newer than the running build.

| Situation | What the player sees |
| :--- | :--- |
| Up to date, offline, GitHub unreachable or rate limited | Nothing. One line in `loader.log`. |
| A newer **stable** release exists | A native Windows box: *"Do you want to update the modloader?"* (French on a French Windows). |
| Player clicks **Yes** | The default browser opens `https://github.com/LucasLhomme/CrabeLoader/releases/latest`. |
| Player clicks **No** (or the default, **No**) | Nothing more until the next launch, when the question is asked again. |

Design choices worth knowing:

* **It never blocks the game.** The request and the box live on their own thread, so the game keeps running while the box is open.
* **The default button is "No".** A key pressed in game never opens a browser by accident.
* **Drafts and prereleases are ignored**, as is any tag that is not a semantic version.
* **The page that opens is fixed in the code.** It is never taken from GitHub's answer, so a tampered response cannot send the player elsewhere.
* **It runs before the game-build check.** A game build CrabeLoader does not recognise is exactly when a newer loader helps most.

### Privacy

The only request is an HTTPS `GET` to `api.github.com` for this repository's latest release. GitHub sees the player's IP address and a `User-Agent` of `CrabeLoader/<version>`. Nothing else is sent: no game data, no identifier, no telemetry. The answer is read for three fields (`tag_name`, `draft`, `prerelease`) and discarded.

### Releasing

`VERSION` is the single source of truth: the built loader reports it, and the release tag is derived from it (`v<VERSION>`), so a loader and its own release can never disagree. If they could, players of a release would be offered that same release forever.

1. Merge into `main` with a new `VERSION` (plain `X.Y.Z`).
2. The release workflow builds, runs the tests and prepares a **draft** release, flagged pre-release. A draft is visible to maintainers only: it is not on the public releases page, it has no tag yet, and its files can only be downloaded while signed in to GitHub with write access.
3. Download and test `CrabeInstaller.exe` from the draft.
4. Publish the draft (*Releases, Edit, Publish release*). It becomes public as a pre-release, and the tag is created at that moment.
5. Untick "Set as a pre-release" when players should be told. From then on the update check sees it and they are asked.

The update check only sees published, stable releases, so nobody is prompted before step 5. While a release is still a draft, every merge into `main` rebuilds it, so it always matches `main`; once published it is frozen, and a merge that leaves `VERSION` unchanged does nothing. A `VERSION` that is not plain `X.Y.Z` fails the run, since a suffix would hide the release from the update check. Releases are only ever prepared from `main`.

To hand a build to a scanner or to someone else before publishing, download it from the draft and send the file itself: a draft has no public link.

### Turning it off

`Crabe/crabe.toml` (created on first launch):

```toml
[updates]
check = false   # never connect to GitHub
```

A `crabe.toml` written before this section existed keeps the check on.

### Trying it without the game

`update_probe.exe` runs the same code outside the game (build target `update_probe`):

```powershell
cmake --build build --config Release --target update_probe
.\build\Release\update_probe.exe --current 0.1.0 --fake-latest v9.9.9 --no-open
.\build\Release\update_probe.exe                      # the real request, against GitHub
```

`--fake-latest` skips the network, `--no-open` prints the URL instead of opening the browser, `--current` pretends to be another version. Until a release is published, the real request ends in *"HTTP status 404"* in the log: that is expected.

### Where it lives

| Layer | File |
| :--- | :--- |
| Domain (pure) | `include/domain/update_check.hpp`, `src/domain/update_check.cpp` |
| Application | `src/application/update_checker.cpp`, `src/application/update_launcher.cpp` |
| Infrastructure | `src/infrastructure/update_services.cpp` (WinHTTP, `MessageBoxW`, `ShellExecuteExW`) |
| Tests | `tests/cpp/test_update_checker.cpp` |

> **Note:** when `[multiplayer].enabled = true`, the multiplayer module hooks WinHTTP process-wide and relaxes certificate checks. The update check inherits that. The worst a forged answer can do is show a false prompt, and **Yes** still opens only the fixed releases page.

---

## 2. CrabeInstaller.exe

A single executable that installs CrabeLoader without any manual file work.

1. It looks for the game in your Steam libraries and asks **Yes / No / Cancel**. If it finds nothing, or you say **No**, it opens the native Windows folder picker (`IFileOpenDialog`).
2. It checks that the folder contains `DisneyInfinity3.exe`.
3. It installs CrabeLoader's two files next to `DisneyInfinity3.exe`: `dinput8.dll`, a small bootstrap, and `CrabeLoader.asi`, the loader itself. No game file is renamed or modified.
4. It creates `mods\` and `mods\README.txt` (English first, then French).

### The two files

**`dinput8.dll`, the bootstrap.** The game imports `DINPUT8.dll`, and Windows looks in the game folder before the system folders, so the bootstrap is the one loaded. It forwards the six DirectInput exports (`DirectInput8Create`, `DllCanUnloadNow`, `DllGetClassObject`, `DllRegisterServer`, `DllUnregisterServer`, `GetdfDIJoystick`) to the system's own `dinput8.dll` (System32, which Windows redirects to SysWOW64 for this 32-bit game). At the game's entry point, before any game code runs, it loads `CrabeLoader.asi` from the game folder and calls its exported `CrabeLoaderBootstrap()` function. If `CrabeLoader.asi` is missing or fails to start, the bootstrap appends a `Bootstrap:` warning line to `loader.log` and the game starts without CrabeLoader. The code is in `src/bootstrap/`.

**`CrabeLoader.asi`, the loader.** Everything else lives here: the hooks, the Lua runtime, the overlay, the `.asi` plugin loader and the plugin API. When the bootstrap calls `CrabeLoaderBootstrap()`, CrabeLoader loads the other `.asi` plugins right there, before the game's code runs. Because it is an `.asi`, another ASI loader can start it instead of the bootstrap; see [Other ASI loaders](asi_plugins.md#other-asi-loaders).

Because no game file changes, Steam's *Verify integrity of game files* no longer removes CrabeLoader.

`CrabeLoader.asi` still carries the old Bink forwarders (`include/minhook/lib/proxy/proxy.hpp`). If two CrabeLoader copies are loaded (for example a leftover 1.0 `bink2w32.dll` and `CrabeLoader.asi`), only one runs the loader, chosen by file name: `bink2w32.dll` first, then `dinput8.dll` (a 1.1 development single file), then any other name such as `CrabeLoader.asi` (`src/infrastructure/loader_instance.cpp`). The bootstrap carries a different description, so it never counts as a loader copy. The first line of `loader.log` names the copy that runs, normally `loaded as CrabeLoader.asi`.

### The rule that protects the game folder

Each file is recognised by the file description in its version resource (`include/shared/loader_marker.hpp`): `CrabeLoader Proxy DLL for Disney Infinity 3.0` for the loader (every release since 1.0.0 carries it) and `CrabeLoader Bootstrap for Disney Infinity 3.0` for the bootstrap. The installer **never overwrites a game file, or a `dinput8.dll` or `CrabeLoader.asi` that is not CrabeLoader's**:

| State of the folder | What happens |
| :--- | :--- |
| Neither `dinput8.dll` nor `CrabeLoader.asi` | **Fresh install**: both files are written. |
| `dinput8.dll` is the bootstrap and `CrabeLoader.asi` is CrabeLoader (or either one is missing) | **Update**: both files are replaced or added. |
| `dinput8.dll` is a 1.1 development single-file CrabeLoader | **Update**: it is replaced by the bootstrap, and `CrabeLoader.asi` is added. |
| `dinput8.dll` is anything else (often an `.asi` plugin loader such as Ultimate ASI Loader) | **Refused (`BootstrapSlotTaken`), nothing changed.** The message says that if this `dinput8.dll` is an ASI loader, the player can simply copy `CrabeLoader.asi` next to the game instead; otherwise they must move or delete that `dinput8.dll`. |
| `CrabeLoader.asi` is anything other than CrabeLoader | **Refused (`LoaderSlotTaken`), nothing changed.** The player must move or delete that file. |
| `bink2w32.dll` is CrabeLoader (or missing) and `bink2w32_orig.dll` exists | **Migration from 1.0**: the game's original is put back as `bink2w32.dll` (the old CrabeLoader is retired), and both files are installed. |
| `bink2w32.dll` is the game's own, beside an old `bink2w32_orig.dll` | Both are left alone; the two files are installed or updated as above. |
| `bink2w32.dll` is an old CrabeLoader and `bink2w32_orig.dll` is gone | **Refused (`LegacyOriginalMissing`), nothing changed**, since the game could not play its Bink videos. The message tells the player to use Steam's *Verify integrity of game files*. |

How it stays safe even when something goes wrong:

* Both bundled files are checked first: each must be a Windows DLL of a sensible size carrying the right description, so a swapped or damaged payload is refused before anything is written.
* Each file is first written beside its final name as `<name>.crabe-new` (`CrabeLoader.asi.crabe-new`, `dinput8.dll.crabe-new`), hashed back, and only then moved into place.
* A previous CrabeLoader file is not deleted straight away: it is retired beside itself as `<name>.crabe-old` and only removed once the whole install has succeeded.
* Renames use a **no-replace** move, so they fail rather than overwrite.
* Any failure part-way (a file in use, for example) rolls everything back: the new files are removed, the retired `.crabe-old` files are restored, and the folder is left as it was found. If even a restore fails, the error message says which file could not be put back.
* While `DisneyInfinity3.exe` is running, the installer refuses before touching anything and asks you to close the game. Windows would let it rename a loaded DLL, so an install would otherwise only half-apply until the next launch.
* An existing `mods\README.txt` is never overwritten.

The decision itself lives in `include/installer/install_plan.hpp` and `src/installer/install_plan.cpp`; the file operations are in `src/installer/game_installer.cpp`.

### Command line

```text
CrabeInstaller [--game-dir <folder>] [--quiet] [--loader <CrabeLoader.asi>] [--bootstrap <dinput8.dll>]
```

| Option | Effect |
| :--- | :--- |
| `--game-dir` | Skip detection and the folder dialog. |
| `--quiet` | Show no window. Exit code: `0` installed, `1` refused or failed, `2` cancelled. |
| `--loader` | Install this `CrabeLoader.asi` instead of the bundled one. `--payload` is kept as an old name for this option. |
| `--bootstrap` | Install this `dinput8.dll` instead of the bundled one. |

### Building it

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 `
      -DCRABELOADER_AS_SHARED=ON -DCRABELOADER_BUILD_INSTALLER=ON
cmake --build build --config Release --target CrabeInstaller
```

By default both files are **embedded** in the executable (`CrabeLoader.asi` as resource 101, `dinput8.dll` as resource 102), so `CrabeInstaller.exe` is one self-contained file and is rebuilt with the freshest files every time. With `-DCRABELOADER_INSTALLER_EMBED_DLL=OFF` the installer instead looks for `CrabeLoader.asi` and `dinput8.dll` beside itself, which ships as three files and avoids DLLs hidden inside an executable, a pattern some antivirus heuristics dislike.

Tests: `tests/cpp/test_installer.cpp` drives the real file operations in a temporary folder: fresh install, update, the upgrade from a 1.1 single-file `dinput8.dll`, the 1.0 migration, every refusal (including a foreign `dinput8.dll` and a foreign `CrabeLoader.asi`), swapped or bad payloads, and a locked file on each of the two files with a full rollback. `tests/cpp/test_bootstrap.cpp` checks how the bootstrap starts `CrabeLoader.asi`. The dialogs themselves can only be checked by eye; see `docs/testing/manual_checklist.md`.

### Releases

Each release carries `CrabeInstaller.exe` (Option A), the raw `dinput8.dll` and `CrabeLoader.asi` (Option B), a zip, and `SHA256SUMS.txt`. The zip holds `dinput8.dll`, `CrabeLoader.asi`, `CrabeInstaller.exe`, `mods/`, `docs/crabe_api.def.lua` and `sdk/crabe_plugin.h`. GitHub adds the source archives itself.

### Limits worth knowing

* Only one program can be loaded as `dinput8.dll`. If another tool (such as an `.asi` plugin loader) already uses that name, the installer refuses rather than replace it. If that tool is an ASI loader, copy `CrabeLoader.asi` next to the game by hand instead: the ASI loader starts it (see [Other ASI loaders](asi_plugins.md#other-asi-loaders)).
* The repository the update check asks is fixed in `include/domain/update_check.hpp`. A fork that publishes its own releases changes the three constants there.
* There is no uninstall button; the steps are under "Uninstalling" below.

### Installing by hand

Without the installer: copy `dinput8.dll` and `CrabeLoader.asi` next to `DisneyInfinity3.exe` and create a `mods` folder.

Coming from 1.0 by hand: delete the old `bink2w32.dll`, then rename `bink2w32_orig.dll` back to `bink2w32.dll`.

Already have an ASI loader installed as `dinput8.dll`: keep it and copy only `CrabeLoader.asi` next to `DisneyInfinity3.exe`.

### Uninstalling

Delete `dinput8.dll` and `CrabeLoader.asi`. No game file was changed by the install. The `mods\README.txt` the installer writes says the same.
