/*
** CrabeLoader
** File description:
** Declares the production IHookBackend: MinHook for the patch, HDE32 for the patch length.
** Win32 and x86, and unrunnable without a process to patch, which is why the split exists.
** Including this pulls MinHook.h in; a caller wanting only ownership includes the registry.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_MINHOOK_BACKEND_HPP_
#define CRABELOADER_INFRASTRUCTURE_MINHOOK_BACKEND_HPP_

#include <cstddef>
#include <cstdint>
#include <expected>

#include "infrastructure/hook_registry.hpp"

namespace crabe::infrastructure {

    class MinHookBackend final : public IHookBackend {
    public:
        // Initializes MinHook and records whether initialization succeeded.
        MinHookBackend();

        // Destructor. Avoids calling MH_Uninitialize to prevent tearing down active trampolines.
        ~MinHookBackend() override;

        MinHookBackend(const MinHookBackend&) = delete;
        MinHookBackend& operator=(const MinHookBackend&) = delete;

        // Installs a hook at target using MinHook, returning trampoline or error.
        std::expected<void*, HookError> patch(std::uintptr_t target, void* detour,
                                              void** original) override;

        // Unpatches the hook at target using MinHook.
        bool unpatch(std::uintptr_t target) override;

        // Measures overwritten bytes at target using HDE32 disassembler.
        // Returns 0 if bytes cannot be read or decoded.
        std::size_t measureStolenBytes(std::uintptr_t target) override;

        // Checks whether MinHook initialized successfully.
        [[nodiscard]] bool isAvailable() const noexcept { return _initialised; }

    private:
        bool _initialised = false;
    };

} // namespace crabe::infrastructure

#endif /* !CRABELOADER_INFRASTRUCTURE_MINHOOK_BACKEND_HPP_ */
