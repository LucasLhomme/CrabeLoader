# Native `.asi` Plugins

CrabeLoader loads `.asi` plugins: native 32-bit DLLs with a `.asi` extension. It loads them the same way
Ultimate ASI Loader does, so most existing plugins work unchanged. Lua mods remain the way to build gameplay
features. An `.asi` plugin is for the rare job that needs native code of its own. Through the optional
[plugin API](#talking-to-crabeloader-the-plugin-api), such a plugin can log, hook and talk to Lua mods.

## Where to put them

CrabeLoader searches four places, in this order, next to `DisneyInfinity3.exe`:

| Folder | Source shown in the log and in Lua |
| :--- | :--- |
| the game folder itself | `game folder` |
| `scripts/` | `scripts` |
| `plugins/` | `plugins` |
| `mods/<mod>/` (the mod's own folder, not its subfolders) | `mod <id>` |

Inside one folder, files load in alphabetical order, ignoring case. Subfolders are not searched, and
mod folders starting with `.` or `_` are skipped, exactly as for Lua mods.

A plugin inside a mod folder belongs to that mod. When the active profile in `crabe.toml` disables the
mod, its `.asi` files are skipped too. The VFS never serves an `.asi` file as a game asset.

## When they load

Plugins load **before any of the game's code runs**. That is just before the executable's entry point,
after every DLL the game imports has started. This is the moment plugins are written for: they can patch
the game before it reads its settings or opens its window.

If CrabeLoader is loaded later than usual (injected into a running game, for instance), the entry point has
already passed. Plugins then load from CrabeLoader's own thread as soon as it starts, and the log says so.

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
| `skipped` | Turned off by `[plugins]` or by a disabled mod. |
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

## Other ASI loaders

CrabeLoader installs as `dinput8.dll`, the name Ultimate ASI Loader also commonly uses. Only one of them
can have that name, and CrabeInstaller refuses to replace a `dinput8.dll` it did not write. CrabeLoader
loads `.asi` plugins itself, so a separate ASI loader is not needed. If one is installed under another
name anyway, a plugin it has already loaded shows as `already_loaded` and is not started twice.
