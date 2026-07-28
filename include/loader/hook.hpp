/*
** CrabeLoader
** File description:
** hook
*/

#ifndef HOOK_HPP_
#define HOOK_HPP_

#include <cstddef>
#include <windows.h>

// A single x86 trampoline hook.
//
// install() overwrites the first bytes of `src` with a jmp to `dst`, and keeps a
// copy of those bytes -- followed by a jmp back -- in an executable trampoline.
// getOriginal() returns that trampoline, so a detour can still call through.
//
// The number of stolen bytes is decided by Memory::PrologueLength(), never by the
// caller: it must cover *whole* instructions, and a wrong count corrupts the host
// process' code. install() refuses to hook a prologue it cannot size.
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
        static void writeJump(void* from, void* to);

        void* _src = nullptr;
        void* _dst = nullptr;
        void* _trampoline = nullptr;
        size_t _len = 0;
        bool _installed = false;
};

#endif /* !HOOK_HPP_ */
