/*
** CrabeLoader
** File description:
** hook
*/

#ifndef HOOK_HPP_
#define HOOK_HPP_

#include <windows.h>

// A single inline detour hook, backed by MinHook.
//
// install() replaces `src` with a detour to `dst` and keeps the MinHook-built
// trampoline that still runs the original stolen instructions.
// getOriginal() returns that trampoline, so a detour can call through to the
// real function.
//
// MinHook is reference-counted process-wide (MH_Initialize/MH_Uninitialize):
// each installed Hook holds one reference, so unrelated Hook instances can be
// created and destroyed independently without tearing down hooks still in use.
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
