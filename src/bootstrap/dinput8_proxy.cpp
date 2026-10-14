/*
** CrabeLoader
** File description:
** Exports the six functions of dinput8.dll, so the game loads the bootstrap under that name.
** Each call goes to the system's own dinput8.dll, loaded the first time one of them is called.
** Never loads anything from DllMain: the loader lock forbids LoadLibrary there.
**
** Authors: @LucasLhomme
*/

#include <windows.h>
#include <unknwn.h>

#include <array>
#include <mutex>
#include <string>

#pragma comment(linker, "/export:DirectInput8Create=_CrabeDirectInput8Create@20")
#pragma comment(linker, "/export:DllCanUnloadNow=_CrabeDllCanUnloadNow@0,PRIVATE")
#pragma comment(linker, "/export:DllGetClassObject=_CrabeDllGetClassObject@12,PRIVATE")
#pragma comment(linker, "/export:DllRegisterServer=_CrabeDllRegisterServer@0,PRIVATE")
#pragma comment(linker, "/export:DllUnregisterServer=_CrabeDllUnregisterServer@0,PRIVATE")
#pragma comment(linker, "/export:GetdfDIJoystick=_CrabeGetdfDIJoystick@0")

namespace {

    using DirectInput8CreateFn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
    using DllCanUnloadNowFn = HRESULT(WINAPI*)();
    using DllGetClassObjectFn = HRESULT(WINAPI*)(REFCLSID, REFIID, LPVOID*);
    using DllRegisterServerFn = HRESULT(WINAPI*)();
    using GetdfDIJoystickFn = const void*(WINAPI*)();

    struct SystemDinput8 {
        HMODULE module{nullptr};
        DirectInput8CreateFn create{nullptr};
        DllCanUnloadNowFn canUnloadNow{nullptr};
        DllGetClassObjectFn getClassObject{nullptr};
        DllRegisterServerFn registerServer{nullptr};
        DllRegisterServerFn unregisterServer{nullptr};
        GetdfDIJoystickFn joystickFormat{nullptr};
    };

    /// Loads dinput8.dll from the system folder once, whatever thread asks first, and resolves its exports.
    /// The system folder is named explicitly, because a bare name would find this very DLL in the game folder.
    /// A 32-bit process asking for System32 is redirected to SysWOW64 by Windows itself.
    const SystemDinput8& systemDinput8()
    {
        static SystemDinput8 system;
        static std::once_flag once;
        std::call_once(once, [] {
            std::array<wchar_t, MAX_PATH> folder{};
            const UINT length = GetSystemDirectoryW(folder.data(), static_cast<UINT>(folder.size()));
            if (length == 0 || length >= folder.size())
                return;
            const std::wstring path = std::wstring(folder.data(), length) + L"\\dinput8.dll";
            system.module = LoadLibraryW(path.c_str());
            if (!system.module)
                return;
            system.create = reinterpret_cast<DirectInput8CreateFn>(GetProcAddress(system.module, "DirectInput8Create"));
            system.canUnloadNow = reinterpret_cast<DllCanUnloadNowFn>(GetProcAddress(system.module, "DllCanUnloadNow"));
            system.getClassObject = reinterpret_cast<DllGetClassObjectFn>(GetProcAddress(system.module, "DllGetClassObject"));
            system.registerServer = reinterpret_cast<DllRegisterServerFn>(GetProcAddress(system.module, "DllRegisterServer"));
            system.unregisterServer = reinterpret_cast<DllRegisterServerFn>(GetProcAddress(system.module, "DllUnregisterServer"));
            system.joystickFormat = reinterpret_cast<GetdfDIJoystickFn>(GetProcAddress(system.module, "GetdfDIJoystick"));
        });
        return system;
    }

} // namespace

/// DirectInput8Create of the system's dinput8.dll; E_FAIL when it could not be loaded.
extern "C" HRESULT WINAPI CrabeDirectInput8Create(HINSTANCE instance, DWORD version, REFIID iid, LPVOID* out,
                                                  LPUNKNOWN outer)
{
    const auto create = systemDinput8().create;
    if (!create) {
        if (out)
            *out = nullptr;
        return E_FAIL;
    }
    return create(instance, version, iid, out, outer);
}

/// DllCanUnloadNow of the system's dinput8.dll; S_FALSE keeps this DLL loaded when it is missing.
extern "C" HRESULT WINAPI CrabeDllCanUnloadNow()
{
    const auto canUnloadNow = systemDinput8().canUnloadNow;
    return canUnloadNow ? canUnloadNow() : S_FALSE;
}

/// DllGetClassObject of the system's dinput8.dll, for COM clients that create DirectInput by class id.
extern "C" HRESULT WINAPI CrabeDllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID* out)
{
    const auto getClassObject = systemDinput8().getClassObject;
    if (!getClassObject) {
        if (out)
            *out = nullptr;
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    return getClassObject(clsid, iid, out);
}

/// DllRegisterServer of the system's dinput8.dll.
extern "C" HRESULT WINAPI CrabeDllRegisterServer()
{
    const auto registerServer = systemDinput8().registerServer;
    return registerServer ? registerServer() : E_FAIL;
}

/// DllUnregisterServer of the system's dinput8.dll.
extern "C" HRESULT WINAPI CrabeDllUnregisterServer()
{
    const auto unregisterServer = systemDinput8().unregisterServer;
    return unregisterServer ? unregisterServer() : E_FAIL;
}

/// GetdfDIJoystick of the system's dinput8.dll: the joystick data format, or null when it could not be loaded.
extern "C" const void* WINAPI CrabeGetdfDIJoystick()
{
    const auto joystickFormat = systemDinput8().joystickFormat;
    return joystickFormat ? joystickFormat() : nullptr;
}
