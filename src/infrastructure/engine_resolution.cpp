/*
** CrabeLoader
** File description:
** Hooks the engine's two display-resolution checks so 4K UHD, ultrawide, 16:10 and Steam Deck
** modes pass instead of being filtered out and clamped to 2560x1440 at boot, and recentres the
** 16:9 Scaleform UI on any other aspect. On a 16:9 mode every detour defers to the original.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/engine_resolution.hpp"

#include <algorithm>
#include <span>
#include <windows.h>

#include "domain/game_profile.hpp"
#include "infrastructure/memory.hpp"
#include "shared/logger.hpp"

namespace {

    // IsResolutionSupported (rva 0x459B0 in di3-gold-steam-1.0), __cdecl(width, height):
    //   mov ecx, [resolution_count]; xor eax, eax; push esi; test ecx, ecx; jle ...; mov edx, [esp+0xC]
    // The count's address is absolute, so it is wildcarded: it moves if the image is rebased.
    constexpr const char* kSupportedSymbol = "Resolution_IsSupported";
    constexpr const char* kSupportedPattern = "8B 0D ?? ?? ?? ?? 33 C0 56 85 C9 7E 1F 8B 54 24";
    constexpr std::size_t kSupportedLength = 16;

    // ResolutionList::Contains (rva 0x65A010), __thiscall(this, const uint32_t*, const uint32_t*),
    // ret 8. It walks the list the enumerator built, comparing each entry to the two values:
    //   push ebx; push esi; push edi; call <vector size>; xor edx, edx; test eax, eax; jle ...;
    //   mov esi, [esp+0x14]; mov esi, [esi]
    // The call displacement is wildcarded for the same reason.
    constexpr const char* kContainsSymbol = "ResolutionList_Contains";
    constexpr const char* kContainsPattern = "53 56 57 E8 ?? ?? ?? ?? 33 D2 85 C0 7E 25 8B 74 24 14 8B 36";
    constexpr std::size_t kContainsLength = 20;

    // Flash::Movie::SetViewport (rva 0x3F3C20), __thiscall(this, float x, float y, float width,
    // float height), ret 0x10. Width and height are fractions of r_defaultTarget()'s viewport:
    //   sub esp, 0x34; push ebx; push edi; mov ebx, ecx; xor edi, edi; cmp [ebx+0x3C], edi; je ...;
    //   push esi; call r_defaultTarget; movsx esi, word [eax+0x60]; call r_defaultTarget;
    //   movsx eax, word [eax+0x62]
    constexpr const char* kMovieViewportSymbol = "Flash_Movie_SetViewport";
    constexpr const char* kMovieViewportPattern =
        "83 EC 34 53 57 8B D9 33 FF 39 7B 3C 0F 84 ?? ?? ?? ?? 56 E8 ?? ?? ?? ?? 0F BF 70 60 E8 ?? ?? ?? ?? 0F BF 40 62";
    constexpr std::size_t kMovieViewportLength = 37;
    constexpr std::uintptr_t kDefaultTargetCallOffset = 0x13;
    // R_Target::viewport.w / .h, as the function itself reads them (16-bit).
    constexpr std::size_t kTargetViewportWidthOffset = 0x60;
    constexpr std::size_t kTargetViewportHeightOffset = 0x62;
    // The aspect every Scaleform screen was authored for.
    constexpr float kUiAspect = 16.0f / 9.0f;
    constexpr float kUiAspectTolerance = 0.01f;

    // Smallest side and largest side a mode may have, whichever of the two arguments is which.
    constexpr std::uint32_t kMinSide = 480;
    constexpr std::uint32_t kMaxSide = 7680;
    // 32:9 super ultrawide is 3.56; anything beyond 4 is a mis-read, not a monitor.
    constexpr std::uint32_t kMaxAspectNumerator = 4;

} // namespace

namespace crabe::infrastructure {

EngineResolution& EngineResolution::get()
{
    static EngineResolution instance;
    return instance;
}

bool EngineResolution::isPlausibleMode(std::uint32_t first, std::uint32_t second) noexcept
{
    const std::uint32_t small = std::min(first, second);
    const std::uint32_t large = std::max(first, second);
    return small >= kMinSide && large <= kMaxSide && large <= small * kMaxAspectNumerator;
}

std::uintptr_t EngineResolution::resolve(const char* symbol, std::uintptr_t base,
                                         const char* pattern, std::size_t length)
{
    const auto* profile = crabe::domain::activeProfile();

    std::uintptr_t address = 0;
    if (profile) {
        // A matched profile that does not carry the symbol is a refusal, never a guess.
        const std::uint32_t rva = profile->engineSymbolRva(symbol);
        if (rva == crabe::domain::kUnmeasured)
            return 0;
        address = base + rva;
    } else {
        address = crabe::memory::patternScan(pattern, reinterpret_cast<HMODULE>(base));
    }

    // Whichever way the address was found, the bytes under it have the last word.
    if (address == 0 || !crabe::memory::isReadable(address, length))
        return 0;
    const std::span<const std::uint8_t> bytes(reinterpret_cast<const std::uint8_t*>(address), length);
    if (crabe::memory::findPattern(bytes, pattern) != 0)
        return 0;
    return address;
}

void EngineResolution::initialize()
{
    if (_attempted)
        return;
    _attempted = true;

    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    if (!base)
        return;

    const std::uintptr_t supported = resolve(kSupportedSymbol, base, kSupportedPattern, kSupportedLength);
    const std::uintptr_t contains = resolve(kContainsSymbol, base, kContainsPattern, kContainsLength);

    // Widening only one of the two is worse than neither: the enumerator would list a mode
    // the boot check then rejects and clamps, rewriting the saved resolution as before.
    if (!supported || !contains) {
        logger.warning("EngineResolution: resolution checks not found where expected "
                       "(supported=0x{:X}, contains=0x{:X}); the engine keeps its nine resolutions.",
                       supported, contains);
        return;
    }

    if (!_supportedHook.install(reinterpret_cast<void*>(supported),
                                reinterpret_cast<void*>(&EngineResolution::hkIsResolutionSupported),
                                "EngineResolution::IsResolutionSupported")) {
        logger.warning("EngineResolution: failed to hook IsResolutionSupported at rva 0x{:X}.", supported - base);
        return;
    }
    if (!_containsHook.install(reinterpret_cast<void*>(contains),
                               reinterpret_cast<void*>(&EngineResolution::hkResolutionListContains),
                               "EngineResolution::ResolutionListContains")) {
        logger.warning("EngineResolution: failed to hook ResolutionList::Contains at rva 0x{:X}.", contains - base);
        _supportedHook.remove();
        return;
    }

    logger.info("EngineResolution: display resolution checks widened (rva 0x{:X}, 0x{:X}).",
                supported - base, contains - base);

    // Independent of the two checks above: without it a wide mode still works, with the UI
    // pushed to the right, so a failure here is reported and nothing else is undone.
    const std::uintptr_t movieViewport =
        resolve(kMovieViewportSymbol, base, kMovieViewportPattern, kMovieViewportLength);
    const std::uintptr_t defaultTarget =
        movieViewport ? crabe::memory::resolveCall(movieViewport + kDefaultTargetCallOffset) : 0;
    if (!movieViewport || !defaultTarget) {
        logger.warning("EngineResolution: Flash::Movie::SetViewport not found where expected; "
                       "the UI is not recentred on non-16:9 screens.");
        return;
    }
    _defaultTarget = reinterpret_cast<void* (__cdecl*)()>(defaultTarget);
    if (!_movieViewportHook.install(reinterpret_cast<void*>(movieViewport),
                                    reinterpret_cast<void*>(&EngineResolution::hkMovieSetViewport),
                                    "EngineResolution::MovieSetViewport")) {
        logger.warning("EngineResolution: failed to hook Flash::Movie::SetViewport at rva 0x{:X}.",
                       movieViewport - base);
        return;
    }
    logger.info("EngineResolution: Scaleform UI recentred to 16:9 on wider or taller screens (rva 0x{:X}).",
                movieViewport - base);
}

void EngineResolution::uninitialize()
{
    _movieViewportHook.remove();
    _containsHook.remove();
    _supportedHook.remove();
}

bool EngineResolution::isHooked() const noexcept
{
    return _supportedHook.isInstalled() && _containsHook.isInstalled();
}

bool __cdecl EngineResolution::hkIsResolutionSupported(std::uint32_t width, std::uint32_t height)
{
    if (isPlausibleMode(width, height))
        return true;

    using Fn = bool(__cdecl*)(std::uint32_t, std::uint32_t);
    const auto original = reinterpret_cast<Fn>(get()._supportedHook.getOriginal());
    return original != nullptr && original(width, height);
}

void __fastcall EngineResolution::hkMovieSetViewport(void* self, void* edx, float x, float y,
                                                     float width, float height)
{
    using Fn = void(__fastcall*)(void*, void*, float, float, float, float);
    EngineResolution& resolution = get();
    const auto original = reinterpret_cast<Fn>(resolution._movieViewportHook.getOriginal());

    // Only a movie covering the whole screen is recentred; a sub-rectangle was placed on purpose.
    const auto* target = (width == 1.0f && height == 1.0f && resolution._defaultTarget)
        ? static_cast<const std::uint8_t*>(resolution._defaultTarget())
        : nullptr;
    if (target) {
        const float screenWidth = *reinterpret_cast<const std::int16_t*>(target + kTargetViewportWidthOffset);
        const float screenHeight = *reinterpret_cast<const std::int16_t*>(target + kTargetViewportHeightOffset);
        if (screenWidth > 0.0f && screenHeight > 0.0f) {
            const float aspect = screenWidth / screenHeight;
            if (aspect > kUiAspect + kUiAspectTolerance) {
                const float uiWidth = screenHeight * kUiAspect;
                x += (screenWidth - uiWidth) * 0.5f;
                width = uiWidth / screenWidth;
            } else if (aspect < kUiAspect - kUiAspectTolerance) {
                const float uiHeight = screenWidth / kUiAspect;
                y += (screenHeight - uiHeight) * 0.5f;
                height = uiHeight / screenHeight;
            }
        }
    }

    if (original)
        original(self, edx, x, y, width, height);
}

bool __fastcall EngineResolution::hkResolutionListContains(void* self, void* edx,
                                                           const std::uint32_t* first,
                                                           const std::uint32_t* second)
{
    // The engine passes pointers to the two values; a null one is the engine's bug, so let
    // the original fault or fail as it always did rather than hide it.
    if (first && second && isPlausibleMode(*first, *second))
        return true;

    using Fn = bool(__fastcall*)(void*, void*, const std::uint32_t*, const std::uint32_t*);
    const auto original = reinterpret_cast<Fn>(get()._containsHook.getOriginal());
    return original != nullptr && original(self, edx, first, second);
}

} // namespace crabe::infrastructure
