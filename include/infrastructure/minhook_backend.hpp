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

// The production IHookBackend: MinHook, plus HDE32 for the patch length.
//
// Everything in hook_registry.hpp that touches live memory is here, and that is
// the whole point of the split. This file is Win32 and x86 and cannot be run
// without a process to patch; the ownership rules next door are neither, and
// are covered by unit tests against FakeHookBackend.
//
// Including this header brings MinHook.h with it. A caller that only needs to
// reach the loader's registry should include infrastructure/hook_registry.hpp
// and call coreRegistry() instead -- that declaration exists so the reload path
// and the crash reporter do not have to compile the hook engine.

#include <cstddef>
#include <cstdint>
#include <expected>

#include "infrastructure/hook_registry.hpp"

namespace crabe::infrastructure {

    class MinHookBackend final : public IHookBackend {
    public:
        // Initialises MinHook once. A failure is recorded rather than thrown:
        // the loader has to keep running without hooks, and every patch()
        // afterwards refuses with a reason instead of patching into a library
        // that has no trampoline heap.
        MinHookBackend();

        // Deliberately does NOT call MH_Uninitialize.
        //
        // MH_Uninitialize disables and removes every hook and frees the
        // trampoline pages. Another thread may be executing inside one of those
        // pages at this exact moment -- during static destruction there is no
        // way to know -- and freeing it would be a race with no upside. So the
        // pages are leaked at process exit, for the same reason CodeCave leaks
        // its cave page. Individual hooks are still removed properly through
        // unpatch(), which is what the registry drives.
        ~MinHookBackend() override;

        MinHookBackend(const MinHookBackend&) = delete;
        MinHookBackend& operator=(const MinHookBackend&) = delete;

        std::expected<void*, HookError> patch(std::uintptr_t target, void* detour,
                                              void** original) override;

        bool unpatch(std::uintptr_t target) override;

        // HDE32's answer, via code_cave.hpp's measureStolenLength: decode
        // forward until at least a jmp's worth of whole instructions is
        // covered. 0 when the target's bytes cannot be read or cannot be
        // decoded, which the registry turns into kMinimumStolenBytes.
        //
        // Reads past the fifth byte by design -- the instruction straddling
        // that boundary has to be decoded in full -- so the readability check
        // covers the worst case: five bytes plus one maximal x86 instruction.
        std::size_t measureStolenBytes(std::uintptr_t target) override;

        [[nodiscard]] bool isAvailable() const noexcept { return _initialised; }

    private:
        bool _initialised = false;
    };

} // namespace crabe::infrastructure

#endif /* !CRABELOADER_INFRASTRUCTURE_MINHOOK_BACKEND_HPP_ */
