/*
** CrabeLoader
** File description:
** Adapts the old RAII Hook onto the ownership registry, keeping its install and remove shape.
** A call site with no name yet falls back to the address, which still attributes a fault correctly.
** Patches nothing; the memory work is src/infrastructure/minhook_backend.cpp.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/hook.hpp"

#include <format>

#include "shared/logger.hpp"

namespace crabe::infrastructure {

namespace {
    // The registry wants a name for every hook, and a few call sites have none
    // to give yet. The address is the honest fallback: it still puts the hook in
    // the inventory and still attributes a fault to the right owner, which is
    // more than the previous behaviour managed.
    std::string fallbackName(uintptr_t target)
    {
        return std::format("hook@0x{:X}", target);
    }
}

Hook::~Hook()
{
    remove();
}

bool Hook::install(void* src, void* dst, const char* name)
{
    if (isInstalled())
        return false;

    const uintptr_t target = reinterpret_cast<uintptr_t>(src);
    const std::string hookName = name != nullptr ? std::string(name) : fallbackName(target);

    // The registry does the refusing now -- a null target, a null detour and a
    // collision all come back as a named HookError, already logged with both
    // owners' names where that applies.
    auto installed = coreRegistry().install(kCoreOwner, hookName, target, dst, &_trampoline);
    if (!installed) {
        _trampoline = nullptr;
        return false;
    }

    _handle = *installed;
    return true;
}

void Hook::remove()
{
    if (!_handle.valid())
        return;

    coreRegistry().remove(_handle);
    _handle = HookHandle{};
    _trampoline = nullptr;
}

bool Hook::installLogged(uintptr_t addr, void* detour, const char* owner, const char* name)
{
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

    if (addr == 0) {
        logger.warning("{}: {} skipped (address not resolved).", owner, name);
        return false;
    }

    // "<component>::<function>", so the registry's inventory and a crash report
    // read the same way the log line always has.
    const std::string registryName = std::format("{}::{}", owner, name);
    if (!install(reinterpret_cast<void*>(addr), detour, registryName.c_str())) {
        logger.error("{}: failed to hook {} at 0x{:X}.", owner, name, addr);
        return false;
    }
    logger.debug("{}: {} hooked.", owner, name);
    return true;
}

bool Hook::isInstalled() const
{
    return _handle.valid();
}

void* Hook::getOriginal() const
{
    return _trampoline;
}

} // namespace crabe::infrastructure
