/*
** CrabeLoader
** File description:
** Resolves and calls LoopScenes::ActivateFreeCamera, the engine's free-camera state machine.
** Both addresses come from signatures, and under a matched profile they must land on its RVAs.
** Every call runs under the SEH guard, so a wrong address costs a log line, not the game.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/engine_free_camera.hpp"

#include <cstring>
#include <windows.h>

#include "domain/game_profile.hpp"
#include "infrastructure/crash_handler.hpp"
#include "infrastructure/memory.hpp"
#include "shared/logger.hpp"

namespace {

    // Prologue of LoopScenes::ActivateFreeCamera(int playerId, bool noNoControl),
    // a __thiscall ending in `ret 8`. The wildcard is the camera-scene hook it calls first.
    constexpr const char* kActivatePattern =
        "83 EC 08 55 56 8B 74 24 14 57 56 8B E9 FF 15 ?? ?? ?? ?? "
        "8B F8 83 C4 04 85 FF 74 1D 80 BF B0 00 00 00 00";

    // BaseLoop's own `s_Scenes.CreateFreeCameras(); s_Scenes.ActivateFreeCamera(0);`:
    // the first call reaches CreateFreeCameras, the `mov ecx, imm32` carries s_Scenes,
    // and the call after it must reach the prologue above.
    constexpr const char* kCallerPattern =
        "E8 ?? ?? ?? ?? 6A 00 6A 00 B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B CE 5E E9";
    constexpr std::uintptr_t kCallerCreateCallOffset = 0;
    constexpr std::uintptr_t kCallerScenesImmOffset = 10;
    constexpr std::uintptr_t kCallerCallOffset = 14;

    // CreateFreeCameras opens with `push esi; mov esi, [CameraScene::s_Head]`.
    constexpr std::uint8_t kCreatePrologue[] = { 0x56, 0x8B, 0x35 };

    constexpr const char* kActivateSymbol = "LoopScenes::ActivateFreeCamera";
    constexpr const char* kCreateSymbol = "LoopScenes::CreateFreeCameras";
    constexpr const char* kScenesSymbol = "BaseLoop::s_Scenes";

    using ActivateFreeCameraFn = bool(__thiscall*)(void* scenes, int playerId, bool noNoControl);
    using CreateFreeCamerasFn = void(__fastcall*)(void* scenes, void* edx);
    using GetCameraSceneFn = void*(__cdecl*)(int playerId);

    // Inside ActivateFreeCamera, measured on di3-gold-steam-1.0: the `call [imm32]`
    // reaching the camera-scene hook.
    constexpr std::uintptr_t kSceneHookSlotOffset = 0x0F;

    // Engine object layout, read off the same function and its callees.
    // DeleteFreeCameras ends in `mov dword ptr [ebx+0x54], 0` (rva 0x29FF3F).
    constexpr std::uintptr_t kScenesPropertySetId = 0x54;
    constexpr std::uintptr_t kSceneCurrentCamera = 0x1A8;
    constexpr std::uintptr_t kSceneFallbackCamera = 0x1B8;
    constexpr std::uintptr_t kCameraName = 0x18;

    constexpr const char* kFreeCamName = "FreeCam";

    template <typename T>
    T readAt(std::uintptr_t address)
    {
        return *reinterpret_cast<const T*>(address);
    }

    // True in degraded mode, where the scan is all there is; under a matched
    // profile, true only when the scan landed exactly on the measured RVA.
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

EngineFreeCamera& EngineFreeCamera::get()
{
    static EngineFreeCamera instance;
    return instance;
}

void EngineFreeCamera::initialize()
{
    if (!_attempted) {
        _attempted = true;
        resolve();
    }
    if (!_createFreeCameras || _createHook.isInstalled())
        return;

    if (!_createHook.install(reinterpret_cast<void*>(_createFreeCameras),
                             reinterpret_cast<void*>(&EngineFreeCamera::hkCreateFreeCameras),
                             "EngineFreeCamera::CreateFreeCameras")) {
        crabe::shared::Logger::getInstance().warning(
            "EngineFreeCamera: could not hook CreateFreeCameras; the free camera will not come back after a world change.");
    }
}

void EngineFreeCamera::uninitialize()
{
    _createHook.remove();
}

std::uint32_t EngineFreeCamera::sceneGeneration() const
{
    return _generation.load(std::memory_order_relaxed);
}

void __fastcall EngineFreeCamera::hkCreateFreeCameras(void* scenes, void* edx)
{
    EngineFreeCamera& self = get();
    auto* propertySetId = reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uintptr_t>(scenes) + kScenesPropertySetId);
    const std::uint32_t stale = *propertySetId;
    *propertySetId = 0;
    self._generation.fetch_add(1, std::memory_order_relaxed);

    if (const auto original = reinterpret_cast<CreateFreeCamerasFn>(self._createHook.getOriginal()))
        original(scenes, edx);

    crabe::shared::Logger::getInstance().debug(
        "EngineFreeCamera: camera scenes rebuilt; free-camera property set {} -> {}.",
        stale, *propertySetId);
}

bool EngineFreeCamera::resolve()
{
    auto& logger = crabe::shared::Logger::getInstance();
    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));

    const std::uintptr_t activate = crabe::memory::patternScan(kActivatePattern);
    const std::uintptr_t caller = crabe::memory::patternScan(kCallerPattern);
    if (!activate || !caller) {
        logger.error("EngineFreeCamera: signature not found (activate 0x{:X}, caller 0x{:X}); "
                     "the engine free camera is unavailable.", activate, caller);
        return false;
    }

    if (!crabe::memory::isReadable(caller + kCallerScenesImmOffset, sizeof(std::uint32_t))
        || crabe::memory::resolveCall(caller + kCallerCallOffset) != activate) {
        logger.error("EngineFreeCamera: the caller at rva 0x{:X} does not call rva 0x{:X}; refusing.",
                     caller - base, activate - base);
        return false;
    }

    const std::uintptr_t scenes = *reinterpret_cast<const std::uint32_t*>(caller + kCallerScenesImmOffset);
    const crabe::domain::GameProfile* profile = crabe::domain::activeProfile();
    if (!matchesProfile(profile, base, kActivateSymbol, activate)
        || !matchesProfile(profile, base, kScenesSymbol, scenes)) {
        logger.error("EngineFreeCamera: scan landed off profile '{}' (activate rva 0x{:X}, s_Scenes rva 0x{:X}); refusing.",
                     profile->id, activate - base, scenes - base);
        return false;
    }

    _activate = activate;
    _scenes = scenes;
    logger.info("EngineFreeCamera: resolved (activate rva 0x{:X}, s_Scenes rva 0x{:X}{}).",
                activate - base, scenes - base, profile ? "" : ", degraded: scan only");

    const bool hookCall = crabe::memory::isReadable(_activate + kSceneHookSlotOffset - 2, 6)
        && readAt<std::uint8_t>(_activate + kSceneHookSlotOffset - 2) == 0xFF
        && readAt<std::uint8_t>(_activate + kSceneHookSlotOffset - 1) == 0x15;
    if (hookCall)
        _sceneHookSlot = readAt<std::uint32_t>(_activate + kSceneHookSlotOffset);

    const std::uintptr_t create = crabe::memory::resolveCall(caller + kCallerCreateCallOffset);
    if (!create || !crabe::memory::isReadable(create, sizeof(kCreatePrologue))
        || std::memcmp(reinterpret_cast<const void*>(create), kCreatePrologue, sizeof(kCreatePrologue)) != 0
        || !matchesProfile(profile, base, kCreateSymbol, create)) {
        logger.warning("EngineFreeCamera: CreateFreeCameras not found (rva 0x{:X}); "
                       "the free camera will not come back after a world change.", create ? create - base : 0);
        return true;
    }
    _createFreeCameras = create;
    return true;
}

const char* EngineFreeCamera::currentCameraName(int playerId) const
{
    if (!_sceneHookSlot)
        return nullptr;
    const auto getScene = readAt<GetCameraSceneFn>(_sceneHookSlot);
    void* scene = getScene ? getScene(playerId) : nullptr;
    if (!scene)
        return nullptr;
    const auto sceneAddress = reinterpret_cast<std::uintptr_t>(scene);
    std::uintptr_t current = readAt<std::uintptr_t>(sceneAddress + kSceneCurrentCamera);
    if (!current)
        current = readAt<std::uintptr_t>(sceneAddress + kSceneFallbackCamera);
    return current ? reinterpret_cast<const char*>(current + kCameraName) : nullptr;
}

std::optional<bool> EngineFreeCamera::toggle(int playerId, bool skipNoControl)
{
    if (!_attempted) {
        _attempted = true;
        resolve();
    }
    if (!_activate || !_scenes)
        return std::nullopt;

    const auto activate = reinterpret_cast<ActivateFreeCameraFn>(_activate);
    void* scenes = reinterpret_cast<void*>(_scenes);
    bool active = false;

    const bool completed = CrashHandler::runGuarded(
        [&] {
            active = activate(scenes, playerId, skipNoControl);
            if (active) {
                const char* current = currentCameraName(playerId);
                if (current && std::strcmp(current, kFreeCamName) != 0) {
                    crabe::shared::Logger::getInstance().warning(
                        "EngineFreeCamera: the engine reports the free camera on, but player {}'s current camera is '{}'.",
                        playerId, current);
                }
            }
        },
        "EngineFreeCamera::toggle");
    if (!completed)
        return std::nullopt;
    return active;
}

} // namespace crabe::infrastructure
