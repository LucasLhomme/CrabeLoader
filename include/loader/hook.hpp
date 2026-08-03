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

        bool install(void* src, void* dst);
        void remove();

        bool isInstalled() const;
        void* getOriginal() const;

    private:
        void* _src = nullptr;
        void* _trampoline = nullptr;
        bool _installed = false;
};

#endif /* !HOOK_HPP_ */
