/*
** CrabeLoader
** File description:
** Guards Meridian graph array operations against corrupted or uninitialized heap pointers.
** Prevents fatal EXCEPTION_ACCESS_VIOLATION when loading node graphs from console updates.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/engine_meridian.hpp"

#include <windows.h>

#include "domain/game_profile.hpp"
#include "infrastructure/memory.hpp"
#include "shared/logger.hpp"

namespace crabe::infrastructure {

namespace {

    constexpr const char* kAppendSymbol = "Meridian_DynArrayAppend";
    // 56 8B F1 8B 06 85 C0 74 06 0F B7 48 02
    constexpr const char* kAppendPattern = "56 8B F1 8B 06 85 C0 74 06 0F B7 48 02";

    bool matchesProfile(const crabe::domain::GameProfile* profile,
                        std::uintptr_t base,
                        const char* symbol,
                        std::uintptr_t address) noexcept
    {
        if (profile == nullptr) return true;
        const std::uint32_t expectedRva = profile->engineSymbolRva(symbol);
        if (expectedRva == crabe::domain::kUnmeasured) return true;
        return address == (base + expectedRva);
    }

    bool isSafeBuffer(void* buf) noexcept
    {
        if (buf == nullptr)
            return true;

        if ((reinterpret_cast<std::uintptr_t>(buf) & 1) != 0)
            return false;

        __try {
            const auto* pHeader = reinterpret_cast<const volatile std::uint16_t*>(buf);
            const std::uint16_t cap = pHeader[0];
            const std::uint16_t count = pHeader[1];

            // A valid DynArray has reasonable capacity and count <= capacity
            if (cap == 0 || cap > 32768 || count > cap)
                return false;

            // Probe start and end of buffer memory to verify readability and writability
            auto* pData = reinterpret_cast<volatile std::uint8_t*>(buf);
            const std::uint32_t totalBytes = 4 + static_cast<std::uint32_t>(cap) * sizeof(std::uint16_t);

            const std::uint8_t bStart = pData[0];
            pData[0] = bStart;

            const std::uint8_t bEnd = pData[totalBytes - 1];
            pData[totalBytes - 1] = bEnd;

            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

} // namespace

EngineMeridian& EngineMeridian::get()
{
    static EngineMeridian instance;
    return instance;
}

bool EngineMeridian::resolve()
{
    const HMODULE hGame = GetModuleHandleW(nullptr);
    const auto base = reinterpret_cast<std::uintptr_t>(hGame);
    const auto* profile = crabe::domain::activeProfile();


    if (profile != nullptr) {
        const std::uint32_t rva = profile->engineSymbolRva(kAppendSymbol);
        if (rva != crabe::domain::kUnmeasured) {
            _appendTarget = base + rva;
        }
    }

    if (_appendTarget == 0) {
        const std::uintptr_t match = crabe::memory::patternScan(kAppendPattern, hGame);
        if (match != 0 && matchesProfile(profile, base, kAppendSymbol, match)) {
            _appendTarget = match;
        }
    }

    return _appendTarget != 0;
}

void EngineMeridian::initialize()
{
    if (!_attempted) {
        _attempted = true;
        resolve();
    }

    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

    if (_appendTarget != 0 && !_appendHook.isInstalled()) {
        if (!_appendHook.install(reinterpret_cast<void*>(_appendTarget),
                                 reinterpret_cast<void*>(&EngineMeridian::hkDynArrayAppend),
                                 "EngineMeridian::DynArrayAppend")) {
            logger.warning("EngineMeridian: failed to install DynArray guard hook.");
        } else {
            logger.info("EngineMeridian: DynArray guard hook installed at 0x{:X}.", _appendTarget);
        }
    }
}

void EngineMeridian::uninitialize()
{
    _appendHook.remove();
}

bool EngineMeridian::isHooked() const noexcept
{
    return _appendHook.isInstalled();
}

std::uint32_t EngineMeridian::sanitizedCount() const noexcept
{
    return s_sanitizedCount.load(std::memory_order_relaxed);
}

void __fastcall EngineMeridian::hkDynArrayAppend(void* thisPtr, void* /*edxDummy*/, const std::uint16_t* pVal)
{
    if (thisPtr != nullptr) {
        auto** ppBuf = reinterpret_cast<void**>(thisPtr);
        void* buf = nullptr;
        bool safe = false;

        __try {
            buf = *ppBuf;
            safe = isSafeBuffer(buf);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            safe = false;
        }

        if (!safe) {
            s_sanitizedCount.fetch_add(1, std::memory_order_relaxed);
            crabe::shared::Logger::getInstance().warning(
                "EngineMeridian: sanitized corrupted DynArray buffer 0x{:08X} at 0x{:08X} to nullptr.",
                reinterpret_cast<std::uintptr_t>(buf),
                reinterpret_cast<std::uintptr_t>(thisPtr));
            *ppBuf = nullptr;
        }
    }

    auto orig = reinterpret_cast<void(__fastcall*)(void*, void*, const std::uint16_t*)>(
        get()._appendHook.getOriginal());
    if (orig != nullptr) {
        orig(thisPtr, nullptr, pVal);
    }
}

} // namespace crabe::infrastructure
