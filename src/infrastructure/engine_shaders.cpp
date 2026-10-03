/*
** CrabeLoader
** File description:
** Implements safe shader fallback hooking to prevent assertion failures on missing shader hashes.
** Intercepts Shader_LookupCrc and provides a default fallback shader if the CRC is not found.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/engine_shaders.hpp"

#include <windows.h>

#include "domain/game_profile.hpp"
#include "infrastructure/memory.hpp"
#include "shared/logger.hpp"

namespace {

    // 0x4509D0 in DisneyInfinity3.exe (rva 0x000509D0 in di3-gold-steam-1.0):
    // push ebx; push ebp; push esi; mov esi, [esp+0x18]; push edi; xor edi, edi; dec esi
    constexpr const char* kLookupPattern = "53 55 56 8B 74 24 18 57 33 FF 4E";
    constexpr const char* kLookupSymbol = "Shader_LookupCrc";

    // 0x45D5C0 in DisneyInfinity3.exe (rva 0x0005D5C0 in di3-gold-steam-1.0):
    // mov eax, [esp+4]; sub esp, 8; push ebx; xor ebx, ebx; cmp byte ptr [eax+1], bl
    constexpr const char* kParseBindingsPattern = "8B 44 24 04 83 EC 08 53 33 DB 38 58 01";
    constexpr const char* kParseBindingsSymbol = "Shader_ParseBindings";

    bool matchesProfile(const crabe::domain::GameProfile* profile, std::uintptr_t base,
                        const char* symbol, std::uintptr_t address)
    {
        if (!profile)
            return true;
        const std::uint32_t rva = profile->engineSymbolRva(symbol);
        return rva != crabe::domain::kUnmeasured && base + rva == address;
    }

} // namespace

namespace crabe::infrastructure {

EngineShaders& EngineShaders::get()
{
    static EngineShaders instance;
    return instance;
}

bool EngineShaders::resolve()
{
    const HMODULE hGame = GetModuleHandleA(nullptr);
    if (!hGame)
        return false;

    const auto base = reinterpret_cast<std::uintptr_t>(hGame);
    const auto* profile = crabe::domain::activeProfile();

    if (profile) {
        const std::uint32_t rvaLookup = profile->engineSymbolRva(kLookupSymbol);
        if (rvaLookup != crabe::domain::kUnmeasured) {
            _lookupTarget = base + rvaLookup;
        }
        const std::uint32_t rvaParse = profile->engineSymbolRva(kParseBindingsSymbol);
        if (rvaParse != crabe::domain::kUnmeasured) {
            _parseBindingsTarget = base + rvaParse;
        }
    }

    if (_lookupTarget == 0) {
        const std::uintptr_t match = crabe::memory::patternScan(kLookupPattern, hGame);
        if (match != 0 && matchesProfile(profile, base, kLookupSymbol, match)) {
            _lookupTarget = match;
        }
    }

    if (_parseBindingsTarget == 0) {
        const std::uintptr_t match = crabe::memory::patternScan(kParseBindingsPattern, hGame);
        if (match != 0 && matchesProfile(profile, base, kParseBindingsSymbol, match)) {
            _parseBindingsTarget = match;
        }
    }

    return _lookupTarget != 0;
}

void EngineShaders::initialize()
{
    if (!_attempted) {
        _attempted = true;
        resolve();
    }

    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

    if (_lookupTarget != 0 && !_lookupHook.isInstalled()) {
        if (!_lookupHook.install(reinterpret_cast<void*>(_lookupTarget),
                                 reinterpret_cast<void*>(&EngineShaders::hkLookupShaderCrc),
                                 "EngineShaders::LookupShaderCrc")) {
            logger.warning("EngineShaders: failed to install shader lookup hook; missing shaders may cause asserts.");
        } else {
            logger.info("EngineShaders: shader fallback hook installed at 0x{:X}.", _lookupTarget);
        }
    }

    if (_parseBindingsTarget != 0 && !_parseBindingsHook.isInstalled()) {
        if (!_parseBindingsHook.install(reinterpret_cast<void*>(_parseBindingsTarget),
                                        reinterpret_cast<void*>(&EngineShaders::hkParseBindings),
                                        "EngineShaders::ParseBindings")) {
            logger.warning("EngineShaders: failed to install material binding guard hook.");
        } else {
            logger.info("EngineShaders: material binding guard hook installed at 0x{:X}.", _parseBindingsTarget);
        }
    }
}

void EngineShaders::uninitialize()
{
    _lookupHook.remove();
    _parseBindingsHook.remove();
}

bool EngineShaders::isHooked() const noexcept
{
    return _lookupHook.isInstalled();
}

std::uint32_t EngineShaders::fallbackCount() const noexcept
{
    return s_fallbackCount.load(std::memory_order_relaxed);
}

void* __cdecl EngineShaders::hkLookupShaderCrc(std::uint32_t targetCrc,
                                               const ShaderEntry* array,
                                               std::uint32_t count)
{
    if (array != nullptr && count > 0) {
        int low = 0;
        int high = static_cast<int>(count) - 1;
        while (low <= high) {
            const int mid = (low + high) / 2;
            const std::uint32_t midCrc = array[mid].crc;
            if (midCrc < targetCrc) {
                low = mid + 1;
            } else if (midCrc > targetCrc) {
                high = mid - 1;
            } else {
                return array[mid].shader;
            }
        }
    }

    s_fallbackCount.fetch_add(1, std::memory_order_relaxed);

    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
    logger.warning("EngineShaders: shader CRC 0x{:08X} not found in engine table (total={}); substituting fallback shader.",
                   targetCrc, count);

    return findFallbackShader(array, count);
}

namespace {

bool isSafeBindingObject(void** table, std::uint32_t index) noexcept
{
    if (index >= 2048)
        return false;

    __try {
        void* obj = table[index];
        if (obj == nullptr)
            return true;

        if ((reinterpret_cast<std::uintptr_t>(obj) & 3) != 0)
            return false;

        auto* pRef = reinterpret_cast<volatile LONG*>(reinterpret_cast<std::uintptr_t>(obj) + 4);
        const LONG val = *pRef;
        if (val < 0 || val > 10000000)
            return false;

        InterlockedCompareExchange(pRef, val, val);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

} // namespace

void __cdecl EngineShaders::hkParseBindings(void* descriptor, void* stream, void** table)
{
    if (descriptor != nullptr && stream != nullptr && table != nullptr) {
        const auto* desc = reinterpret_cast<const std::uint8_t*>(descriptor);
        const std::uint8_t count = desc[1];
        auto* entries = reinterpret_cast<std::uint32_t*>(stream);

        for (std::uint8_t i = 0; i < count; ++i) {
            const std::uint32_t index = entries[i * 2];
            const std::uint32_t flags = entries[i * 2 + 1];
            if ((index & flags) != 0xFFFFFFFF) {
                if (!isSafeBindingObject(table, index)) {
                    entries[i * 2] = 0xFFFFFFFF;
                    entries[i * 2 + 1] = 0xFFFFFFFF;
                }
            }
        }
    }

    auto orig = reinterpret_cast<void(__cdecl*)(void*, void*, void**)>(
        get()._parseBindingsHook.getOriginal());
    if (orig != nullptr) {
        orig(descriptor, stream, table);
    }
}

void* EngineShaders::findFallbackShader(const ShaderEntry* array, std::uint32_t count) noexcept
{
    if (array == nullptr || count == 0)
        return nullptr;

    // Primary search: scan sorted shader array for a zero-parameter shader (e.g. solidcolor, error).
    // Zero-parameter shaders are guaranteed to skip the material parameter loop (0x45D5C0),
    // preventing any buffer desynchronization or access violation.
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto* sh = reinterpret_cast<const std::uint8_t*>(array[i].shader);
        if (sh != nullptr) {
            const std::uint8_t paramCount = sh[0xA1];
            const std::uint16_t paramSize = *reinterpret_cast<const std::uint16_t*>(&sh[0xA2]);
            if (paramCount == 0 && paramSize == 0) {
                return array[i].shader;
            }
        }
    }

    // Fallback to array[0].shader if none found
    return array[0].shader;
}

} // namespace crabe::infrastructure
