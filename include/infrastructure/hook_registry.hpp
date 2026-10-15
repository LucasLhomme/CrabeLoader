/*
** CrabeLoader
** File description:
** Declares the inventory of every installed hook with its owner, and the refusal of collisions.
** Two owners hooking one address used to corrupt each other in silence; that is now refused.
** Touches no memory and needs no Windows: all patching sits behind IHookBackend.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_HOOK_REGISTRY_HPP_
#define CRABELOADER_INFRASTRUCTURE_HOOK_REGISTRY_HPP_

#include <cstddef>
#include <cstdint>
#include <expected>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace crabe::infrastructure {

    enum class HookError : std::uint8_t {
        TargetUnreadable,
        TargetNotExecutable,
        NotEnoughSpaceForJump,
        InstructionBoundaryUnresolvable,
        TargetTooFarForRel32,
        AlreadyHookedBy,
        TrampolineAllocationFailed,
        DetourUnusable,
    };

    // Returns a short, stable description string for a hook error.
    [[nodiscard]] const char* describe(HookError error) noexcept;

    using OwnerId = std::string;

    inline constexpr const char* kCoreOwner = "core";
    inline constexpr std::size_t kMinimumStolenBytes = 5;

    struct HookHandle {
        std::uint64_t id{0};

        // Checks whether the handle represents a valid identifier.
        [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }

        [[nodiscard]] friend constexpr bool operator==(HookHandle, HookHandle) noexcept = default;
    };

    struct HookInfo {
        std::string name;
        OwnerId owner;
        std::uintptr_t target{0};
        std::size_t stolenBytes{0};
    };

    struct HookCollision {
        std::uintptr_t target{0};
        OwnerId incumbent;
        OwnerId challenger;
    };

    struct IHookBackend {
        virtual ~IHookBackend() = default;

        // Redirects target to detour and returns the original code trampoline.
        // Writes the trampoline to original if non-null.
        virtual std::expected<void*, HookError> patch(std::uintptr_t target, void* detour,
                                                      void** original) = 0;

        // Restores the original bytes at target. Returns false if not patched.
        virtual bool unpatch(std::uintptr_t target) = 0;

        // Returns the number of bytes a patch at target will cover.
        virtual std::size_t measureStolenBytes(std::uintptr_t target) = 0;
    };

    enum class Attribution : std::uint8_t {
        Private,
        PublishToCrashHandler,
    };

    class HookRegistry {
    public:
        // Constructs a HookRegistry with the given hook backend and attribution mode.
        explicit HookRegistry(IHookBackend& backend,
                              Attribution attribution = Attribution::Private);

        // Destructor.
        ~HookRegistry();

        HookRegistry(const HookRegistry&) = delete;
        HookRegistry& operator=(const HookRegistry&) = delete;

        // Installs a hook at target and records owner.
        // Returns the created handle or an error if a collision or backend failure occurs.
        [[nodiscard]] std::expected<HookHandle, HookError>
        install(OwnerId owner, std::string_view name, std::uintptr_t target, void* detour,
                void** original);

        // Removes every hook owned by owner in reverse installation order.
        // Returns the number of removed hooks.
        std::size_t removeAllOwnedBy(const OwnerId& owner);

        // Removes the hook identified by handle. Returns false if handle is invalid or not found.
        bool remove(HookHandle handle);

        // Finds the installed hook covering the given address, if any.
        [[nodiscard]] std::optional<HookInfo> findCovering(std::uintptr_t address) const;

        // Returns all recorded hook collision attempts.
        [[nodiscard]] std::span<const HookCollision> collisions() const;

        // Clears all recorded hook collision attempts.
        void clearCollisions();

        // Returns all installed hooks in installation order.
        [[nodiscard]] std::vector<HookInfo> hooks() const;

        // Returns the count of installed hooks.
        [[nodiscard]] std::size_t size() const;

        // Computes a checksum over installed hook contents and order.
        [[nodiscard]] std::uint64_t fingerprint() const;

    private:
        struct Entry {
            HookHandle handle;
            OwnerId owner;
            std::string name;
            std::uintptr_t target{0};
            std::size_t stolenBytes{0};
            void* detour{nullptr};
            void* trampoline{nullptr};
            std::size_t mirrorSlot{static_cast<std::size_t>(-1)};
        };

        // Unpatches and removes a hook entry at the given index.
        void unpatchAndForgetLocked(std::size_t index);

        // Publishes a hook entry into the crash attribution mirror.
        void publishLocked(Entry& entry);

        // Retires a hook entry from the crash attribution mirror.
        void retireLocked(const Entry& entry);

        IHookBackend& _backend;
        Attribution _attribution;

        mutable std::mutex _mutex;
        std::vector<Entry> _hooks;
        std::vector<HookCollision> _collisions;
        std::uint64_t _nextId{1};
    };

    // Returns the singleton core HookRegistry backed by MinHook.
    [[nodiscard]] HookRegistry& coreRegistry();

    inline constexpr std::size_t kMaxAttributedHooks = 64;

    struct HookAttribution {
        static constexpr std::size_t kTextCapacity = 64;

        std::uintptr_t target{0};
        std::size_t stolenBytes{0};
        char owner[kTextCapacity]{};
        char name[kTextCapacity]{};
    };

    // Resolves which hook covers address using the crash attribution mirror.
    // Safe to call from crash handler context without taking locks or allocating.
    [[nodiscard]] bool crashAttribution(std::uintptr_t address, HookAttribution& out) noexcept;

    // Copies mirrored hook attributions up to capacity into out.
    // Returns the count of copied entries.
    [[nodiscard]] std::size_t crashAttributionTable(HookAttribution* out,
                                                    std::size_t capacity) noexcept;

    // Empties the crash attribution mirror.
    void clearCrashAttribution() noexcept;

} // namespace crabe::infrastructure

#endif /* !CRABELOADER_INFRASTRUCTURE_HOOK_REGISTRY_HPP_ */
