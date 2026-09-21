/*
** CrabeLoader
** File description:
** The Win32 backend: MinHook status mapped to typed errors, HDE32 for the stolen byte count.
** The process-wide registry is built here and deliberately never destroyed, see coreRegistry.
** Owns no ownership rule -- those are in src/infrastructure/hook_registry.cpp.
**
** Authors: @LucasLhomme
*/

// The Win32 half of infrastructure/minhook_backend.hpp, and the one place the
// loader's process-wide hook registry is built.
//
// MinHook's own vocabulary does not line up one-to-one with HookError, and the
// mapping below is where that is reconciled. Two mismatches are worth knowing
// before reading it:
//
//  * MH_ERROR_UNSUPPORTED_FUNCTION is MinHook's single answer for "no
//    trampoline could be built", whether that is because an instruction could
//    not be decoded or because the function is shorter than a jmp. It maps to
//    InstructionBoundaryUnresolvable, which means NotEnoughSpaceForJump is only
//    ever produced by a backend that can tell the two apart.
//  * MinHook reports nothing that corresponds to TargetTooFarForRel32, and on
//    Win32/x86 it cannot: a rel32 displacement spans the whole address space.
//
// Every branch logs MH_StatusToString's own text as well as the mapped error,
// so nothing is lost to the translation.

#include "infrastructure/minhook_backend.hpp"

#include "infrastructure/code_cave.hpp"
#include "infrastructure/memory.hpp"
#include "minhook/MinHook.h"
#include "shared/logger.hpp"

namespace crabe::infrastructure {

    namespace {

        // The longest x86 instruction, so measureStolenBytes' decode of the
        // instruction straddling the fifth byte cannot read unmapped memory.
        constexpr std::size_t kMaxInstructionLength = 15;

        HookError translate(MH_STATUS status) noexcept
        {
            switch (status) {
                case MH_ERROR_NOT_EXECUTABLE:
                    return HookError::TargetNotExecutable;

                case MH_ERROR_UNSUPPORTED_FUNCTION:
                    return HookError::InstructionBoundaryUnresolvable;

                case MH_ERROR_MEMORY_ALLOC:
                    return HookError::TrampolineAllocationFailed;

                case MH_ERROR_MEMORY_PROTECT:
                    return HookError::TargetUnreadable;

                case MH_ERROR_ALREADY_CREATED:
                case MH_ERROR_ENABLED:
                    // The registry refuses an overlapping install before it
                    // gets here, so reaching this means MinHook holds a hook
                    // the registry does not know about -- something installed
                    // outside the registry, which is exactly the situation it
                    // exists to end.
                    return HookError::AlreadyHookedBy;

                default:
                    // Includes MH_ERROR_NOT_INITIALIZED and MH_UNKNOWN. No
                    // trampoline came back and MinHook will not say why in
                    // terms this enum has a word for; the logged status string
                    // carries the detail.
                    return HookError::TrampolineAllocationFailed;
            }
        }

    } // namespace

    MinHookBackend::MinHookBackend()
    {
        const MH_STATUS status = MH_Initialize();
        _initialised = (status == MH_OK || status == MH_ERROR_ALREADY_INITIALIZED);
        if (!_initialised) {
            crabe::shared::Logger::getInstance().error(
                "MinHookBackend: MH_Initialize failed ({}); no hook can be installed this session.",
                MH_StatusToString(status));
        }
    }

    MinHookBackend::~MinHookBackend() = default;

    std::expected<void*, HookError> MinHookBackend::patch(std::uintptr_t target, void* detour,
                                                         void** original)
    {
        crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

        if (!_initialised)
            return std::unexpected(HookError::TrampolineAllocationFailed);

        void* const source = reinterpret_cast<void*>(target);

        void* trampoline = nullptr;
        MH_STATUS status = MH_CreateHook(source, detour, &trampoline);
        if (status != MH_OK) {
            logger.error("MinHookBackend: MH_CreateHook failed at 0x{:X}: {}.",
                         target, MH_StatusToString(status));
            return std::unexpected(translate(status));
        }

        status = MH_EnableHook(source);
        if (status != MH_OK) {
            logger.error("MinHookBackend: MH_EnableHook failed at 0x{:X}: {}.",
                         target, MH_StatusToString(status));
            // Created but not enabled leaves MinHook holding a record that
            // would make the next install there fail with ALREADY_CREATED, so
            // take it back out before reporting.
            MH_RemoveHook(source);
            return std::unexpected(translate(status));
        }

        if (original != nullptr)
            *original = trampoline;
        return trampoline;
    }

    bool MinHookBackend::unpatch(std::uintptr_t target)
    {
        if (!_initialised)
            return false;

        void* const source = reinterpret_cast<void*>(target);

        // Disable first: MH_RemoveHook on an enabled hook works, but doing it
        // in two steps means the bytes are already back when the record goes.
        const MH_STATUS disabled = MH_DisableHook(source);
        const MH_STATUS removed = MH_RemoveHook(source);

        if (removed != MH_OK) {
            crabe::shared::Logger::getInstance().warning(
                "MinHookBackend: MH_RemoveHook failed at 0x{:X}: {} (disable said {}).",
                target, MH_StatusToString(removed), MH_StatusToString(disabled));
            return false;
        }
        return true;
    }

    std::size_t MinHookBackend::measureStolenBytes(std::uintptr_t target)
    {
        if (target == 0)
            return 0;

        if (!crabe::memory::isReadable(target, kJmpLength + kMaxInstructionLength))
            return 0;

        std::size_t failedAtOffset = 0;
        const std::size_t measured =
            measureStolenLength(reinterpret_cast<const std::uint8_t*>(target), &failedAtOffset);
        if (measured == 0) {
            crabe::shared::Logger::getInstance().debug(
                "MinHookBackend: HDE32 could not decode the instruction at 0x{:X} (+{}).",
                target, failedAtOffset);
        }
        return measured;
    }

    HookRegistry& coreRegistry()
    {
        // Both are deliberately immortal, and that is load-bearing rather than
        // laziness.
        //
        // A Hook lives as a member of another singleton -- RenderHook's,
        // LuaCall's, MessageHook's -- and calls remove() from its own destructor
        // at process exit. Those singletons are constructed *before* this one,
        // because constructing them is what leads to the first install() that
        // gets here, so static destruction order tears this registry down
        // *first*. A Hook destructor running afterwards would then lock a mutex
        // that no longer exists.
        //
        // So neither object is ever destroyed. Nothing is lost by it. MinHook's
        // trampoline pages are already leaked on purpose (see ~MinHookBackend),
        // a process on its way out does not care whether its own code is still
        // patched, and unpatching at that point would race whatever thread is
        // still executing inside a trampoline -- which is precisely the race
        // ~MinHookBackend refuses to take.
        static MinHookBackend* backend = new MinHookBackend();
        static HookRegistry* registry =
            new HookRegistry(*backend, Attribution::PublishToCrashHandler);
        return *registry;
    }

} // namespace crabe::infrastructure
