# ReShade through CrabeLoader

CrabeLoader can host a ReShade 6.x effect runtime on the existing Direct3D 11
swap chain. This is an optional alternative to the normal `d3d11.dll` / `dxgi.dll`
installation. It uses the official `ReShadeCreateEffectRuntime`,
`ReShadeUpdateAndPresentEffectRuntime`, and `ReShadeDestroyEffectRuntime` exports.

The automatic ReShade graphics hooks are disabled while the DLL loads. The game
keeps its original D3D11 COM objects. RTSS can continue hooking the native graphics
runtime and applying its frame limit. CrabeLoader renders ReShade before its own
menu, so menu text is not post-processed.

## Install

1. Build and deploy CrabeLoader normally.
2. For built-in and external add-ons, build the companion DLL using the command
   below. Close the game and copy `build/reshade-manual-addons/ReShade32.dll` to
   `<game>/Crabe/ReShade32.dll`. A stock **32-bit** ReShade 6.x DLL still works for
   ordinary effects, but its manual runtime does not initialize add-ons.
3. Rename the original ReShade graphics proxy in the game root to a name that
   Windows will not load, such as `d3d11.dll.proxy-disabled`. Do not rename a
   system DLL or a graphics DLL belonging to another mod.
4. Keep `ReShade.ini`, the preset, and `reshade-shaders/` in the game root.
5. Launch normally, with RTSS enabled. Press Home to open ReShade.

The presence of `Crabe/ReShade32.dll` enables the bridge; no Lua or CrabeMenu
change is needed. The loader refuses manual initialization if another ReShade
instance is already loaded. When the file is absent, it does not load ReShade or
alter the environment. The temporary environment changes apply only to the game
process during loading and are immediately restored.

## Create the custom ReShade DLL

Run these commands from the root of your CrabeLoader checkout on Windows.
Install these prerequisites first:

- Git for Windows, available as `git` in the terminal.
- Visual Studio 2022 or its Build Tools, including the **Desktop development
  with C++** workload, MSVC v143, and a Windows SDK with the shader compiler.
- Network access to GitHub for the initial source and dependency downloads.

```powershell
& .\tools\reshade\build.ps1
```

The script performs the complete custom DLL build:

1. Clone official `crosire/reshade` tag `v6.8.0` and its submodules into
   `build/reshade-src`. Verify the pinned commit
   `18deaa52de0c425a78b329e9cb3c497281cd00ec` before making changes.
2. Apply `tools/reshade/manual-addons.patch` to the manual D3D11 runtime API.
   This adds built-in/external add-on lifecycle and frame events.
3. Copy `tools/reshade/crabe_manual_depth.hpp` into the ReShade source tree.
   This supplies native draw/depth-clear and deferred-command capture.
4. Locate MSBuild through `vswhere`, then build **Release / Win32**. Upstream's
   Release configuration enables full add-on support (`RESHADE_ADDON=2`).
5. Copy the result to **`build/reshade-manual-addons/ReShade32.dll`**.

Everything downloaded or compiled stays in ignored `build/`; only the patch,
capture source, build script and tests belong in Git. Re-running the script
recognizes an already-applied patch and refreshes the capture header. It refuses
a different ReShade revision or an unpatched checkout with unrelated edits.
After editing the patch itself, use a fresh source checkout for the new patch.
The script accepts `-SourceDir` for that purpose and an optional `-MSBuild` path.
No ReShade binary is committed, and the build script does not deploy game files.

### Install the custom DLL

Close Disney Infinity completely. Back up the existing DLL, then copy the custom
build into the bridge's exact loading path:

```powershell
$gameDir = 'D:\Games\steamapps\common\Disney Infinity 3.0 Gold Edition'
$installed = Join-Path $gameDir 'Crabe\ReShade32.dll'
if (Test-Path -LiteralPath $installed) {
    $backupName = $installed + '.' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.backup'
    Copy-Item -LiteralPath $installed -Destination $backupName
}
Copy-Item -LiteralPath .\build\reshade-manual-addons\ReShade32.dll -Destination $installed -Force
Get-FileHash -LiteralPath .\build\reshade-manual-addons\ReShade32.dll, $installed
```

Both hashes must match. Adjust `$gameDir` for another Steam library. Keep the
root `d3d11.dll`/`dxgi.dll` ReShade proxy disabled, and keep the game's existing
`ReShade.ini`, preset and shader folders. Restart the game; F4 cannot replace a
loaded DLL. The locally compiled DLL may identify itself as **UNOFFICIAL**.

The companion exports `CrabeReShadeManualAddonSupport` (version 2). The updated
CrabeLoader bridge logs support for both manual add-on initialization and native
game depth events. A stock ReShade DLL lacks this capability marker; rebuilding
CrabeLoader alone does not create or replace the custom ReShade DLL.

The Add-ons tab should list the built-in **Generic Depth** and **Effect Runtime
Sync** entries without installing any external files. For external add-ons, set
`[ADDON] AddonPath` in the game-root `ReShade.ini`; it defaults to the game root.
Use `.addon32` files for this 32-bit game. Any 64-bit DLSS helper and its add-ons
belong in their own process and folder. Nothing installs DLSS5-Feeder for you.

## Native depth capture

The companion forwards native D3D11 draws, depth clears, viewport/depth bindings,
and deferred command-list execution to Generic Depth. It hooks method code,
leaving game COM object identities and returned pointers intact. Optimized D3D11
method variants are discovered after calls; unchanged method tables use a cache.
ReShade's own rendering and native command-list replay are excluded to prevent
counting a draw twice. Retained depth resources expire after 120 unused frames
and are released on runtime destruction; deferred contexts are retained until
runtime destruction. Final command-list release removes their tracking state.

Test in a **3D gameplay scene**, then open Add-ons > Generic Depth. A flat menu
may have no meaningful depth. Allow several rendered frames for its heuristics.
Enable **Copy depth buffer before clear operations** if the game clears scene
depth before presentation. DisplayDepth should then show scene geometry; use its
reversed-depth setting only after a real depth buffer is available. Multisampled
depth remains subject to ReShade's depth-resolve capabilities; start with in-game
MSAA disabled. DLSS5-Feeder itself is not installed or game-validated by this fix.

Useful diagnostics in `ReShade.log`:

- `Crabe depth: native draw/clear event forwarding enabled (COM identities preserved).`
- `Crabe depth: observed ... depth resource (samples=...).`

### Isolated lifecycle test

```powershell
cmake -S tools/reshade/tests -B build/reshade-smoke -G "Visual Studio 17 2022" -A Win32 -DRESHADE_SOURCE="$PWD/build/reshade-src"
cmake --build build/reshade-smoke --config Release
& ./build/reshade-smoke/Release/manual_addon_smoke.exe ./build/reshade-manual-addons/ReShade32.dll ./build/reshade-smoke/Release/manual_addon_probe.addon32
```

The test uses a hidden WARP D3D11 window and a unique temporary configuration. It
checks Generic Depth's actual device/runtime private state and external add-on
init/frame/destroy callbacks. A real D3D11 scene alternates depth 0.25 and 0.75
between immediate and deferred draws, clears the source before presentation,
and reads back the texture bound to the DEPTH semantic. It checks at least 60
GPU depth samples, excludes duplicate/effect draw events, and exercises three
destroy/recreate/resize cycles plus invalid-input rejection. It does not launch the game or alter its files.
A stock DLL fails the capability check, providing a negative control. Test logs
remain in the reported temporary directory for inspection.

## Lifecycle and limitations

The runtime is created on the first non-test Present and rendered on the render
thread. It is destroyed before ResizeBuffers releases the old back buffer, then
recreated on the next Present. Runtime creation failures and trapped native faults
disable the bridge for the session and are logged. The DLL stays loaded because
its input hooks can still be executing.

Color, sharpening, LUT and other effects using the back buffer remain supported.
The companion DLL initializes add-ons and supplies native depth events as
described above. These hooks observe a subset of D3D11 events for depth capture;
they do not provide the complete event surface of ReShade's normal graphics proxy.
Only one primary swap chain is hosted. Changes to the DLL require restarting the
game. Loading and initial shader compilation may pause startup.

## Diagnose

Look for these entries in `loader.log`:

- `ReShadeBridge: loaded Crabe/ReShade32.dll with automatic graphics hooks disabled.`
- `ReShadeBridge: manual D3D11 effect runtime created on the game's existing swap chain.`
- `ReShadeBridge: manual add-on lifecycle supported; built-in and external add-ons will initialize with the runtime.` (companion DLL only)

With the companion DLL, `ReShade.log` also reports `Searching for add-ons` and
external add-on loading. A full add-on build by itself is not sufficient when
using the stock manual-runtime exports; use the companion build above.

`ReShade.log` remains in the game root and reports shader compilation. A runtime
creation message proves initialization; verify Home, an enabled effect, resizing,
and gameplay separately. A long gameplay session has not been established by a
successful startup alone.

## Disable or restore the proxy installation

With the game closed, rename `Crabe/ReShade32.dll` to `ReShade32.dll.disabled` to
disable the bridge. To return to the normal proxy installation, also rename the
root backup (for example `d3d11.dll.proxy-disabled`) back to its original name.
Do not enable both paths at once.

## In-game depth validation

The user confirmed the native-depth build in Disney Infinity 3.0 gameplay at
2560x1440. Generic Depth listed full-resolution D32S8 buffers with draw activity,
and DisplayDepth showed scene geometry, reconstructed normals and distance
shading. This verifies game depth reaches ReShade through the manual bridge.
Long-session stability and DLSS5-Feeder integration still need separate testing.

## Validation observations

The proxy installation of ReShade 6.8.0.2157 initialized and compiled shaders,
then crashed near a warning that the ID3D11Device0 reference count was
inconsistent. Disabling RTSS's Direct3D10 renderer did not fix it. The latest
proxy crash attempted execution at 0x3137325F, with game return addresses at
RVAs 0x7BD66 and 0x7CB3E. These observations suggest an invalid object/lifetime
interaction; they do not identify the component responsible for corruption.

The first bridge test created the manual runtime, compiled the installed shader
set, and remained responsive with both RTSSHooks.dll and ReShade32.dll loaded.
The original device-reference warning did not appear during that startup.
Manual testing confirmed that Home opens the overlay and enabled effects work. The first runtime
creation paused for about 21 seconds before effect shader compilation began.

The leading explanation for this pause is ReShade's synchronous update check.
In 6.8, runtime construction calls `check_for_update()` immediately after the
adapter log. That function opens `https://api.github.com/repos/crosire/reshade/tags`
synchronously and applies its two-second request timeouts only *after*
`InternetOpenUrl` returns. The network call itself was not instrumented, so the
attribution remains an inference.
The installed effect shaders compiled afterward in roughly 0.2 seconds.
No startup-delay workaround has been applied.

Matching upstream API and implementation:
- https://github.com/crosire/reshade/blob/v6.8.0/include/reshade.hpp
- https://github.com/crosire/reshade/blob/v6.8.0/source/addon.cpp
- https://github.com/crosire/reshade/blob/v6.8.0/source/dll_main.cpp

- https://github.com/crosire/reshade/blob/v6.8.0/source/runtime_update_check.cpp
