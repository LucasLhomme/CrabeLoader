/*
** CrabeLoader
** File description:
** A stand-in .asi plugin for test_asi_plugins: a real 32-bit DLL whose start the test can observe.
** Built three times: plain (also exercising the plugin API), refusing in DllMain, crashing in CrabePluginInit.
** Does nothing else, so loading it into the test process is harmless.
**
** Authors: @LucasLhomme
*/

#include <windows.h>

#include <cstdio>
#include <cstring>

#include "sdk/crabe_plugin.h"

namespace {

    /// A function for the probe to hand to installHook; never called.
    int __cdecl probeTarget(int value)
    {
        return value + 1;
    }

    /// The detour the probe hands to installHook; never called.
    int __cdecl probeDetour(int value)
    {
        return value + 2;
    }

    const CrabePluginApi* probeApi = nullptr;
    int presentCount = 0;

    /// Counts Present calls and records the arguments of the last one in the shared values.
    void __cdecl probePresent(void* swapChain, uint32_t flags, void* user)
    {
        char text[64] = {};
        std::snprintf(text, sizeof(text), "%d %p %u %s", ++presentCount, swapChain, flags,
                      user == &presentCount ? "user" : "nouser");
        probeApi->setValue(probeApi, "probe.present", text);
    }

    /// Faults on purpose, so the test sees the loader turn this plugin's render callbacks off.
    void __cdecl probeResize(void* swapChain, void* user)
    {
        (void)swapChain;
        (void)user;
        *static_cast<volatile int*>(nullptr) = 3;
    }

} // namespace

/// Records in the process environment that the plugin started, or refuses to start in the refusing build.
BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
    (void)module;
    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH)
        return TRUE;
#if defined(CRABE_ASI_PROBE_REFUSES)
    return FALSE;
#else
    SetEnvironmentVariableW(L"CRABE_ASI_PROBE", L"started");
    return TRUE;
#endif
}

/// Stands in for CrabeLoader.asi's bootstrap export in test_bootstrap; the crashing build faults here too.
extern "C" __declspec(dllexport) void __cdecl CrabeLoaderBootstrap()
{
#if defined(CRABE_ASI_PROBE_INIT_CRASHES)
    *static_cast<volatile int*>(nullptr) = 2;
#else
    SetEnvironmentVariableW(L"CRABE_BOOTSTRAP_CALLED", L"yes");
#endif
}

/// Calls every function of the plugin API once, leaving traces the test reads back; the crashing build faults.
extern "C" __declspec(dllexport) void __cdecl CrabePluginInit(const CrabePluginApi* api)
{
#if defined(CRABE_ASI_PROBE_INIT_CRASHES)
    (void)api;
    *static_cast<volatile int*>(nullptr) = 1;
#else
    if (!api || api->size < sizeof(CrabePluginApi) || api->version != CRABE_PLUGIN_API_VERSION)
        return;
    api->log(api, CRABE_LOG_INFO, "probe started {not a format}");
    api->setValue(api, "probe.name", api->pluginName);
    char cut[8] = {};
    const int length = api->getValue(api, "probe.name", cut, sizeof(cut));
    const bool readBack = length == static_cast<int>(std::strlen(api->pluginName)) &&
                          std::strncmp(cut, api->pluginName, sizeof(cut) - 1) == 0 && cut[sizeof(cut) - 1] == '\0';
    api->setValue(api, "probe.readBack", readBack ? "ok" : "bad");
    api->setValue(api, "probe.missing", api->getValue(api, "probe.none", cut, sizeof(cut)) == -1 ? "ok" : "bad");
    void* original = nullptr;
    api->installHook(api, reinterpret_cast<void*>(&probeTarget), reinterpret_cast<void*>(&probeDetour), &original,
                     "target");
    api->runLua(api, "Crabe.write('probe')");
    api->setValue(api, "probe.blankLua", api->runLua(api, "  \n") == 0 ? "ok" : "bad");
    api->setValue(api, "probe.loader", api->loaderVersion);
    probeApi = api;
    const int present = api->onPresent(api, &probePresent, &presentCount);
    const int resize = api->onBeforeResize(api, &probeResize, nullptr);
    api->setValue(api, "probe.renderRegistered", present == 1 && resize == 1 ? "ok" : "bad");
#endif
}
