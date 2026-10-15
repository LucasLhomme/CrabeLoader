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
        static constexpr std::uintptr_t kTrampolineBase = 0x7A000000;

        std::expected<void*, HookError> patch(std::uintptr_t target, void* detour,
                                              void** original) override
        {
            if (_nextFailure.has_value()) {
                const HookError error = *_nextFailure;
                _nextFailure.reset();
                return std::unexpected(error);
            }

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

        // Configures the next patch call to fail with the specified error.
        void failNextPatchWith(HookError error) { _nextFailure = error; }

        // Sets the measured stolen bytes override for a specific target.
        void setStolenBytes(std::uintptr_t target, std::size_t bytes)
        {
            _stolenBytes[target] = bytes;
        }

        // Sets the default measured stolen bytes returned when no target override exists.
        void setDefaultStolenBytes(std::size_t bytes) { _defaultStolenBytes = bytes; }

        [[nodiscard]] bool isPatched(std::uintptr_t target) const { return _patched.contains(target); }

        [[nodiscard]] std::size_t patchedCount() const { return _patched.size(); }

        // Returns the detour function pointer registered for the given target, or nullptr.
        [[nodiscard]] void* detourAt(std::uintptr_t target) const
        {
            const auto it = _patched.find(target);
            return it == _patched.end() ? nullptr : it->second.detour;
        }

        // Returns the trampoline pointer registered for the given target, or nullptr.
        [[nodiscard]] void* trampolineAt(std::uintptr_t target) const
        {
            const auto it = _patched.find(target);
            return it == _patched.end() ? nullptr : it->second.trampoline;
        }

        // Returns the recorded sequence of patched targets in call order.
        [[nodiscard]] std::span<const std::uintptr_t> patchOrder() const { return _patchOrder; }

        // Returns the recorded sequence of unpatched targets in call order.
        [[nodiscard]] std::span<const std::uintptr_t> unpatchOrder() const { return _unpatchOrder; }

        [[nodiscard]] std::size_t measureCalls() const { return _measureCalls; }

        // Clears the history of patch and unpatch calls without modifying current patches.
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
