/*
** CrabeLoader
** File description:
** Declares the in-memory IHookBackend a test drives instead of a real patched process.
** No address handed to it is ever dereferenced, so a test may invent one such as 0x401000.
** Lives in include/ rather than tests/ so anything linking the registry can also drive it.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_FAKE_HOOK_BACKEND_HPP_
#define CRABELOADER_INFRASTRUCTURE_FAKE_HOOK_BACKEND_HPP_

// The IHookBackend a test uses: an in-memory map instead of a patched process.
//
// It exists so the interesting half of hooking -- who owns what, what collides
// with what, what order things come out in -- can be asserted without a running
// game. Nothing here patches memory, and no address handed to it is ever
// dereferenced, so a test is free to invent addresses like 0x401000.
//
// It also has to be able to *fail*, because half the registry's behaviour is
// what it does when the engine says no. failNextPatchWith() is how a test
// reaches the error paths MinHook would only produce against a real function.
//
// Header-only and inline on purpose: it is a test double, and giving it a
// translation unit in the shipped DLL would be the one way to accidentally link
// it into the loader.

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

#include "infrastructure/hook_registry.hpp"

namespace crabe::infrastructure {

    class FakeHookBackend final : public IHookBackend {
    public:
        // Where the fake's trampoline pointers start. A recognisable
        // non-null value that is never dereferenced: the registry only stores
        // it and hands it back, and a test only checks it is distinct and not
        // null. Deliberately not a real allocation -- a test that called
        // through one would be testing nothing.
        static constexpr std::uintptr_t kTrampolineBase = 0x7A000000;

        std::expected<void*, HookError> patch(std::uintptr_t target, void* detour,
                                              void** original) override
        {
            if (_nextFailure.has_value()) {
                const HookError error = *_nextFailure;
                _nextFailure.reset();
                return std::unexpected(error);
            }

            // MinHook refuses a second hook on the same address, so the fake
            // must too -- otherwise a test could "pass" against behaviour the
            // real backend would never allow.
            if (_patched.contains(target))
                return std::unexpected(HookError::AlreadyHookedBy);

            void* const trampoline = reinterpret_cast<void*>(kTrampolineBase + _trampolinesHandedOut);
            ++_trampolinesHandedOut;

            _patched.emplace(target, Patch{detour, trampoline});
            _patchOrder.push_back(target);

            if (original != nullptr)
                *original = trampoline;
            return trampoline;
        }

        bool unpatch(std::uintptr_t target) override
        {
            const auto it = _patched.find(target);
            if (it == _patched.end())
                return false;
            _patched.erase(it);
            _unpatchOrder.push_back(target);
            return true;
        }

        std::size_t measureStolenBytes(std::uintptr_t target) override
        {
            ++_measureCalls;
            const auto it = _stolenBytes.find(target);
            return it == _stolenBytes.end() ? _defaultStolenBytes : it->second;
        }

        // ---- Steering ----

        // The next patch() fails with `error` and consumes the setting; every
        // one after that succeeds again.
        void failNextPatchWith(HookError error) { _nextFailure = error; }

        // What measureStolenBytes() answers for one target, overriding the
        // default. 0 is meaningful: it is how a test reaches the registry's
        // "the backend could not tell" fallback.
        void setStolenBytes(std::uintptr_t target, std::size_t bytes)
        {
            _stolenBytes[target] = bytes;
        }

        // What measureStolenBytes() answers for a target with no override.
        void setDefaultStolenBytes(std::size_t bytes) { _defaultStolenBytes = bytes; }

        // ---- Observation ----
        //
        // The registry's own accounting is not evidence that anything was
        // patched. These are what a test compares it against.

        [[nodiscard]] bool isPatched(std::uintptr_t target) const { return _patched.contains(target); }

        [[nodiscard]] std::size_t patchedCount() const { return _patched.size(); }

        // The detour and trampoline recorded for `target`, or nullptr if it is
        // not patched.
        [[nodiscard]] void* detourAt(std::uintptr_t target) const
        {
            const auto it = _patched.find(target);
            return it == _patched.end() ? nullptr : it->second.detour;
        }

        [[nodiscard]] void* trampolineAt(std::uintptr_t target) const
        {
            const auto it = _patched.find(target);
            return it == _patched.end() ? nullptr : it->second.trampoline;
        }

        // Every target patch() accepted, and every target unpatch() accepted,
        // in call order. The second is how "removal in reverse installation
        // order" is proved against the backend rather than against the
        // registry's own bookkeeping.
        [[nodiscard]] std::span<const std::uintptr_t> patchOrder() const { return _patchOrder; }
        [[nodiscard]] std::span<const std::uintptr_t> unpatchOrder() const { return _unpatchOrder; }

        [[nodiscard]] std::size_t measureCalls() const { return _measureCalls; }

        // Forgets the call history but not what is currently patched: a test
        // that wants a clean order log without tearing its hooks down first.
        void resetHistory()
        {
            _patchOrder.clear();
            _unpatchOrder.clear();
            _measureCalls = 0;
        }

    private:
        struct Patch {
            void* detour{nullptr};
            void* trampoline{nullptr};
        };

        std::unordered_map<std::uintptr_t, Patch> _patched;
        std::unordered_map<std::uintptr_t, std::size_t> _stolenBytes;
        std::vector<std::uintptr_t> _patchOrder;
        std::vector<std::uintptr_t> _unpatchOrder;
        std::optional<HookError> _nextFailure;
        std::size_t _defaultStolenBytes{kMinimumStolenBytes};
        std::size_t _measureCalls{0};
        std::uintptr_t _trampolinesHandedOut{0};
    };

} // namespace crabe::infrastructure

#endif /* !CRABELOADER_INFRASTRUCTURE_FAKE_HOOK_BACKEND_HPP_ */
