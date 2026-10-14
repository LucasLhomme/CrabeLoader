/*
** CrabeLoader
** File description:
** Detours the exe's entry point through the hook registry, with a naked x86 stub as detour.
** The stub saves every register, runs the callback, restores them and jumps on to the entry.
** Runs the callback at most once, and swallows any C++ exception it lets escape.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/entry_point_hook.hpp"

#include <windows.h>

#include <atomic>
#include <cstdint>

#include "infrastructure/hook.hpp"
#include "shared/logger.hpp"

namespace crabe::infrastructure {

    namespace {

        EntryCallback g_callback = nullptr;
        void* g_entryTrampoline = nullptr;
        std::atomic_bool g_ran{false};

        /// The hook on the entry point; never removed, since the entry point never returns.
        Hook& entryHook()
        {
            static Hook hook;
            return hook;
        }

        /// Runs the callback the first time the entry point is reached.
        void __cdecl runEntryCallback() noexcept
        {
            if (g_ran.exchange(true) || !g_callback)
                return;
            try {
                g_callback();
            } catch (...) {
                crabe::shared::Logger::getInstance().error("EntryPointHook: the entry callback threw; the game starts anyway.");
            }
        }

        /// Stands in for the entry point: keeps the registers the system handed it, then continues into the game.
        __declspec(naked) void entryDetour()
        {
            __asm {
                pushad
                pushfd
                call runEntryCallback
                popfd
                popad
                jmp dword ptr [g_entryTrampoline]
            }
        }

        /// Returns the executable's entry point address, or 0 when its headers do not look like a PE image.
        std::uintptr_t executableEntryPoint()
        {
            const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
            if (!base)
                return 0;
            const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
            if (dos->e_magic != IMAGE_DOS_SIGNATURE)
                return 0;
            const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
            if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.AddressOfEntryPoint == 0)
                return 0;
            return base + nt->OptionalHeader.AddressOfEntryPoint;
        }

    } // namespace

    bool runBeforeEntryPoint(EntryCallback callback)
    {
        const std::uintptr_t entry = executableEntryPoint();
        if (!callback || !entry || entryHook().isInstalled())
            return false;
        g_callback = callback;
        if (!entryHook().install(reinterpret_cast<void*>(entry), reinterpret_cast<void*>(&entryDetour),
                                 "EntryPoint")) {
            g_callback = nullptr;
            return false;
        }
        g_entryTrampoline = entryHook().getOriginal();
        return true;
    }

} // namespace crabe::infrastructure
