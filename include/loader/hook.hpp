/*
** CrabeLoader
** File description:
** hook
*/

#ifndef HOOK_HPP_
#define HOOK_HPP_

#include <windows.h>

// A single inline detour hook, backed by MinHook. install() redirects `src`
// to `dst`; getOriginal() returns the trampoline for calling through to the
// real function. MinHook itself is reference-counted process-wide.
class Hook {
    public:
        Hook() = default;
        ~Hook();

        Hook(const Hook&) = delete;
        Hook& operator=(const Hook&) = delete;

        // Redirects `src` to `dst` via MinHook; getOriginal() then returns the
        // trampoline for calling through to the real function. MinHook itself
        // is reference-counted process-wide.
        bool install(void* src, void* dst);

        // install() at a resolved address, with the outcome logged as
        // "<owner>: <name> ...". An address of 0 means "not found" and is
        // skipped rather than patched: a wrong address overwrites live code
        // and crashes the host process on the next execution.
        bool installLogged(uintptr_t addr, void* detour, const char* owner, const char* name);

        void remove();

        bool isInstalled() const;
        void* getOriginal() const;

    private:
        void* _src = nullptr;
        void* _trampoline = nullptr;
        bool _installed = false;
};

#endif /* !HOOK_HPP_ */
