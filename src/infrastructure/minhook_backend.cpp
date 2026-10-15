/*
** CrabeLoader
** File description:
** The Win32 backend: MinHook status mapped to typed errors, HDE32 for the stolen byte count.
** The process-wide registry is built here and deliberately never destroyed, see coreRegistry.
** Owns no ownership rule -- those are in src/infrastructure/hook_registry.cpp.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/minhook_backend.hpp"

#include "infrastructure/code_cave.hpp"
#include "infrastructure/memory.hpp"
#include "minhook/MinHook.h"
#include "shared/logger.hpp"

#include <memory>

namespace crabe::infrastructure {

    namespace {

        constexpr std::size_t kMaxInstructionLength = 15;

        // Translates a MinHook status code into a typed HookError.
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
                    return HookError::AlreadyHookedBy;

                default:
                    return HookError::TrampolineAllocationFailed;
            }
        }

    } // namespace

    // Initializes the MinHook library subsystem.
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

    // Destroys the MinHook backend.
    MinHookBackend::~MinHookBackend() = default;

    // Creates and enables a MinHook detour at the specified target address.
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

        if (original != nullptr)
            *original = trampoline;

        status = MH_EnableHook(source);
        if (status != MH_OK) {
            logger.error("MinHookBackend: MH_EnableHook failed at 0x{:X}: {}.",
                         target, MH_StatusToString(status));
            MH_RemoveHook(source);
            if (original != nullptr)
                *original = nullptr;
            return std::unexpected(translate(status));
        }

        return trampoline;
    }

    // Disables and removes a MinHook detour at the specified target address.
    bool MinHookBackend::unpatch(std::uintptr_t target)
    {
        if (!_initialised)
            return false;

        void* const source = reinterpret_cast<void*>(target);

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

    // Measures the number of bytes overwritten by a 5-byte jump at the target.
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

    // Returns the process-wide hook registry singleton.
    HookRegistry& coreRegistry()
    {
        static MinHookBackend* backend = std::make_unique<MinHookBackend>().release();
        static HookRegistry* registry =
            std::make_unique<HookRegistry>(*backend, Attribution::PublishToCrashHandler).release();
        return *registry;
    }

} // namespace crabe::infrastructure
