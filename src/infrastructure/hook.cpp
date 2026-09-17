/*
** CrabeLoader
** File description:
** hook
*/

#include "infrastructure/hook.hpp"

#include <cstdint>
#include <mutex>
#include "minhook/MinHook.h"
#include "shared/logger.hpp"

namespace {
    int g_refCount = 0;
    std::mutex g_refMutex;

    // MH_Initialize/MH_Uninitialize are process-global; Hook instances are
    // created/destroyed independently, so reference-count them.
    bool acquireMinHook()
    {
        std::lock_guard<std::mutex> lock(g_refMutex);
        if (g_refCount == 0) {
            MH_STATUS status = MH_Initialize();
            if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
                Logger::getInstance().error("Hook: MH_Initialize failed: {}", MH_StatusToString(status));
                return false;
            }
        }
        ++g_refCount;
        return true;
    }

    void releaseMinHook()
    {
        std::lock_guard<std::mutex> lock(g_refMutex);
        if (--g_refCount == 0) {
            MH_Uninitialize();
        }
    }
}

Hook::~Hook()
{
    remove();
}

// Redirects `src` to `dst` via MinHook; getOriginal() then returns the
// trampoline for calling through to the real function.
bool Hook::install(void* src, void* dst)
{
    if (_installed || !src || !dst) return false;
    if (!acquireMinHook()) return false;

    Logger& logger = Logger::getInstance();

    void* original = nullptr;
    MH_STATUS status = MH_CreateHook(src, dst, &original);
    if (status != MH_OK) {
        logger.error("Hook: MH_CreateHook failed at 0x{:X}: {}",
                    reinterpret_cast<uintptr_t>(src), MH_StatusToString(status));
        releaseMinHook();
        return false;
    }

    status = MH_EnableHook(src);
    if (status != MH_OK) {
        logger.error("Hook: MH_EnableHook failed at 0x{:X}: {}",
                    reinterpret_cast<uintptr_t>(src), MH_StatusToString(status));
        MH_RemoveHook(src);
        releaseMinHook();
        return false;
    }

    _src = src;
    _trampoline = original;
    _installed = true;
    return true;
}

void Hook::remove()
{
    if (!_installed) return;

    MH_DisableHook(_src);
    MH_RemoveHook(_src);
    releaseMinHook();

    _src = nullptr;
    _trampoline = nullptr;
    _installed = false;
}

bool Hook::installLogged(uintptr_t addr, void* detour, const char* owner, const char* name)
{
    Logger& logger = Logger::getInstance();

    if (addr == 0) {
        logger.warning("{}: {} skipped (address not resolved).", owner, name);
        return false;
    }
    if (!install(reinterpret_cast<void*>(addr), detour)) {
        logger.error("{}: failed to hook {} at 0x{:X}.", owner, name, addr);
        return false;
    }
    logger.debug("{}: {} hooked.", owner, name);
    return true;
}

bool Hook::isInstalled() const
{
    return _installed;
}

void* Hook::getOriginal() const
{
    return _trampoline;
}
