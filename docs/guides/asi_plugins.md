# Native `.asi` Plugins

CrabeLoader loads `.asi` plugins: native 32-bit DLLs with a `.asi` extension. It loads them the same way
Ultimate ASI Loader does, so most existing plugins work unchanged. Lua mods remain the way to build gameplay
features. An `.asi` plugin is for the rare job that needs native code of its own. Through the optional
[plugin API](#talking-to-crabeloader-the-plugin-api), such a plugin can log, hook, talk to Lua mods and
[draw on the game's swap chain](#drawing-on-the-games-swap-chain).

## CrabeLoader is an `.asi` too

CrabeLoader itself ships as `CrabeLoader.asi`, next to a small bootstrap `dinput8.dll`. The game imports
`dinput8.dll`, so Windows loads the bootstrap from the game folder. The bootstrap forwards DirectInput to
the system's copy and, at the game's entry point, loads `CrabeLoader.asi` and calls its exported
`CrabeLoaderBootstrap()` function. CrabeLoader then loads the other `.asi` plugins right there, before any
of the game's code runs.

If `CrabeLoader.asi` is missing or fails to start, the bootstrap writes a `Bootstrap:` warning line to
`loader.log` and the game starts without CrabeLoader. Because CrabeLoader is an ordinary `.asi`, another
ASI loader can start it instead of the bootstrap; see [Other ASI loaders](#other-asi-loaders).

## Where to put them

CrabeLoader searches four places, in this order, next to `DisneyInfinity3.exe`:

| Folder | Source shown in the log and in Lua |
| :--- | :--- |
| the game folder itself | `game folder` |
| `scripts/` | `scripts` |
| `plugins/` | `plugins` |
| `mods/<mod>/` (the mod's own folder, not its subfolders) | `mod <id>` |

Inside one folder, files load in alphabetical order, ignoring case. Subfolders are not searched, and
mod folders starting with `.` or `_` are skipped, exactly as for Lua mods. `CrabeLoader.asi` itself is
never listed or loaded as a plugin, wherever it sits.

A plugin inside a mod folder belongs to that mod. When the active profile in `crabe.toml` disables the
mod, its `.asi` files are skipped too. The VFS never serves an `.asi` file as a game asset.

## When they load

Plugins load **before any of the game's code runs**. That is at the executable's entry point, after every
DLL the game imports has started, when the bootstrap `dinput8.dll` starts `CrabeLoader.asi`. This is the
moment plugins are written for: they can patch the game before it reads its settings or opens its window.

If CrabeLoader is started later than usual (by another ASI loader, or injected into a running game, for
instance), plugins load from CrabeLoader's own thread as soon as it starts, and the log says so.

Plugins load **once per game launch**. `F4` reloads Lua mods only: native code cannot be safely taken out
of a running game, so adding or changing an `.asi` needs a restart.

## Turning plugins off

`Crabe/crabe.toml` has a `[plugins]` section:

```toml
[plugins]
asi  = true            # false = load no .asi plugin at all
skip = []              # file names never to load, e.g. ["example.asi"]
```

`skip` compares file names only, ignoring case and folder: `"Example.ASI"` skips `scripts/example.asi`.
An existing `crabe.toml` without this section loads every plugin.

## What gets refused

CrabeLoader checks every file before Windows loads it, and keeps the game running whatever a plugin does.

| Status | Meaning |
| :--- | :--- |
| `loaded` | Started normally. |
| `already_loaded` | Another loader already loaded this exact file; it is left as it is. |
| `skipped` | Turned off by `[plugins]` or by a disabled mod, or the file is another copy of CrabeLoader (only one loader may run). |
| `failed` | Not a 32-bit DLL (a 64-bit build, a program, a renamed text file), a missing dependency, or a `DllMain` that returned `FALSE`. |
| `crashed` | Its `DllMain` raised an exception. The plugin is skipped and logged, and the game starts anyway. |

Each plugin gets one line in `loader.log`, then a summary:

```text
[INFO] AsiLoader: loaded widescreen_fix.asi (scripts), at 0x6E2B0000.
[WARNING] AsiLoader: could not load overlay64.asi (plugins): not built for 32-bit Windows (a 64-bit build?); the game only loads 32-bit plugins.
[INFO] AsiLoader: 1 of 2 .asi plugin(s) loaded, 1 failed.
```

## Reading the list from Lua

`Crabe.Plugins` lets a Lua mod check whether a native plugin it relies on is present:

```lua
for _, plugin in ipairs(Crabe.Plugins.list()) do
    Crabe.write(string.format("%s [%s] %s %s", plugin.file, plugin.source, plugin.status, plugin.detail))
end

if not Crabe.Plugins.isLoaded("my_mod_native.asi") then
    Crabe.write("[MyMod] my_mod_native.asi is missing: the native half of this mod is off.")
end
```

`list()` returns one table per plugin found, in load order: `file` (relative to the game folder, forward
slashes), `source`, `status` and `detail`. `isLoaded(name)` matches the file name in any folder and any case.

## Writing a plugin

An `.asi` is an ordinary Win32 DLL with the extension changed. Build it for **x86 (Win32)**, since the game
is 32-bit. Keep `DllMain` short: it runs while Windows holds its loader lock, so start a thread for
anything long.

```cpp
#include <windows.h>

static DWORD WINAPI pluginMain(LPVOID)
{
    // Your native work here.
    return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        CloseHandle(CreateThread(nullptr, 0, &pluginMain, nullptr, 0, nullptr));
    }
    return TRUE;
}
```

A plugin's own folder is searched first for the DLLs it depends on. A plugin in `mods/<mod>/` can
therefore ship its dependencies next to it.

## Talking to CrabeLoader: the plugin API

A plugin can also work with CrabeLoader and Lua mods. Copy the single C header
[`include/sdk/crabe_plugin.h`](../../include/sdk/crabe_plugin.h) into your sources. It is also in the
release zip, under `sdk/`. There is no library to link.

Then export `CrabePluginInit`. CrabeLoader calls it once, right after loading your plugin, on the game's main
thread, before the game's own code runs. It passes a table of functions that stays valid until the game exits:

| Function | What it does |
| :--- | :--- |
| `log(api, level, message)` | Writes a line to `loader.log`, prefixed with your plugin's file name. |
| `installHook(api, target, detour, &original, name)` | Hooks a function through CrabeLoader's hook registry. A target another owner already hooks is refused instead of silently corrupted, and crash reports name your plugin. Returns 1 or 0. |
| `removeHooks(api)` | Removes every hook your plugin installed. |
| `runLua(api, code)` | Queues a Lua chunk for the game's Lua state, with `Crabe` and `Game` available. It runs on the next loader tick, and errors are logged under your plugin's name. |
| `setValue(api, key, value)` / `getValue(api, key, buffer, size)` | Share text values with Lua mods (`Crabe.Plugins.get` / `Crabe.Plugins.set`) and with other plugins. |
| `onPresent(api, callback, user)` | *(API v2)* Calls `callback(swapChain, flags, user)` on the render thread in every `IDXGISwapChain::Present`, before CrabeLoader's overlay. `NULL` removes it. |
| `onBeforeResize(api, callback, user)` | *(API v2)* Calls `callback(swapChain, user)` in `IDXGISwapChain::ResizeBuffers`, before the buffers are released. Drop every back-buffer reference there. `NULL` removes it. |

Every function takes the table first and can be called from any thread. Strings are UTF-8.

```cpp
#include <windows.h>
#include "crabe_plugin.h"

static const CrabePluginApi* g_api = nullptr;

extern "C" __declspec(dllexport) void __cdecl CrabePluginInit(const CrabePluginApi* api)
{
    if (api->version < CRABE_PLUGIN_API_VERSION)
        return;
    g_api = api;
    api->log(api, CRABE_LOG_INFO, "hello from native code");
    api->setValue(api, "my_plugin.ready", "yes");
    api->runLua(api, "Crabe.write('[my_plugin] Lua says hi')");
}
```

And from a Lua mod:

```lua
if Crabe.Plugins.get("my_plugin.ready") == "yes" then
    Crabe.Plugins.set("my_plugin.speed", 2)
end
```

A plugin without `CrabePluginInit` still loads normally: the API is optional. Keys are shared by everyone,
so prefix them with your plugin or mod name. If `CrabePluginInit` raises an exception, it is logged, and the
game keeps running.

## Drawing on the game's swap chain

CrabeLoader already hooks `Present` and `ResizeBuffers`, so a plugin that draws on the game's swap chain
uses `onPresent` and `onBeforeResize` (plugin API v2) instead of hooking them itself. Each plugin has one
callback of each kind; setting it again replaces it. Keep the callbacks cheap, because they run every frame:
do file reads and `LoadLibrary` in `CrabePluginInit`, and register the callbacks there once rather than every
frame. A `Present` callback that raises an exception is logged and not called again, but your resize callback
still runs so you can release the buffers. A resize callback that raises an exception turns both off.

This plugin logs every 600th frame and every resize. A real renderer casts `swapChain` to `IDXGISwapChain*`
and passes its own state as `user`.

```cpp
#include <windows.h>
#include <dxgi.h>
#include "crabe_plugin.h"

static const CrabePluginApi* g_api = nullptr;
static unsigned int g_frames = 0;

static void __cdecl onPresent(void* swapChain, uint32_t flags, void* user)
{
    (void)swapChain;
    (void)user;
    if (!(flags & DXGI_PRESENT_TEST) && ++g_frames % 600 == 0)
        g_api->log(g_api, CRABE_LOG_INFO, "600 more frames presented");
}

static void __cdecl onBeforeResize(void* swapChain, void* user)
{
    (void)swapChain;
    (void)user;
    g_api->log(g_api, CRABE_LOG_INFO, "the swap chain is resizing");
}

extern "C" __declspec(dllexport) void __cdecl CrabePluginInit(const CrabePluginApi* api)
{
    if (api->version < 2)
        return;
    g_api = api;
    api->onPresent(api, &onPresent, nullptr);
    api->onBeforeResize(api, &onBeforeResize, nullptr);
}
```

## Other ASI loaders

CrabeLoader loads `.asi` plugins itself, so a separate ASI loader is not needed. But if you already have
one, such as Ultimate ASI Loader installed as `dinput8.dll`, you can keep it: because CrabeLoader is itself
an `.asi`, just copy `CrabeLoader.asi` next to `DisneyInfinity3.exe` (without CrabeLoader's bootstrap
`dinput8.dll`) and that loader starts it. CrabeInstaller refuses to replace a `dinput8.dll` that is not
CrabeLoader's, and its message points to this route.

When another ASI loader starts CrabeLoader:

* The `.asi` files in the game folder, `scripts/` and `plugins/` are loaded by that other loader.
  CrabeLoader lists them as `already_loaded`, does not start them twice, and still hands them the plugin
  API through `CrabePluginInit`.
* CrabeLoader itself loads only the `.asi` files inside mod folders, from its own thread, a little later
  than the bootstrap would.
* When CrabeLoader starts depends on the other loader, not on CrabeLoader.
* `loader.log` and the `Crabe/` folder are still written next to `DisneyInfinity3.exe`, even when
  `CrabeLoader.asi` is loaded from `scripts/` or `plugins/`.
