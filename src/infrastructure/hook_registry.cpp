/*
** CrabeLoader
** File description:
** The ownership half of the registry: collision refusal, reverse removal, the lock-free mirror.
** The mirror is fixed-size and lock-free because the crash handler may not allocate or take a lock.
** Touches no memory: a guard bank fails the build if MinHook or Windows reaches this file.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/hook_registry.hpp"

#include <algorithm>
#include <atomic>
#include <cstring>

#include "shared/logger.hpp"

namespace crabe::infrastructure {

    namespace {

        constexpr std::size_t kNoMirrorSlot = static_cast<std::size_t>(-1);

        struct MirrorSlot {
            std::atomic<bool> live{false};
            std::atomic<std::uintptr_t> target{0};
            std::atomic<std::size_t> stolenBytes{0};
            char owner[HookAttribution::kTextCapacity]{};
            char name[HookAttribution::kTextCapacity]{};
        };

        MirrorSlot g_mirror[kMaxAttributedHooks];

        // Performs a truncating null-terminated string copy into the destination buffer.
        void copyFixed(char* destination, std::size_t capacity, std::string_view text) noexcept
        {
            if (capacity == 0)
                return;
            const std::size_t count = std::min(text.size(), capacity - 1);
            if (count != 0)
                std::memcpy(destination, text.data(), count);
            destination[count] = '\0';
        }

        // Checks whether two memory address ranges overlap.
        constexpr bool rangesOverlap(std::uintptr_t aStart, std::size_t aSize,
                                     std::uintptr_t bStart, std::size_t bSize) noexcept
        {
            return aStart < bStart + bSize && bStart < aStart + aSize;
        }

    } // namespace

    // Returns a description string for the given hook error.
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

    // Constructs the hook registry with the specified backend and attribution mode.
    HookRegistry::HookRegistry(IHookBackend& backend, Attribution attribution)
        : _backend(backend), _attribution(attribution)
    {
    }

    // Destructor. Removes all remaining hooks in reverse installation order.
    HookRegistry::~HookRegistry()
    {
        const std::lock_guard<std::mutex> lock(_mutex);
        while (!_hooks.empty())
            unpatchAndForgetLocked(_hooks.size() - 1);
    }

    // Installs and registers a hook, rejecting collisions with existing hooks.
    std::expected<HookHandle, HookError>
    HookRegistry::install(OwnerId owner, std::string_view name, std::uintptr_t target, void* detour,
                          void** original)
    {
        crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

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

        for (const Entry& entry : _hooks) {
            if (entry.target == target && entry.owner == owner && entry.detour == detour) {
                if (original != nullptr)
                    *original = entry.trampoline;
                logger.debug("HookRegistry: '{}' at 0x{:X} is already installed for owner '{}'; "
                             "reusing handle {}.",
                             name, target, owner, entry.handle.id);
                return entry.handle;
            }
        }

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

    // Removes all hooks belonging to the specified owner in reverse order.
    std::size_t HookRegistry::removeAllOwnedBy(const OwnerId& owner)
    {
        const std::lock_guard<std::mutex> lock(_mutex);

        std::size_t removed = 0;

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

    // Removes the hook identified by handle.
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

    // Finds the hook covering the given address, if any.
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

    // Returns a span of all recorded hook collision records.
    std::span<const HookCollision> HookRegistry::collisions() const
    {
        const std::lock_guard<std::mutex> lock(_mutex);
        return std::span<const HookCollision>(_collisions);
    }

    // Clears all recorded hook collision records.
    void HookRegistry::clearCollisions()
    {
        const std::lock_guard<std::mutex> lock(_mutex);
        _collisions.clear();
    }

    // Returns a snapshot copy of all installed hooks.
    std::vector<HookInfo> HookRegistry::hooks() const
    {
        const std::lock_guard<std::mutex> lock(_mutex);

        std::vector<HookInfo> result;
        result.reserve(_hooks.size());
        for (const Entry& entry : _hooks)
            result.push_back(HookInfo{entry.name, entry.owner, entry.target, entry.stolenBytes});
        return result;
    }

    // Returns the number of currently installed hooks.
    std::size_t HookRegistry::size() const
    {
        const std::lock_guard<std::mutex> lock(_mutex);
        return _hooks.size();
    }

    // Computes an FNV-1a checksum over the hook table contents.
    std::uint64_t HookRegistry::fingerprint() const
    {
        const std::lock_guard<std::mutex> lock(_mutex);

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
            hash ^= 0xFFULL;
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

    // Unpatches and removes a hook entry at the given index while holding the lock.
    void HookRegistry::unpatchAndForgetLocked(std::size_t index)
    {
        Entry& entry = _hooks[index];

        retireLocked(entry);

        if (!_backend.unpatch(entry.target)) {
            crabe::shared::Logger::getInstance().warning(
                "HookRegistry: the backend did not recognise 0x{:X} ('{}', owner '{}') on removal; "
                "dropping the record anyway.",
                entry.target, entry.name, entry.owner);
        }

        _hooks.erase(_hooks.begin() + static_cast<std::ptrdiff_t>(index));
    }

    // Publishes a hook entry to the crash attribution mirror if configured.
    void HookRegistry::publishLocked(Entry& entry)
    {
        if (_attribution != Attribution::PublishToCrashHandler)
            return;

        for (std::size_t index = 0; index < kMaxAttributedHooks; ++index) {
            MirrorSlot& slot = g_mirror[index];
            if (slot.live.load(std::memory_order_relaxed))
                continue;

            copyFixed(slot.owner, HookAttribution::kTextCapacity, entry.owner);
            copyFixed(slot.name, HookAttribution::kTextCapacity, entry.name);
            slot.target.store(entry.target, std::memory_order_relaxed);
            slot.stolenBytes.store(entry.stolenBytes, std::memory_order_relaxed);
            slot.live.store(true, std::memory_order_release);

            entry.mirrorSlot = index;
            return;
        }

        crabe::shared::Logger::getInstance().warning(
            "HookRegistry: the crash-attribution table is full at {} entries; '{}' will not be "
            "named in a crash report.",
            kMaxAttributedHooks, entry.name);
    }

    // Retires a hook entry from the crash attribution mirror.
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

    // Resolves which hook covers address using the crash attribution mirror.
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

    // Copies mirrored hook attributions up to capacity into out.
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

    // Clears all entries in the crash attribution mirror.
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

#if defined(MH_ALL_HOOKS)
#error "hook_registry.cpp is not backend-free: MinHook.h reached it transitively"
#endif
#if defined(_WINDOWS_) || defined(_INC_WINDOWS) || defined(_WINBASE_) || defined(_MINWINBASE_)
#error "hook_registry.cpp is not backend-free: a Win32 header reached it, so something in \
here is reaching for the operating system instead of leaving that to IHookBackend"
#endif
