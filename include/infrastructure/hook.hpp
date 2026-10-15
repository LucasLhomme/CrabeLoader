/*
** CrabeLoader
** File description:
** Declares one inline detour scoped to the object holding it, installed and removed by RAII.
** No longer a MinHook wrapper: every install goes through the registry as the core owner.
** Patches nothing itself; the memory work is infrastructure/minhook_backend.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef HOOK_HPP_
#define HOOK_HPP_

#include <cstdint>
#include <windows.h>

#include "infrastructure/hook_registry.hpp"

namespace crabe::infrastructure {

class Hook {
    public:
        Hook() = default;

        // Destructor that removes the installed hook via RAII.
        ~Hook();

        Hook(const Hook&) = delete;
        Hook& operator=(const Hook&) = delete;

        // Redirects source to destination and stores the original trampoline.
        // Registers the hook under an optional descriptive name.
        bool install(void* src, void* dst, const char* name = nullptr);

        // Installs a detour at a resolved address and logs the operation result.
        // Returns false and skips patching if the resolved address is zero.
        bool installLogged(uintptr_t addr, void* detour, const char* owner, const char* name);

        // Removes the installed detour hook and releases its handle.
        void remove();

        // Checks whether the detour hook is currently installed.
        bool isInstalled() const;

        // Returns the trampoline pointer to call the original function.
        void* getOriginal() const;

    private:
        HookHandle _handle;
        void* _trampoline = nullptr;
};

} // namespace crabe::infrastructure

#endif /* !HOOK_HPP_ */
