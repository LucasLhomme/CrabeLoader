/*
** CrabeLoader
** File description:
** hook
*/

#include "loader/hook.hpp"
#include "loader/memory.hpp"

#include <cstdint>
#include <cstring>

namespace {
    // jmp rel32
    constexpr size_t kJumpSize = 5;
}

Hook::~Hook()
{
    remove();
}

void Hook::writeJump(void* from, void* to)
{
    auto* bytes = static_cast<uint8_t*>(from);
    auto rel = static_cast<uint32_t>(
        static_cast<uint8_t*>(to) - bytes - kJumpSize
    );

    bytes[0] = 0xE9;
    std::memcpy(bytes + 1, &rel, sizeof(rel));
}

bool Hook::install(void* src, void* dst)
{
    if (_installed || !src || !dst) return false;

    size_t len = Memory::PrologueLength(reinterpret_cast<uintptr_t>(src), kJumpSize);
    if (len == 0) return false;   // prologue not relocatable: refuse

    // Trampoline: the stolen bytes, then a jump back to the rest of the function.
    auto* trampoline = static_cast<uint8_t*>(
        VirtualAlloc(nullptr, len + kJumpSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)
    );
    if (!trampoline) return false;

    std::memcpy(trampoline, src, len);
    writeJump(trampoline + len, static_cast<uint8_t*>(src) + len);

    DWORD oldProtect = 0;
    if (!VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return false;
    }

    writeJump(src, dst);
    // Leftover bytes of the last stolen instruction, so a disassembler (and any
    // code branching just past the hook) still sees valid instructions.
    std::memset(static_cast<uint8_t*>(src) + kJumpSize, 0x90, len - kJumpSize);

    DWORD ignored = 0;
    VirtualProtect(src, len, oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), src, len);

    _src = src;
    _dst = dst;
    _trampoline = trampoline;
    _len = len;
    _installed = true;
    return true;
}

void Hook::remove()
{
    if (!_installed) return;

    // The trampoline holds the original bytes: copy them back before freeing it.
    DWORD oldProtect = 0;
    if (VirtualProtect(_src, _len, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        std::memcpy(_src, _trampoline, _len);

        DWORD ignored = 0;
        VirtualProtect(_src, _len, oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), _src, _len);
    }

    VirtualFree(_trampoline, 0, MEM_RELEASE);

    _src = nullptr;
    _dst = nullptr;
    _trampoline = nullptr;
    _len = 0;
    _installed = false;
}

bool Hook::isInstalled() const
{
    return _installed;
}

void* Hook::getOriginal() const
{
    return _trampoline;
}
