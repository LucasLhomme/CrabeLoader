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
    // Generates a fallback hook name formatted with the target address.
    std::string fallbackName(uintptr_t target)
    {
        return std::format("hook@0x{:X}", target);
    }
}

// Destructor. Removes the hook if currently installed.
Hook::~Hook()
{
    remove();
}

// Installs detour redirection and registers the hook in the core registry.
bool Hook::install(void* src, void* dst, const char* name)
{
    if (isInstalled())
        return false;

    const uintptr_t target = reinterpret_cast<uintptr_t>(src);
    const std::string hookName = name != nullptr ? std::string(name) : fallbackName(target);

    auto installed = coreRegistry().install(kCoreOwner, hookName, target, dst, &_trampoline);
    if (!installed) {
        _trampoline = nullptr;
        return false;
    }

    _handle = *installed;
    return true;
}

// Removes the hook from the registry and resets internal state.
void Hook::remove()
{
    if (!_handle.valid())
        return;

    coreRegistry().remove(_handle);
    _handle = HookHandle{};
    _trampoline = nullptr;
}

// Resolves address and logs installation status with owner attribution.
bool Hook::installLogged(uintptr_t addr, void* detour, const char* owner, const char* name)
{
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

    if (addr == 0) {
        logger.warning("{}: {} skipped (address not resolved).", owner, name);
        return false;
    }

    const std::string registryName = std::format("{}::{}", owner, name);
    if (!install(reinterpret_cast<void*>(addr), detour, registryName.c_str())) {
        logger.error("{}: failed to hook {} at 0x{:X}.", owner, name, addr);
        return false;
    }
    logger.debug("{}: {} hooked.", owner, name);
    return true;
}

// Checks whether the hook handle is valid.
bool Hook::isInstalled() const
{
    return _handle.valid();
}

// Returns the original function trampoline pointer.
void* Hook::getOriginal() const
{
    return _trampoline;
}

} // namespace crabe::infrastructure
