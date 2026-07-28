/*
** CrabeLoader
** File description:
** hook
*/

#include "loader/hook.hpp"
#include "loader/exception.hpp"

Hook::Hook()
{
}

Hook::~Hook()
{
    remove();
}

void Hook::clearMemory(void* target, size_t size)
{
    VirtualFree(target, 0, MEM_RELEASE);
    _src = nullptr;
    _dst = nullptr;
    _trampoline = nullptr;
    _len = 0;
    _installed = false;
}

void Hook::restoreMemoryProtection(void* target, size_t size, DWORD oldProtect)
{
    if (!VirtualProtect(target, size, oldProtect, &oldProtect)) {
        memcpy(target, _trampoline, size);
        VirtualProtect(target, size, oldProtect, &oldProtect);
    }
}

bool Hook::IsInstalled()
{
    return _installed;
}

void* Hook::getOriginal() const
{
    return _trampoline;
}

bool Hook::changeMemoryProtection(void* target, size_t size, DWORD newProtect, DWORD& oldProtect)
{
    return VirtualProtect(target, size, newProtect, &oldProtect) != 0;
}

void Hook::writeJump(void* from, void* to)
{
    auto* srcBytes = static_cast<uint8_t*>(from);
    auto relAddr = static_cast<uintptr_t>(
        static_cast<uint8_t*>(to) - srcBytes - 5
    );

    srcBytes[0] = 0xE9; // JMP

    *reinterpret_cast<uint32_t*>(srcBytes + 1) = static_cast<uint32_t>(relAddr);
}

void Hook::padWithNops(void* target, size_t offset, size_t totalLen)
{
    if (totalLen > offset) {
        auto* bytes = static_cast<uint8_t*>(target);
        std::memset(bytes + offset, 0x90, totalLen - offset); // 0x90 = NOP
    }
}

void* Hook::allocateTrampoline()
{
    auto* trampoline = static_cast<uint8_t*>(
        VirtualAlloc(nullptr, _len + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)
    );
    if (!trampoline) return nullptr;

    std::memcpy(trampoline, _src, _len);

    writeJump(trampoline + _len, static_cast<uint8_t*>(_src) + _len);

    return trampoline;
}

bool Hook::install(void* src, void* dst, size_t len)
{
    if (_installed || len < 5) return false;

    _src = src;
    _dst = dst;
    _len = len;

    DWORD oldProtect;
    if (!changeMemoryProtection(_src, _len, PAGE_EXECUTE_READWRITE, oldProtect)) {
        return false;
    }

    try {
        _trampoline = allocateTrampoline();
        if (!_trampoline)
            throw HookException(Severity::ERR, "Failed to allocate trampoline: VirtualAlloc returned null");
    } catch (const HookException& e) {
        e.log();
        DWORD dummyProtect;
        changeMemoryProtection(_src, _len, oldProtect, dummyProtect);
        return false;
    }

    writeJump(_src, _dst);
    padWithNops(_src, 5, _len);

    DWORD dummyProtect;
    changeMemoryProtection(_src, _len, oldProtect, dummyProtect);

    _installed = true;
    return true;
}

void Hook::remove()
{
    if (!_installed)
        return;
    // Restore original memory protection
    DWORD oldProtect;
    changeMemoryProtection(_src, _len, PAGE_EXECUTE_READWRITE, oldProtect);
    // Clear trampoline memory
    clearMemory(_trampoline, _len + 5);
    // Restore original memory protection
    restoreMemoryProtection(_src, _len, oldProtect);
    // Clear all member variables
    clearMemory(_src, _len);
}