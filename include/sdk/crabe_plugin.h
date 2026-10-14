/*
** CrabeLoader
** File description:
** The plugin API a native .asi plugin can use: copy this one C header into the plugin's sources.
** Export CrabePluginInit and CrabeLoader calls it once, right after loading the plugin.
** Plain C with fixed-size types, so any 32-bit compiler can use it; nothing to link against.
**
** Authors: @LucasLhomme
*/

#ifndef CRABE_PLUGIN_H_
#define CRABE_PLUGIN_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CRABE_PLUGIN_API_VERSION 1

enum {
    CRABE_LOG_DEBUG = 0,
    CRABE_LOG_INFO = 1,
    CRABE_LOG_WARNING = 2,
    CRABE_LOG_ERROR = 3
};

typedef struct CrabePluginApi CrabePluginApi;

/* Every function takes the table it came from first, so CrabeLoader knows which plugin is calling.
   Strings are UTF-8 and copied before the call returns. Every function may be called from any thread. */
struct CrabePluginApi {
    /* sizeof(CrabePluginApi) as CrabeLoader built it; a later version only appends fields. */
    uint32_t size;
    /* CRABE_PLUGIN_API_VERSION of the running CrabeLoader. */
    uint32_t version;
    /* The running CrabeLoader version, e.g. "1.1.0". */
    const char* loaderVersion;
    /* The plugin's file name, e.g. "my_plugin.asi". */
    const char* pluginName;

    /* Writes one line to loader.log, prefixed with the plugin's name. */
    void(__cdecl* log)(const CrabePluginApi* self, int level, const char* message);

    /* Detours `target` to `detour` through CrabeLoader's hook registry, which refuses a target another owner
       already hooks and names the plugin in crash reports. Writes the trampoline to *original first.
       Returns 1 on success, 0 on refusal (the reason is in loader.log). */
    int(__cdecl* installHook)(const CrabePluginApi* self, void* target, void* detour, void** original,
                              const char* name);

    /* Removes every hook this plugin installed, newest first. Returns how many were removed. */
    int(__cdecl* removeHooks)(const CrabePluginApi* self);

    /* Queues a Lua chunk for the game's Lua state; it runs on the next loader tick, on the script thread,
       with the Crabe and Game APIs available. An error is logged with the plugin's name.
       Returns 1 once queued, 0 for an empty chunk. */
    int(__cdecl* runLua)(const CrabePluginApi* self, const char* code);

    /* Sets a value shared with Lua mods (Crabe.Plugins.get / Crabe.Plugins.set) and every plugin.
       A NULL value erases the key. Prefix keys with the plugin's name to avoid clashes. Returns 1, or 0 for no key. */
    int(__cdecl* setValue)(const CrabePluginApi* self, const char* key, const char* value);

    /* Copies a shared value into `buffer` (always NUL-terminated, cut short if it does not fit).
       Returns the value's full length in bytes, or -1 when the key is not set. */
    int(__cdecl* getValue)(const CrabePluginApi* self, const char* key, char* buffer, uint32_t bufferSize);
};

/* The function a plugin exports to receive the table; the table stays valid until the game exits.
   It runs on the game's main thread before the game's own code, so keep it short. */
typedef void(__cdecl* CrabePluginInitFn)(const CrabePluginApi* api);

#define CRABE_PLUGIN_INIT_SYMBOL "CrabePluginInit"

#ifdef __cplusplus
}
#endif

#endif /* !CRABE_PLUGIN_H_ */
