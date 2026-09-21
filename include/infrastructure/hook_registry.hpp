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

// An inventory of every inline hook the loader has installed, and who owns it.
//
// Until now hooks were installed and forgotten. Three things follow from that,
// and all three are the reason this file exists:
//
//  * two owners hooking the same address corrupt each other in silence. The
//    second detour overwrites the first one's stolen bytes, so the first
//    trampoline now re-executes half an instruction. Nothing reports it; the
//    game simply dies somewhere else later.
//  * nothing can be undone by owner. A mod that installs a hook and is then
//    hot-reloaded leaves the detour pointing into a Lua closure that no longer
//    exists, which is a guaranteed crash on the next call.
//  * a fault inside a detour cannot be attributed. The crash report could name
//    the address, but not whose hook that address belongs to.
//
// So: install() records the owner, the name, the target and how many bytes the
// patch covers; removeAllOwnedBy() undoes one owner's hooks in reverse order;
// and findCovering() answers "whose hook is this address inside?".
//
// The patching itself is deliberately *not* here. It sits behind IHookBackend,
// and this translation unit reaches neither MinHook nor the Win32 hooking APIs
// -- see the compile-time guard at the foot of src/infrastructure/
// hook_registry.cpp, which fails the build if MinHook ever arrives. That is
// what makes the ownership rules a unit test against FakeHookBackend instead of
// a play session: the collision policy, the reverse-order removal and the
// range arithmetic are all pure bookkeeping, and pure bookkeeping is testable.
//
// The impure half is MinHookBackend, in infrastructure/minhook_backend.hpp.

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

    // Why a hook was not installed. Each value is one concrete cause the
    // caller can act on, rather than a bool that means "somewhere, somehow".
    enum class HookError : std::uint8_t {
        // The target address is 0, or the backend could not read the bytes it
        // has to relocate. An unresolved address arrives here: patching 0
        // would overwrite whatever is mapped low and kill the process.
        TargetUnreadable,

        // The page at the target is not executable, so it is not code and the
        // address is wrong.
        TargetNotExecutable,

        // The function is shorter than the 5 bytes a relative jmp needs. Seen
        // on tiny forwarding thunks.
        NotEnoughSpaceForJump,

        // The length disassembler could not tell where an instruction ends, so
        // the patch would split one and leave the tail as garbage.
        InstructionBoundaryUnresolvable,

        // The detour is further from the target than a rel32 displacement can
        // reach. Cannot happen in this loader -- the game is Win32/x86, where
        // the whole address space is within rel32 -- and is kept because the
        // backend interface is not x86-only in principle.
        TargetTooFarForRel32,

        // Something already covers those bytes. The offender is in
        // collisions(); see the collision policy on install() below.
        AlreadyHookedBy,

        // The backend could not allocate the trampoline that calls through to
        // the original code.
        TrampolineAllocationFailed,

        // The detour pointer is null. Not in the same family as the rest --
        // this one is a bug in the caller, not a property of the target -- but
        // it is a real refusal the old Hook::install already made, and I7 says
        // a refusal carries a reason rather than a false.
        DetourUnusable,
    };

    // A short, stable reason string for a log line or a test failure message.
    // Never null; points at a string literal.
    [[nodiscard]] const char* describe(HookError error) noexcept;

    // Who installed a hook: a mod id, or "core" for the loader itself. A plain
    // string for the same reason domain::ModId is one -- it is what a manifest
    // declares and what a report has to print, so a distinct type would buy
    // nothing but conversions.
    using OwnerId = std::string;

    // The owner every hook the loader installs on its own behalf belongs to.
    // Never a valid mod id (validateModId rejects it), so a mod cannot claim
    // the loader's hooks by naming itself "core".
    inline constexpr const char* kCoreOwner = "core";

    // How many bytes a hook is assumed to cover when the backend cannot say.
    // The length of a relative jmp: whatever else a detour does, it overwrites
    // at least this much. Same constant as code_cave.hpp's kJmpLength, spelled
    // again rather than shared because a cave and a detour only happen to
    // agree on it.
    inline constexpr std::size_t kMinimumStolenBytes = 5;

    // The receipt install() hands back.
    //
    // An opaque id rather than an index or a pointer: the table behind it is
    // compacted as hooks come and go, so an index would silently start naming
    // whatever moved into that slot. Ids are never reused, so a handle to a
    // removed hook stays detectably stale for the life of the process.
    struct HookHandle {
        std::uint64_t id{0};

        [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }

        [[nodiscard]] friend constexpr bool operator==(HookHandle, HookHandle) noexcept = default;
    };

    // One installed hook, as the registry knows it.
    struct HookInfo {
        // What the hook is called, for a log line and a crash report, e.g.
        // "RenderHook::IDXGISwapChain::Present".
        std::string name;

        OwnerId owner;

        // The address that was patched.
        std::uintptr_t target{0};

        // How many bytes the patch covers, starting at `target`. This is what
        // makes findCovering() and overlap detection possible: a fault two
        // bytes into a detour is still inside that hook.
        std::size_t stolenBytes{0};
    };

    // An attempt to hook bytes something already covers. Kept rather than only
    // logged, because the interesting case is a *pair* of mods and the user
    // needs to be told both names to choose between them.
    struct HookCollision {
        // The address the challenger asked for -- not necessarily the
        // incumbent's target, since a collision can be a partial overlap.
        std::uintptr_t target{0};

        // Who already covers those bytes.
        OwnerId incumbent;

        // Who was refused.
        OwnerId challenger;
    };

    // The patching engine, abstracted so the registry above it can be tested.
    //
    // Three operations, and nothing about ownership: the backend does not know
    // what a mod is, and the registry does not know what a trampoline is.
    struct IHookBackend {
        virtual ~IHookBackend() = default;

        // Redirects `target` to `detour`. On success the trampoline that calls
        // through to the original code is both returned and, when `original`
        // is non-null, written there. Both, because the registry wants the
        // value and every existing call site wants the out-parameter.
        virtual std::expected<void*, HookError> patch(std::uintptr_t target, void* detour,
                                                      void** original) = 0;

        // Puts the original bytes back. False if `target` was not patched by
        // this backend.
        virtual bool unpatch(std::uintptr_t target) = 0;

        // How many bytes a patch at `target` will cover: a whole number of
        // instructions, at least kMinimumStolenBytes. 0 means the backend
        // could not tell, which the registry treats as kMinimumStolenBytes
        // rather than as a refusal -- see install().
        //
        // Must be called before patch(), for the obvious reason: afterwards
        // the bytes at `target` are the loader's own jmp.
        virtual std::size_t measureStolenBytes(std::uintptr_t target) = 0;
    };

    // Whether a registry mirrors its ranges where a crash handler can read
    // them. Exactly one registry per process should -- the loader's own.
    enum class Attribution : std::uint8_t {
        // The registry keeps its table to itself. What a test registry wants:
        // it must not be able to leave a crash handler reading its ranges
        // after it has gone out of scope.
        Private,

        // ...and also publishes into the fixed-size mirror crashAttribution()
        // reads.
        PublishToCrashHandler,
    };

    class HookRegistry {
    public:
        // `backend` must outlive the registry. Passed by reference rather than
        // owned because the production backend is a process-lifetime static
        // and a test's fake is a local the test also asserts against.
        explicit HookRegistry(IHookBackend& backend,
                              Attribution attribution = Attribution::Private);

        ~HookRegistry();

        HookRegistry(const HookRegistry&) = delete;
        HookRegistry& operator=(const HookRegistry&) = delete;

        // Installs a hook at `target` and records `owner` as its owner.
        //
        // Collision policy, in full:
        //
        //  * the same owner asking again for the same target *and* the same
        //    detour is idempotent: no second patch, and the original handle
        //    comes back. This is what makes a reload that re-runs a mod's
        //    setup safe.
        //  * anything else touching bytes an existing hook already covers --
        //    the same address, or a partial overlap of its stolen range -- is
        //    refused with AlreadyHookedBy, and a HookCollision is recorded. A
        //    partial overlap is just as fatal as an exact hit: the second
        //    jmp lands inside the first one's relocated instructions.
        //  * that includes an owner colliding with itself, at a different
        //    address inside its own range. It is a bug either way, and one
        //    that is silent today is exactly what this file is for.
        //
        // `stolenBytes` in the recorded HookInfo is the backend's measurement,
        // or kMinimumStolenBytes when the backend cannot tell. Not a refusal:
        // a detour always overwrites at least a jmp's worth, so the minimum is
        // a correct lower bound for collision detection, and refusing here
        // would mean a length disassembler that cannot read one game function
        // takes the overlay down with it. The fallback is logged, because an
        // under-measured range is a collision this file might then miss.
        [[nodiscard]] std::expected<HookHandle, HookError>
        install(OwnerId owner, std::string_view name, std::uintptr_t target, void* detour,
                void** original);

        // Removes every hook owned by `owner`, in reverse installation order,
        // and returns how many went. Reverse order because hooks are not
        // independent: the last one installed may be the one whose trampoline
        // the earlier ones' detours call through, so unwinding in the order
        // they went in would run a detour whose original has already been
        // restored. Called on hot-reload and on plugin unload.
        //
        // An owner with no hooks returns 0. Not an error: the reload path calls
        // this for every mod, and most mods hook nothing.
        std::size_t removeAllOwnedBy(const OwnerId& owner);

        // Removes the one hook `handle` names. False if the handle is stale,
        // never installed, or default-constructed.
        bool remove(HookHandle handle);

        // Which hook, if any, covers `address` -- meaning `address` falls in
        // [target, target + stolenBytes). Used to attribute a fault to the
        // owner that installed the hook it happened inside.
        //
        // Allocates (HookInfo carries strings) and takes the registry's lock,
        // so it is for a log line or a test, NOT for an exception handler. The
        // handler's version is crashAttribution() below.
        [[nodiscard]] std::optional<HookInfo> findCovering(std::uintptr_t address) const;

        // Every refused attempt, oldest first.
        //
        // The span is a view into the registry's own storage and is invalidated
        // by the next install(). That is tolerable only because collisions are
        // read after startup or after a reload, never concurrently with one;
        // see the known limitation in CHANGELOG.md.
        [[nodiscard]] std::span<const HookCollision> collisions() const;

        void clearCollisions();

        // ---- Introspection, for tests and for the load report. ----

        // Installed hooks, in installation order.
        [[nodiscard]] std::vector<HookInfo> hooks() const;

        [[nodiscard]] std::size_t size() const;

        // A checksum over the hook table's contents and order.
        //
        // How "removeAllOwnedBy leaves the registry as it was" is asserted
        // without comparing two vectors of strings by hand. Deliberately does
        // *not* mix in the handle counter: that keeps advancing across a
        // remove/install cycle on purpose, so a stale handle stays stale, and
        // folding it in would make the fingerprint of an empty registry depend
        // on how much had happened to it -- which is the opposite of what the
        // check needs to say.
        [[nodiscard]] std::uint64_t fingerprint() const;

    private:
        struct Entry {
            HookHandle handle;
            OwnerId owner;
            std::string name;
            std::uintptr_t target{0};
            std::size_t stolenBytes{0};
            void* detour{nullptr};

            // What the backend handed back, so an idempotent re-install can
            // return the same trampoline instead of a null the caller would
            // then call through.
            void* trampoline{nullptr};

            // Slot in the crash-attribution mirror, or kNoMirrorSlot when this
            // registry does not publish (or the mirror was already full).
            std::size_t mirrorSlot{static_cast<std::size_t>(-1)};
        };

        // Callers already hold _mutex.
        void unpatchAndForgetLocked(std::size_t index);
        void publishLocked(Entry& entry);
        void retireLocked(const Entry& entry);

        IHookBackend& _backend;
        Attribution _attribution;

        mutable std::mutex _mutex;
        std::vector<Entry> _hooks;
        std::vector<HookCollision> _collisions;
        std::uint64_t _nextId{1};
    };

    // The loader's own registry: one per process, backed by MinHook, and the
    // owner of every hook the core installs.
    //
    // Declared here but *defined* in src/infrastructure/minhook_backend.cpp,
    // on purpose. A caller that only needs to reach the registry -- the reload
    // path in domain/, the crash report -- gets it without MinHook.h arriving
    // in its translation unit, and a target that links hook_registry.cpp alone
    // still builds and can be exercised against FakeHookBackend.
    [[nodiscard]] HookRegistry& coreRegistry();

    // ---- Crash-time attribution ----

    // The most hooks the mirror below holds. A fixed table for the same reason
    // CrashReporter's mod list is one: its reader is an exception handler.
    inline constexpr std::size_t kMaxAttributedHooks = 64;

    // One hook, flattened into storage a crash handler can read.
    struct HookAttribution {
        static constexpr std::size_t kTextCapacity = 64;

        std::uintptr_t target{0};
        std::size_t stolenBytes{0};

        // Copies, not pointers into the registry, and truncated to fit. A
        // handler must not be handed anything it could dereference wrongly.
        char owner[kTextCapacity]{};
        char name[kTextCapacity]{};
    };

    // Which hook covers `address`, for a caller that cannot call
    // findCovering().
    //
    // The crash reporter is that caller. findCovering() allocates and takes
    // the registry's mutex, and the handler runs inside a process whose heap
    // may already be corrupt, possibly on the very thread holding that mutex
    // (see the two rules at the head of crash_reporter.hpp). So a registry
    // constructed with Attribution::PublishToCrashHandler also maintains a
    // fixed-size mirror of its ranges, written at install and removal time on
    // an ordinary thread; this reads it with plain atomic loads. No
    // allocation, no lock, nothing for a crashing process to unwind.
    //
    // Worst case a slot is being rewritten as it is read and the text is a mix
    // of two hooks' names. That is bounded and NUL-terminated -- a slightly
    // wrong name in a report, never a bad pointer.
    [[nodiscard]] bool crashAttribution(std::uintptr_t address, HookAttribution& out) noexcept;

    // Every mirrored hook, oldest slot first, for the report's inventory
    // section. Returns how many entries were written into `out`, at most
    // `capacity`. Same guarantees as crashAttribution().
    [[nodiscard]] std::size_t crashAttributionTable(HookAttribution* out,
                                                    std::size_t capacity) noexcept;

    // Empties the mirror. For a test that has published into it and must not
    // leave its ranges visible to anything that runs next.
    void clearCrashAttribution() noexcept;

} // namespace crabe::infrastructure

#endif /* !CRABELOADER_INFRASTRUCTURE_HOOK_REGISTRY_HPP_ */
