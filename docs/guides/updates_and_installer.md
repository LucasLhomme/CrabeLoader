# Update Check & CrabeInstaller

Two small, optional pieces make CrabeLoader easy to install and keep current: a **launch-time update check** inside the DLL, and **`CrabeInstaller.exe`**, a one-click installer.

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

The built loader reads its version from the `VERSION` file, and the check compares it with the release **tag**. The release workflow therefore refuses a tag that differs from `VERSION`; otherwise players of the new release would be offered that same release forever.

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
3. It installs `bink2w32.dll` (the proxy) and keeps the game's own DLL as `bink2w32_orig.dll`.
4. It creates `mods\` and `mods\LISEZMOI.txt` (English first, then French).

### The rule that protects the original DLL

The installer **never overwrites the game's real `bink2w32.dll`**. The real file is recognised by its SHA-256, and only then renamed:

| State of the folder | What happens |
| :--- | :--- |
| `bink2w32_orig.dll` exists | A real original is already safe. Only `bink2w32.dll` is replaced (an **update**). The backup is never touched. |
| No backup, `bink2w32.dll` is the known original | **Fresh install**: it is renamed to `bink2w32_orig.dll`, then the loader takes its place. |
| No backup, `bink2w32.dll` is anything else (including CrabeLoader's own DLL) | **Refused, nothing changed.** The message tells the player to use Steam's *Verify integrity of game files*. |
| No backup, no `bink2w32.dll` | Refused, nothing changed. |

How it stays safe even when something goes wrong:

* Every rename uses a **no-replace** move, so it fails rather than overwrite.
* The loader is first written to `bink2w32.dll.crabe-new`, hashed back, and only then moved into place.
* If the final move fails after the original was renamed, the original is renamed back.
* An update while the game is running fails (Windows will not replace a loaded DLL), with a clear message and everything left as found.
* An existing `mods\LISEZMOI.txt` is never overwritten.

The recognised original lives in `include/installer/install_plan.hpp` (`kKnownOriginalSha256`). If another edition of the game ships a different `bink2w32.dll`, add its hash there.

### Command line

```text
CrabeInstaller [--game-dir <folder>] [--quiet] [--payload <bink2w32.dll>]
```

| Option | Effect |
| :--- | :--- |
| `--game-dir` | Skip detection and the folder dialog. |
| `--quiet` | Show no window. Exit code: `0` installed, `1` refused or failed, `2` cancelled. |
| `--payload` | Install this DLL instead of the bundled one. |

### Building it

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 `
      -DCRABELOADER_AS_SHARED=ON -DCRABELOADER_BUILD_INSTALLER=ON
cmake --build build --config Release --target CrabeInstaller
```

By default the loader DLL is **embedded** in the executable (resource 101), so `CrabeInstaller.exe` is one self-contained file and is rebuilt with the freshest DLL every time. With `-DCRABELOADER_INSTALLER_EMBED_DLL=OFF` the installer instead looks for `bink2w32.dll` beside itself, which ships as two files (an installer plus the DLL) and avoids a DLL hidden inside an executable, a pattern some antivirus heuristics dislike.

Tests: `tests/cpp/test_installer.cpp` drives the real file operations in a temporary folder, including a locked file. The dialogs themselves can only be checked by eye; see `docs/testing/manual_checklist.md`.

### Releases

The release workflow attaches `CrabeInstaller.exe` (Option A), the raw `bink2w32.dll` (Option B), a zip with both plus the mod template, and `SHA256SUMS.txt`.

### Limits worth knowing

* Only one `bink2w32.dll` is recognised as the game's original: the Steam build's. A different edition is refused, not guessed at; add its hash to `kKnownOriginalSha256`.
* The repository the update check asks is fixed in `include/domain/update_check.hpp`. A fork that publishes its own releases changes the three constants there.
* There is no uninstall button; the steps are under "Uninstalling" below.

### Installing by hand

Without the installer: rename the game's `bink2w32.dll` to `bink2w32_orig.dll`, copy CrabeLoader's `bink2w32.dll` in its place, and create a `mods` folder.

### Uninstalling

Delete `bink2w32.dll` and rename `bink2w32_orig.dll` back to `bink2w32.dll` (or use Steam's *Verify integrity of game files*).
