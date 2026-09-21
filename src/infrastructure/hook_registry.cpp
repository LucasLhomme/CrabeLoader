/*
** CrabeLoader
** File description:
** The ownership half of the registry: collision refusal, reverse removal, the lock-free mirror.
** The mirror is fixed-size and lock-free because the crash handler may not allocate or take a lock.
** Touches no memory: a guard bank fails the build if MinHook or Windows reaches this file.
**
** Authors: @LucasLhomme
*/

// The ownership half of infrastructure/hook_registry.hpp.
//
// Read the header first: it holds the collision policy and the reason the
// patching engine is behind an interface. What follows is the bookkeeping, and
// there is deliberately nothing else in here -- no MinHook, no page protection,
// no trampolines. The guard bank at the foot of this file is what keeps that
// true build after build rather than by good intentions.
//
// Two invariants hold the whole file together:
//
//  * _hooks is in installation order and stays that way. removeAllOwnedBy()
//    walks it backwards, and hooks() hands it out as-is, so anything that
//    reordered it would silently change what "reverse installation order"
//    means.
//  * no two live entries' [target, target + stolenBytes) ranges overlap. That
//    is what install() refuses to break, and it is what lets findCovering()
//    stop at the first match.

#include "infrastructure/hook_registry.hpp"

#include <algorithm>
#include <atomic>
#include <cstring>

#include "shared/logger.hpp"

namespace crabe::infrastructure {

    namespace {

        constexpr std::size_t kNoMirrorSlot = static_cast<std::size_t>(-1);

        // ------------------------------------------------------------------
        // The crash-attribution mirror.
        //
        // File scope, so it lives in .bss and is fully constructed before any
        // hook can be installed: an exception handler must never race a lazy
        // initialiser. Written only by a registry constructed with
        // Attribution::PublishToCrashHandler.
        //
        // `live` is the publication gate. A writer clears it (release) before
        // touching the text, and sets it (release) once target, stolenBytes,
        // owner and name are all in place, so a reader that sees live == true
        // sees a slot that was complete at some point. A reader that catches a
        // slot mid-rewrite gets a mix of two names -- bounded, NUL-terminated,
        // and stated in the header as the accepted cost of not taking a lock
        // inside a crashing process.
        // ------------------------------------------------------------------
        struct MirrorSlot {
            std::atomic<bool> live{false};
            std::atomic<std::uintptr_t> target{0};
            std::atomic<std::size_t> stolenBytes{0};
            char owner[HookAttribution::kTextCapacity]{};
            char name[HookAttribution::kTextCapacity]{};
        };

        MirrorSlot g_mirror[kMaxAttributedHooks];

        // Truncating NUL-terminated copy. The report would rather show a
        // clipped mod id than nothing at all.
        void copyFixed(char* destination, std::size_t capacity, std::string_view text) noexcept
        {
            if (capacity == 0)
                return;
            const std::size_t count = std::min(text.size(), capacity - 1);
            if (count != 0)
                std::memcpy(destination, text.data(), count);
            destination[count] = '\0';
        }

        // Whether [aStart, aStart + aSize) and [bStart, bStart + bSize) share a
        // byte. Sizes are always >= kMinimumStolenBytes, so neither range is
        // empty and the naive comparison is correct.
        constexpr bool rangesOverlap(std::uintptr_t aStart, std::size_t aSize,
                                     std::uintptr_t bStart, std::size_t bSize) noexcept
        {
            return aStart < bStart + bSize && bStart < aStart + aSize;
        }

    } // namespace

    const char* describe(HookError error) noexcept
    {
        switch (error) {
            case HookError::TargetUnreadable:                return "the target address is 0 or unreadable";
            case HookError::TargetNotExecutable:             return "the target page is not executable";
            case HookError::NotEnoughSpaceForJump:           return "the target is shorter than a 5-byte jmp";
            case HookError::InstructionBoundaryUnresolvable: return "an instruction boundary could not be resolved";
            case HookError::TargetTooFarForRel32:            return "the detour is out of rel32 range of the target";
            case HookError::AlreadyHookedBy:                 return "those bytes are already hooked";
            case HookError::TrampolineAllocationFailed:      return "the trampoline could not be allocated";
            case HookError::DetourUnusable:                  return "the detour pointer is null";
        }
        return "unknown hook error";
    }

    HookRegistry::HookRegistry(IHookBackend& backend, Attribution attribution)
        : _backend(backend), _attribution(attribution)
    {
    }

    // Removes what is still installed, newest first, for the same reason
    // removeAllOwnedBy() does. A registry going away with live detours pointing
    // at it would leave the process running patched code whose owner no longer
    // exists.
    HookRegistry::~HookRegistry()
    {
        const std::lock_guard<std::mutex> lock(_mutex);
        while (!_hooks.empty())
            unpatchAndForgetLocked(_hooks.size() - 1);
    }

    std::expected<HookHandle, HookError>
    HookRegistry::install(OwnerId owner, std::string_view name, std::uintptr_t target, void* detour,
                          void** original)
    {
        crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

        // An address of 0 means "not resolved" everywhere in this loader, and
        // patching it would overwrite whatever is mapped low and take the
        // process down on the next call. Refused, not warned about.
        if (target == 0) {
            logger.error("HookRegistry: '{}' refused for owner '{}': {}.",
                         name, owner, describe(HookError::TargetUnreadable));
            return std::unexpected(HookError::TargetUnreadable);
        }
        if (detour == nullptr) {
            logger.error("HookRegistry: '{}' refused for owner '{}': {}.",
                         name, owner, describe(HookError::DetourUnusable));
            return std::unexpected(HookError::DetourUnusable);
        }

        const std::lock_guard<std::mutex> lock(_mutex);

        // Idempotency first, before the overlap scan: a re-install of the exact
        // same hook is not a collision with itself.
        for (const Entry& entry : _hooks) {
            if (entry.target == target && entry.owner == owner && entry.detour == detour) {
                // The trampoline the first install produced, kept for exactly
                // this: a caller that asks twice must not be handed a null it
                // would then call through.
                if (original != nullptr)
                    *original = entry.trampoline;
                logger.debug("HookRegistry: '{}' at 0x{:X} is already installed for owner '{}'; "
                             "reusing handle {}.",
                             name, target, owner, entry.handle.id);
                return entry.handle;
            }
        }

        // Measured before the patch, because afterwards these bytes are our own
        // jmp and the answer would always be 5.
        std::size_t stolenBytes = _backend.measureStolenBytes(target);
        if (stolenBytes == 0) {
            stolenBytes = kMinimumStolenBytes;
            logger.warning("HookRegistry: could not measure the patch length at 0x{:X} for '{}'; "
                           "assuming {} bytes, so an overlapping hook within that range may go "
                           "undetected.",
                           target, name, kMinimumStolenBytes);
        }

        for (const Entry& entry : _hooks) {
            if (!rangesOverlap(target, stolenBytes, entry.target, entry.stolenBytes))
                continue;

            _collisions.push_back(HookCollision{target, entry.owner, owner});

            // Loud on purpose. This is the failure the whole file exists to
            // stop being silent, and the user has to be told both names to
            // pick between them.
            logger.error("HookRegistry: '{}' (owner '{}') refused at 0x{:X}: bytes 0x{:X}..0x{:X} "
                         "are already covered by '{}' (owner '{}').",
                         name, owner, target,
                         entry.target, entry.target + entry.stolenBytes,
                         entry.name, entry.owner);
            return std::unexpected(HookError::AlreadyHookedBy);
        }

        std::expected<void*, HookError> patched = _backend.patch(target, detour, original);
        if (!patched) {
            logger.error("HookRegistry: '{}' (owner '{}') failed at 0x{:X}: {}.",
                         name, owner, target, describe(patched.error()));
            return std::unexpected(patched.error());
        }

        Entry entry;
        entry.handle = HookHandle{_nextId++};
        entry.owner = std::move(owner);
        entry.name = std::string(name);
        entry.target = target;
        entry.stolenBytes = stolenBytes;
        entry.detour = detour;
        entry.trampoline = *patched;

        publishLocked(entry);
        _hooks.push_back(std::move(entry));

        const Entry& stored = _hooks.back();
        logger.debug("HookRegistry: '{}' installed at 0x{:X} for owner '{}' ({} bytes, handle {}).",
                     stored.name, stored.target, stored.owner, stored.stolenBytes,
                     stored.handle.id);
        return stored.handle;
    }

    std::size_t HookRegistry::removeAllOwnedBy(const OwnerId& owner)
    {
        const std::lock_guard<std::mutex> lock(_mutex);

        std::size_t removed = 0;

        // Backwards, so the newest hook goes first and so erasing an element
        // cannot move one this loop has yet to look at.
        for (std::size_t i = _hooks.size(); i-- > 0;) {
            if (_hooks[i].owner != owner)
                continue;
            unpatchAndForgetLocked(i);
            ++removed;
        }

        if (removed != 0) {
            crabe::shared::Logger::getInstance().info(
                "HookRegistry: removed {} hook(s) owned by '{}'.", removed, owner);
        }
        return removed;
    }

    bool HookRegistry::remove(HookHandle handle)
    {
        if (!handle.valid())
            return false;

        const std::lock_guard<std::mutex> lock(_mutex);

        const auto it = std::find_if(_hooks.begin(), _hooks.end(),
                                     [handle](const Entry& entry) { return entry.handle == handle; });
        if (it == _hooks.end())
            return false;

        unpatchAndForgetLocked(static_cast<std::size_t>(it - _hooks.begin()));
        return true;
    }

    std::optional<HookInfo> HookRegistry::findCovering(std::uintptr_t address) const
    {
        const std::lock_guard<std::mutex> lock(_mutex);

        for (const Entry& entry : _hooks) {
            if (address < entry.target || address >= entry.target + entry.stolenBytes)
                continue;
            return HookInfo{entry.name, entry.owner, entry.target, entry.stolenBytes};
        }
        return std::nullopt;
    }

    std::span<const HookCollision> HookRegistry::collisions() const
    {
        const std::lock_guard<std::mutex> lock(_mutex);
        return std::span<const HookCollision>(_collisions);
    }

    void HookRegistry::clearCollisions()
    {
        const std::lock_guard<std::mutex> lock(_mutex);
        _collisions.clear();
    }

    std::vector<HookInfo> HookRegistry::hooks() const
    {
        const std::lock_guard<std::mutex> lock(_mutex);

        std::vector<HookInfo> result;
        result.reserve(_hooks.size());
        for (const Entry& entry : _hooks)
            result.push_back(HookInfo{entry.name, entry.owner, entry.target, entry.stolenBytes});
        return result;
    }

    std::size_t HookRegistry::size() const
    {
        const std::lock_guard<std::mutex> lock(_mutex);
        return _hooks.size();
    }

    std::uint64_t HookRegistry::fingerprint() const
    {
        const std::lock_guard<std::mutex> lock(_mutex);

        // FNV-1a over every field that describes the table's contents and
        // order. Not a cryptographic digest and not meant to be -- it only has
        // to change when the table does.
        std::uint64_t hash = 0xcbf29ce484222325ULL;
        const auto mix = [&hash](std::uint64_t value) noexcept {
            for (int byte = 0; byte < 8; ++byte) {
                hash ^= (value >> (byte * 8)) & 0xFFULL;
                hash *= 0x100000001b3ULL;
            }
        };
        const auto mixText = [&hash](std::string_view text) noexcept {
            for (const char character : text) {
                hash ^= static_cast<unsigned char>(character);
                hash *= 0x100000001b3ULL;
            }
            hash ^= 0xFFULL; // A separator, so {"ab",""} and {"a","b"} differ.
            hash *= 0x100000001b3ULL;
        };

        mix(_hooks.size());
        for (const Entry& entry : _hooks) {
            mixText(entry.owner);
            mixText(entry.name);
            mix(entry.target);
            mix(entry.stolenBytes);
            mix(reinterpret_cast<std::uintptr_t>(entry.detour));
        }
        return hash;
    }

    void HookRegistry::unpatchAndForgetLocked(std::size_t index)
    {
        Entry& entry = _hooks[index];

        // Retired from the mirror before the bytes go back, so a handler can
        // never name a hook that is no longer installed.
        retireLocked(entry);

        if (!_backend.unpatch(entry.target)) {
            // The registry forgets it regardless. A target the backend no
            // longer knows about is not one this registry can keep claiming to
            // own, and leaving the entry would block every future install
            // there with a collision against a hook that is already gone.
            crabe::shared::Logger::getInstance().warning(
                "HookRegistry: the backend did not recognise 0x{:X} ('{}', owner '{}') on removal; "
                "dropping the record anyway.",
                entry.target, entry.name, entry.owner);
        }

        _hooks.erase(_hooks.begin() + static_cast<std::ptrdiff_t>(index));
    }

    void HookRegistry::publishLocked(Entry& entry)
    {
        if (_attribution != Attribution::PublishToCrashHandler)
            return;

        // A plain scan, no compare-exchange: writers are serialised by _mutex
        // and only one registry in a process publishes, so the only concurrent
        // access is a reader, which `live` already orders against.
        for (std::size_t index = 0; index < kMaxAttributedHooks; ++index) {
            MirrorSlot& slot = g_mirror[index];
            if (slot.live.load(std::memory_order_relaxed))
                continue;

            // Text first, then the range, then `live` with a release store: a
            // reader that sees the slot live sees all four fields written.
            copyFixed(slot.owner, HookAttribution::kTextCapacity, entry.owner);
            copyFixed(slot.name, HookAttribution::kTextCapacity, entry.name);
            slot.target.store(entry.target, std::memory_order_relaxed);
            slot.stolenBytes.store(entry.stolenBytes, std::memory_order_relaxed);
            slot.live.store(true, std::memory_order_release);

            entry.mirrorSlot = index;
            return;
        }

        // More than kMaxAttributedHooks live hooks. The registry still works;
        // only the crash report's attribution is incomplete, and it says so
        // rather than letting the reader assume the list is exhaustive.
        crabe::shared::Logger::getInstance().warning(
            "HookRegistry: the crash-attribution table is full at {} entries; '{}' will not be "
            "named in a crash report.",
            kMaxAttributedHooks, entry.name);
    }

    void HookRegistry::retireLocked(const Entry& entry)
    {
        if (entry.mirrorSlot == kNoMirrorSlot)
            return;

        MirrorSlot& slot = g_mirror[entry.mirrorSlot];
        slot.live.store(false, std::memory_order_release);
        slot.target.store(0, std::memory_order_relaxed);
        slot.stolenBytes.store(0, std::memory_order_relaxed);
        slot.owner[0] = '\0';
        slot.name[0] = '\0';
    }

    bool crashAttribution(std::uintptr_t address, HookAttribution& out) noexcept
    {
        for (const MirrorSlot& slot : g_mirror) {
            if (!slot.live.load(std::memory_order_acquire))
                continue;

            const std::uintptr_t target = slot.target.load(std::memory_order_relaxed);
            const std::size_t stolenBytes = slot.stolenBytes.load(std::memory_order_relaxed);
            if (target == 0 || address < target || address >= target + stolenBytes)
                continue;

            out.target = target;
            out.stolenBytes = stolenBytes;
            copyFixed(out.owner, HookAttribution::kTextCapacity, slot.owner);
            copyFixed(out.name, HookAttribution::kTextCapacity, slot.name);
            return true;
        }
        return false;
    }

    std::size_t crashAttributionTable(HookAttribution* out, std::size_t capacity) noexcept
    {
        if (out == nullptr)
            return 0;

        std::size_t written = 0;
        for (const MirrorSlot& slot : g_mirror) {
            if (written == capacity)
                break;
            if (!slot.live.load(std::memory_order_acquire))
                continue;

            out[written].target = slot.target.load(std::memory_order_relaxed);
            out[written].stolenBytes = slot.stolenBytes.load(std::memory_order_relaxed);
            copyFixed(out[written].owner, HookAttribution::kTextCapacity, slot.owner);
            copyFixed(out[written].name, HookAttribution::kTextCapacity, slot.name);
            ++written;
        }
        return written;
    }

    void clearCrashAttribution() noexcept
    {
        for (MirrorSlot& slot : g_mirror) {
            slot.live.store(false, std::memory_order_release);
            slot.target.store(0, std::memory_order_relaxed);
            slot.stolenBytes.store(0, std::memory_order_relaxed);
            slot.owner[0] = '\0';
            slot.name[0] = '\0';
        }
    }

} // namespace crabe::infrastructure

// ---------------------------------------------------------------------------
// No hook engine reached this file, enforced by the compiler.
//
// T13's acceptance asks that the registry compile and its tests link without
// MinHook, and asks for that to be mechanically true rather than asserted. This
// is the first of two checks; the second is the crabe_hook_registry_pure target
// in tests/CMakeLists.txt, whose link closure is this file plus the logger and
// nothing else. If the ownership logic ever grew a dependency on the patching
// engine, one of them would stop the build.
//
// MinHook.h opens with `#pragma once` and defines exactly one macro,
// MH_ALL_HOOKS, so that macro is the only preprocessor evidence of it there is.
// It is checked first and on its own.
//
// Windows is checked as well, and that needs justifying, because <windows.h> is
// perfectly legal elsewhere in this layer. Two reasons. It is a second,
// independent net under the first: MinHook.h includes <windows.h>
// unconditionally, so a future MinHook that stopped defining MH_ALL_HOOKS would
// still be caught. And it states the stronger thing that is actually true and
// actually load-bearing -- the ownership logic needs nothing from the operating
// system at all, which is exactly why it can be driven by FakeHookBackend. A
// hand-rolled backend calling VirtualProtect straight from this file would slip
// past a MinHook-only guard; it does not slip past this one.
// ---------------------------------------------------------------------------

#if defined(MH_ALL_HOOKS)
#error "hook_registry.cpp is not backend-free: MinHook.h reached it transitively"
#endif
#if defined(_WINDOWS_) || defined(_INC_WINDOWS) || defined(_WINBASE_) || defined(_MINWINBASE_)
#error "hook_registry.cpp is not backend-free: a Win32 header reached it, so something in \
here is reaching for the operating system instead of leaving that to IHookBackend"
#endif
