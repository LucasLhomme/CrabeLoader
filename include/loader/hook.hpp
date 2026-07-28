/*
** CrabeLoader
** File description:
** hook
*/

#ifndef HOOK_HPP_
#define HOOK_HPP_
#include <windows.h>
#include <cstring>
#include <cstdint>

class Hook {
    public:
        Hook();
        ~Hook();
        bool install(void* src, void* dst, size_t len);
        void remove();
        bool IsInstalled();
        bool UnlockProtection(void* address, size_t size, DWORD newProtect, DWORD* oldProtect);


    protected:
    private:
        void* _src = nullptr;
        void* _dst = nullptr;
        void* _trampoline = nullptr;
        int _len = 0;
        bool _installed = false;

        void *allocateTrampoline();
        bool changeMemoryProtection(void* target, size_t size, DWORD newProtect, DWORD& oldProtect);
        void writeJump(void* from, void* to);
        void padWithNops(void* target, size_t offset, size_t totalLen);
        void restoreMemoryProtection(void* target, size_t size, DWORD oldProtect);
        void clearMemory(void* target, size_t size);

};

#endif /* !HOOK_HPP_ */
