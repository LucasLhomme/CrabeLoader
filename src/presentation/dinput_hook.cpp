/*
** CrabeLoader
** File description:
** Hooks shared DirectInput device code to apply the input remap table to game inputs.
** Addresses come from a throwaway keyboard device, avoiding timing issues with game setup.
** Unbuffered reads are told apart by buffer size, buffered reads by asking the device type.
**
** Authors: @LucasLhomme
*/

#define DIRECTINPUT_VERSION 0x0800

#include <cstring>
#include <format>
#include <string>
#include <windows.h>
#include <dinput.h>

#include "presentation/dinput_hook.hpp"
#include "presentation/input_remap.hpp"
#include "shared/logger.hpp"

namespace crabe::presentation {

namespace {

    // Defined here instead of linking dxguid.lib: these are the published values, and a
    // wrong one makes initialize() fail loudly instead of hooking the wrong thing.
    const GUID kSysKeyboard = { 0x6F1D2B61, 0xD5A0, 0x11CF, { 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };
    const GUID kSysMouse = { 0x6F1D2B60, 0xD5A0, 0x11CF, { 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };
    const GUID kIidDirectInput8A = { 0xBF798030, 0x483A, 0x4DA2, { 0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00 } };

    // IDirectInputDevice8 vtable slots: IUnknown 0-2, GetCapabilities 3 ... GetDeviceState 9, GetDeviceData 10.
    constexpr size_t kSlotGetDeviceState = 9;
    constexpr size_t kSlotGetDeviceData = 10;

    constexpr unsigned long kKeyboardStateSize = 256;
    constexpr size_t kMouseButtonsOffset = 3 * sizeof(LONG);  // DIMOUSESTATE and DIMOUSESTATE2 both: lX, lY, lZ first.
    constexpr DWORD kMouseButtonOffsetBase = 12;              // DIMOFS_BUTTON0

    typedef HRESULT(WINAPI* t_DirectInput8Create)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);

    // Index of the interface method at `slot` in `object`'s vtable.
    void* vtableSlot(void* object, size_t slot)
    {
        return (*reinterpret_cast<void***>(object))[slot];
    }

    // Applies the remap to a buffered keyboard read: scan codes are rewritten, swallowed events removed.
    unsigned long remapKeyboardEvents(unsigned char* events, unsigned long objectSize, unsigned long count)
    {
        const InputRemap& remap = InputRemap::get();
        unsigned long kept = 0;
        for (unsigned long i = 0; i < count; ++i) {
            unsigned char* source = events + static_cast<size_t>(i) * objectSize;
            int target = remap.keyTarget(*reinterpret_cast<DWORD*>(source));
            if (target == InputRemap::kDropped) continue;

            unsigned char* slot = events + static_cast<size_t>(kept) * objectSize;
            if (slot != source) std::memmove(slot, source, objectSize);
            *reinterpret_cast<DWORD*>(slot) = static_cast<DWORD>(target);
            ++kept;
        }
        return kept;
    }

    // Same for the mouse; only the button offsets move, axes and wheel pass through.
    unsigned long remapMouseEvents(unsigned char* events, unsigned long objectSize, unsigned long count)
    {
        const InputRemap& remap = InputRemap::get();
        unsigned long kept = 0;
        for (unsigned long i = 0; i < count; ++i) {
            unsigned char* source = events + static_cast<size_t>(i) * objectSize;
            DWORD offset = *reinterpret_cast<DWORD*>(source);
            bool isButton = offset >= kMouseButtonOffsetBase && offset < kMouseButtonOffsetBase + InputRemap::kMouseButtons;

            int target = isButton ? remap.mouseTarget(offset - kMouseButtonOffsetBase) : 0;
            if (isButton && target == InputRemap::kDropped) continue;

            unsigned char* slot = events + static_cast<size_t>(kept) * objectSize;
            if (slot != source) std::memmove(slot, source, objectSize);
            if (isButton) *reinterpret_cast<DWORD*>(slot) = kMouseButtonOffsetBase + static_cast<DWORD>(target);
            ++kept;
        }
        return kept;
    }

}

DirectInputHook& DirectInputHook::get()
{
    static DirectInputHook instance;
    return instance;
}

bool DirectInputHook::initialize()
{
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

    HMODULE module = GetModuleHandleW(L"DINPUT8.dll");
    if (!module) module = LoadLibraryW(L"DINPUT8.dll");
    auto create = module ? reinterpret_cast<t_DirectInput8Create>(GetProcAddress(module, "DirectInput8Create")) : nullptr;
    if (!create) {
        logger.warning("DirectInputHook: DINPUT8.dll or DirectInput8Create is unavailable; keyboard and mouse remap is off.");
        return false;
    }

    IDirectInput8A* input = nullptr;
    if (FAILED(create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, kIidDirectInput8A,
                      reinterpret_cast<LPVOID*>(&input), nullptr)) || !input) {
        logger.warning("DirectInputHook: DirectInput8Create failed; keyboard and mouse remap is off.");
        return false;
    }

    // A device that is created and never acquired disturbs nothing; it only lends its vtable.
    IDirectInputDevice8A* keyboard = nullptr;
    IDirectInputDevice8A* mouse = nullptr;
    input->CreateDevice(kSysKeyboard, &keyboard, nullptr);
    input->CreateDevice(kSysMouse, &mouse, nullptr);

    void* stateCode = keyboard ? vtableSlot(keyboard, kSlotGetDeviceState) : nullptr;
    void* dataCode = keyboard ? vtableSlot(keyboard, kSlotGetDeviceData) : nullptr;
    void* mouseStateCode = mouse ? vtableSlot(mouse, kSlotGetDeviceState) : nullptr;
    void* mouseDataCode = mouse ? vtableSlot(mouse, kSlotGetDeviceData) : nullptr;

    if (mouse) mouse->Release();
    if (keyboard) keyboard->Release();
    input->Release();

    if (!stateCode || !dataCode) {
        logger.warning("DirectInputHook: could not create a keyboard device to read its code from; remap is off.");
        return false;
    }

    bool stateOk = _hookState.install(stateCode, reinterpret_cast<void*>(&DirectInputHook::hkGetDeviceState),
                                      "DirectInputHook::GetDeviceState");
    bool dataOk = _hookData.install(dataCode, reinterpret_cast<void*>(&DirectInputHook::hkGetDeviceData),
                                    "DirectInputHook::GetDeviceData");
    if (!stateOk || !dataOk) {
        logger.error("DirectInputHook: failed to hook GetDeviceState/GetDeviceData (state {}, data {}).", stateOk, dataOk);
        uninitialize();
        return false;
    }
    logger.info("DirectInputHook: watching GetDeviceState @ 0x{:X}, GetDeviceData @ 0x{:X}.",
                reinterpret_cast<uintptr_t>(stateCode), reinterpret_cast<uintptr_t>(dataCode));

    // Each function the mouse does not share with the keyboard needs its own detour, because a
    // detour reaches the real function through its own trampoline.
    if (mouseStateCode && mouseStateCode != stateCode) {
        if (_hookMouseState.install(mouseStateCode, reinterpret_cast<void*>(&DirectInputHook::hkMouseGetDeviceState),
                                    "DirectInputHook::MouseGetDeviceState")) {
            logger.info("DirectInputHook: the mouse has its own GetDeviceState @ 0x{:X}.",
                        reinterpret_cast<uintptr_t>(mouseStateCode));
        } else {
            logger.warning("DirectInputHook: the mouse's own GetDeviceState could not be hooked; mouse remap may not apply.");
        }
    }
    if (mouseDataCode && mouseDataCode != dataCode) {
        if (_hookMouseData.install(mouseDataCode, reinterpret_cast<void*>(&DirectInputHook::hkMouseGetDeviceData),
                                   "DirectInputHook::MouseGetDeviceData")) {
            logger.info("DirectInputHook: the mouse has its own GetDeviceData @ 0x{:X}.",
                        reinterpret_cast<uintptr_t>(mouseDataCode));
        } else {
            logger.warning("DirectInputHook: the mouse's own GetDeviceData could not be hooked; mouse remap may not apply.");
        }
    }
    return true;
}

void DirectInputHook::uninitialize()
{
    _hookState.remove();
    _hookData.remove();
    _hookMouseState.remove();
    _hookMouseData.remove();
}

long DirectInputHook::afterState(unsigned long size, void* data, long result)
{
    if (FAILED(result) || !data) return result;

    InputRemap& remap = InputRemap::get();
    if (size == kKeyboardStateSize) {
        _keyboardStates.fetch_add(1, std::memory_order_relaxed);
        remap.remapKeyboardState(static_cast<uint8_t*>(data), size);
    } else if (size == sizeof(DIMOUSESTATE) || size == sizeof(DIMOUSESTATE2)) {
        _mouseStates.fetch_add(1, std::memory_order_relaxed);

        // lZ, the wheel, comes right after lX and lY in both layouts; read it before the remap
        // touches the buttons, and whether or not a remap is active.
        LONG wheel = 0;
        std::memcpy(&wheel, static_cast<const uint8_t*>(data) + 2 * sizeof(LONG), sizeof(wheel));
        _wheel.add(static_cast<int32_t>(wheel));
        size_t buttons = (size - kMouseButtonsOffset);
        remap.remapMouseButtons(static_cast<uint8_t*>(data) + kMouseButtonsOffset, buttons);
    } else {
        _otherStates.fetch_add(1, std::memory_order_relaxed);
    }
    return result;
}

long DirectInputHook::afterData(IDirectInputDevice8A* self, unsigned long objectSize, void* events,
                                unsigned long* count, long result)
{
    if (FAILED(result) || !events || !count || *count == 0 || objectSize < sizeof(DWORD)) return result;

    // The device kind is asked for on every call instead of cached by address: a recreated
    // device can land on a freed one's address, and a stale kind would remap the wrong events.
    DIDEVCAPS caps = {};
    caps.dwSize = sizeof(caps);
    if (FAILED(self->GetCapabilities(&caps))) return result;

    auto* bytes = static_cast<unsigned char*>(events);
    DWORD type = GET_DIDEVICE_TYPE(caps.dwDevType);
    if (type == DI8DEVTYPE_KEYBOARD) {
        _keyboardEvents.fetch_add(1, std::memory_order_relaxed);
        if (InputRemap::get().isActive()) *count = remapKeyboardEvents(bytes, objectSize, *count);
    } else if (type == DI8DEVTYPE_MOUSE) {
        _mouseEvents.fetch_add(1, std::memory_order_relaxed);
        if (InputRemap::get().isActive()) *count = remapMouseEvents(bytes, objectSize, *count);
    }
    return result;
}

long __stdcall DirectInputHook::hkGetDeviceState(IDirectInputDevice8A* self, unsigned long size, void* data)
{
    DirectInputHook& hook = get();
    auto original = reinterpret_cast<t_GetDeviceState>(hook._hookState.getOriginal());
    if (!original) return E_FAIL;
    return hook.afterState(size, data, original(self, size, data));
}

long __stdcall DirectInputHook::hkGetDeviceData(IDirectInputDevice8A* self, unsigned long objectSize, void* events,
                                                unsigned long* count, unsigned long flags)
{
    DirectInputHook& hook = get();
    auto original = reinterpret_cast<t_GetDeviceData>(hook._hookData.getOriginal());
    if (!original) return E_FAIL;
    return hook.afterData(self, objectSize, events, count, original(self, objectSize, events, count, flags));
}

long __stdcall DirectInputHook::hkMouseGetDeviceState(IDirectInputDevice8A* self, unsigned long size, void* data)
{
    DirectInputHook& hook = get();
    auto original = reinterpret_cast<t_GetDeviceState>(hook._hookMouseState.getOriginal());
    if (!original) return E_FAIL;
    return hook.afterState(size, data, original(self, size, data));
}

long __stdcall DirectInputHook::hkMouseGetDeviceData(IDirectInputDevice8A* self, unsigned long objectSize, void* events,
                                                     unsigned long* count, unsigned long flags)
{
    DirectInputHook& hook = get();
    auto original = reinterpret_cast<t_GetDeviceData>(hook._hookMouseData.getOriginal());
    if (!original) return E_FAIL;
    return hook.afterData(self, objectSize, events, count, original(self, objectSize, events, count, flags));
}

int DirectInputHook::takeWheelNotches()
{
    return _wheel.takeNotches();
}

std::string DirectInputHook::report() const
{
    return std::format("dinput kbdState:{} kbdData:{} mouseState:{} mouseData:{} otherState:{} wheel:{}",
                       _keyboardStates.load(std::memory_order_relaxed), _keyboardEvents.load(std::memory_order_relaxed),
                       _mouseStates.load(std::memory_order_relaxed), _mouseEvents.load(std::memory_order_relaxed),
                       _otherStates.load(std::memory_order_relaxed), _wheel.readings());
}

} // namespace crabe::presentation
