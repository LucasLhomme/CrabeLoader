/*
** CrabeLoader
** File description:
** A stand-in .asi plugin for test_asi_plugins: a real 32-bit DLL whose start the test can observe.
** Built twice; the CRABE_ASI_PROBE_REFUSES build returns FALSE from DllMain instead of starting.
** Does nothing else, so loading it into the test process is harmless.
**
** Authors: @LucasLhomme
*/

#include <windows.h>

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
