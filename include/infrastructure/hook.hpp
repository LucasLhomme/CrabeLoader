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

// A single inline detour hook, scoped to the object that holds it.
//
// No longer a MinHook wrapper: every install() goes through coreRegistry()
// under the "core" owner, so the loader's own hooks are in the same inventory a
// mod's are -- they collide with each other rather than corrupting each other,
// they are named in a crash report, and they can be removed by owner. The
// patching itself is MinHookBackend's job; see infrastructure/hook_registry.hpp
// for why the two are separate.
//
// The RAII shape is unchanged, and so is every call site: remove() in a
// destructor still takes exactly this hook out, by handle.
class Hook {
    public:
        Hook() = default;
        ~Hook();

        Hook(const Hook&) = delete;
        Hook& operator=(const Hook&) = delete;

        // Redirects `src` to `dst`; getOriginal() then returns the trampoline
        // for calling through to the real function.
        //
        // `name` is what the registry, the log and a crash report will call
        // this hook. It is optional only because a handful of call sites
        // predate the registry; pass one. Without it the hook is recorded as
        // "hook@0x<address>", which attributes a fault to the right bytes but
        // tells the reader nothing about what they were.
        bool install(void* src, void* dst, const char* name = nullptr);

        // install() at a resolved address, with the outcome logged as
        // "<owner>: <name> ...". An address of 0 means "not found" and is
        // skipped rather than patched: a wrong address overwrites live code
        // and crashes the host process on the next execution.
        //
        // `owner` here is the loader component doing the hooking ("RenderHook"),
        // not a registry owner -- the registry owner is always "core" for these.
        // It becomes the first half of the registry name, "<owner>::<name>".
        bool installLogged(uintptr_t addr, void* detour, const char* owner, const char* name);

        void remove();

        bool isInstalled() const;
        void* getOriginal() const;

    private:
        HookHandle _handle;
        void* _trampoline = nullptr;
};

} // namespace crabe::infrastructure

#endif /* !HOOK_HPP_ */
