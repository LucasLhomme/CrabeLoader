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
2. Close the game. Copy the **32-bit** ReShade 6.x DLL to
   `<game>/Crabe/ReShade32.dll`. Its original filename may be `d3d11.dll` or
   `dxgi.dll`; verify it is ReShade before copying it.
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

## Lifecycle and limitations

The runtime is created on the first non-test Present and rendered on the render
thread. It is destroyed before ResizeBuffers releases the old back buffer, then
recreated on the next Present. Runtime creation failures and trapped native faults
disable the bridge for the session and are logged. The DLL stays loaded because
its input hooks can still be executing.

Color, sharpening, LUT and other effects using the back buffer are the intended
use case. Automatic depth-buffer detection depends on graphics interception and
is not provided by this path; depth-dependent effects need additional integration.
Only one primary swap chain is hosted. Changes to the DLL require restarting the
game. Loading and initial shader compilation may pause startup.

## Diagnose

Look for these entries in `loader.log`:

- `ReShadeBridge: loaded Crabe/ReShade32.dll with automatic graphics hooks disabled.`
- `ReShadeBridge: manual D3D11 effect runtime created on the game's existing swap chain.`

`ReShade.log` remains in the game root and reports shader compilation. A runtime
creation message proves initialization; verify Home, an enabled effect, resizing,
and gameplay separately. A long gameplay session has not been established by a
successful startup alone.

## Disable or restore the proxy installation

With the game closed, rename `Crabe/ReShade32.dll` to `ReShade32.dll.disabled` to
disable the bridge. To return to the normal proxy installation, also rename the
root backup (for example `d3d11.dll.proxy-disabled`) back to its original name.
Do not enable both paths at once.

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
