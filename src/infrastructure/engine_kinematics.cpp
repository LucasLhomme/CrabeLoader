/*
** CrabeLoader
** File description:
** Resolves the engine's camera eye, actor position and actor placement from the script natives
** that use them. Each name leads to a handler (see script_natives.hpp) whose body is checked
** byte by byte; under a matched profile it must land on its RVA.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/engine_kinematics.hpp"

#include <windows.h>

#include "infrastructure/crash_handler.hpp"
#include "infrastructure/memory.hpp"
#include "infrastructure/script_natives.hpp"
#include "shared/logger.hpp"

namespace {

    using crabe::infrastructure::script_natives::bytesAt;
    using crabe::infrastructure::script_natives::readAt;

    // Script_KinematicStatePlaceWithPosition, measured on di3-gold-steam-1.0 (rva 0x14A23F0):
    //   +0x60  mov ecx, [g_agentStateManager]
    //   +0x6A  call AgentStateManager::Lookup(uint32_t* outIndex, const uint32_t* handle)
    //   +0x8B  cmp dx, [FullKinematicStateData::s_dataType]
    //   +0x9B  call FullKinematicStateData::Place(const Vector3*, uint32_t flags)
    constexpr const char* kPlaceNative = "KinematicStatePlaceWithPosition";
    constexpr const char* kPlaceSymbol = "Script_KinematicStatePlaceWithPosition";
    constexpr std::uintptr_t kPlaceManagerOffset = 0x60;
    constexpr std::uintptr_t kPlaceLookupCallOffset = 0x6A;
    constexpr std::uintptr_t kPlaceTypeOffset = 0x8B;
    constexpr std::uintptr_t kPlaceCallOffset = 0x9B;

    // Script_KinematicStateGetActualPosition (rva 0x1490200), the same lookup, then for a
    // full kinematic state:
    //   +0x52  test byte [state+0x51], 1     -- position stale
    //   +0x5A  call FullKinematicStateData::<refresh>
    //   +0x5F  lea eax, [state+0x268]         -- the actual position
    constexpr const char* kPositionNative = "KinematicStateGetActualPosition";
    constexpr const char* kPositionSymbol = "Script_KinematicStateGetActualPosition";
    constexpr std::uintptr_t kPositionManagerOffset = 0x26;
    constexpr std::uintptr_t kPositionLookupCallOffset = 0x2D;
    constexpr std::uintptr_t kPositionStaleTestOffset = 0x52;
    constexpr std::uintptr_t kPositionRefreshCallOffset = 0x5A;
    constexpr std::uintptr_t kPositionLeaOffset = 0x5F;

    // Script_CameraGetPosition (rva 0xF7A4C0): `mov ecx, [g_Players]`, the virtual
    // GetCamera(playerId, 0) at vtable +0x28, then Camera::m_Eye read at +0x3C.
    constexpr const char* kCameraNative = "CameraGetPosition";
    constexpr const char* kCameraSymbol = "Script_CameraGetPosition";
    constexpr std::uintptr_t kCameraPlayersOffset = 0x29;
    constexpr std::uintptr_t kCameraSlotOffset = 0x34;
    constexpr std::uintptr_t kCameraEyeReadOffset = 0x3D;

    constexpr std::uintptr_t kGetCameraSlot = 0x28;
    constexpr std::uintptr_t kCameraEye = 0x3C;
    constexpr std::uintptr_t kStateEntryData = 0x08;
    constexpr std::uintptr_t kStateDataType = 0x06;
    constexpr std::uint16_t kStateDataTypeMask = 0x3FF;
    constexpr std::uintptr_t kStateStaleFlags = 0x51;

    using LookupStateFn = void*(__thiscall*)(void* manager, std::uint32_t* outIndex, const std::uint32_t* handle);
    using PlaceFn = void(__thiscall*)(void* state, const float* position, std::uint32_t flags);
    using RefreshFn = void(__thiscall*)(void* state);
    using GetCameraFn = void*(__thiscall*)(void* players, int playerId, int flags);

} // namespace

namespace crabe::infrastructure {

EngineKinematics& EngineKinematics::get()
{
    static EngineKinematics instance;
    return instance;
}

void EngineKinematics::resolveOnce()
{
    if (_attempted)
        return;
    _attempted = true;

    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    const bool place = resolvePlace(base);
    const bool position = place && resolvePosition(base);
    const bool camera = resolveCamera(base);
    crabe::shared::Logger::getInstance().info("EngineKinematics: actor placement {}, actor position {}, camera eye {}.",
                                              place ? "resolved" : "unavailable",
                                              position ? "resolved" : "unavailable",
                                              camera ? "resolved" : "unavailable");
}

// The full kinematic state behind `actorHandle`, or 0. Shared by placement and
// position, which both reach it the way the script natives do.
std::uintptr_t EngineKinematics::fullKinematicState(std::uint32_t actorHandle) const
{
    void* manager = readAt<void*>(_agentStateManager);
    if (!manager)
        return 0;
    std::uint32_t index = 0;
    const auto entry = reinterpret_cast<std::uintptr_t>(
        reinterpret_cast<LookupStateFn>(_lookupState)(manager, &index, &actorHandle));
    if (!entry)
        return 0;
    const auto state = readAt<std::uintptr_t>(entry + kStateEntryData);
    if (!state)
        return 0;
    const std::uint16_t type = readAt<std::uint16_t>(state + kStateDataType) & kStateDataTypeMask;
    return type == readAt<std::uint16_t>(_fullKinematicType) ? state : 0;
}

bool EngineKinematics::resolvePlace(std::uintptr_t base)
{
    auto& logger = crabe::shared::Logger::getInstance();
    const std::uintptr_t handler = script_natives::find(kPlaceNative);
    if (!handler || !script_natives::matchesProfile(base, kPlaceSymbol, handler)) {
        logger.error("EngineKinematics: native '{}' not found or off profile (rva 0x{:X}).",
                     kPlaceNative, handler ? handler - base : 0);
        return false;
    }

    const std::uintptr_t lookup = crabe::memory::resolveCall(handler + kPlaceLookupCallOffset);
    const std::uintptr_t place = crabe::memory::resolveCall(handler + kPlaceCallOffset);
    if (!bytesAt(handler + kPlaceManagerOffset, { 0x8B, 0x0D })
        || !bytesAt(handler + kPlaceTypeOffset, { 0x66, 0x3B, 0x15 }) || !lookup || !place) {
        logger.error("EngineKinematics: '{}' at rva 0x{:X} does not have the measured shape; refusing.",
                     kPlaceNative, handler - base);
        return false;
    }

    _agentStateManager = readAt<std::uint32_t>(handler + kPlaceManagerOffset + 2);
    _fullKinematicType = readAt<std::uint32_t>(handler + kPlaceTypeOffset + 3);
    _lookupState = lookup;
    _place = place;
    return true;
}

// Needs resolvePlace first: it must use the same manager and lookup, which is
// checked rather than assumed.
bool EngineKinematics::resolvePosition(std::uintptr_t base)
{
    auto& logger = crabe::shared::Logger::getInstance();
    const std::uintptr_t handler = script_natives::find(kPositionNative);
    if (!handler || !script_natives::matchesProfile(base, kPositionSymbol, handler)) {
        logger.error("EngineKinematics: native '{}' not found or off profile (rva 0x{:X}).",
                     kPositionNative, handler ? handler - base : 0);
        return false;
    }

    const std::uintptr_t refresh = crabe::memory::resolveCall(handler + kPositionRefreshCallOffset);
    if (!bytesAt(handler + kPositionManagerOffset, { 0x8B, 0x0D })
        || readAt<std::uint32_t>(handler + kPositionManagerOffset + 2) != _agentStateManager
        || crabe::memory::resolveCall(handler + kPositionLookupCallOffset) != _lookupState
        || !bytesAt(handler + kPositionStaleTestOffset, { 0xF6, 0x46, static_cast<std::uint8_t>(kStateStaleFlags), 0x01 })
        || !bytesAt(handler + kPositionLeaOffset, { 0x8D, 0x86 }) || !refresh) {
        logger.error("EngineKinematics: '{}' at rva 0x{:X} does not have the measured shape; refusing.",
                     kPositionNative, handler - base);
        return false;
    }

    _refreshState = refresh;
    _statePosition = readAt<std::uint32_t>(handler + kPositionLeaOffset + 2);
    return true;
}

bool EngineKinematics::resolveCamera(std::uintptr_t base)
{
    auto& logger = crabe::shared::Logger::getInstance();
    const std::uintptr_t handler = script_natives::find(kCameraNative);
    if (!handler || !script_natives::matchesProfile(base, kCameraSymbol, handler)) {
        logger.error("EngineKinematics: native '{}' not found or off profile (rva 0x{:X}).",
                     kCameraNative, handler ? handler - base : 0);
        return false;
    }

    if (!bytesAt(handler + kCameraPlayersOffset, { 0x8B, 0x0D })
        || !bytesAt(handler + kCameraSlotOffset, { 0x8B, 0x42, static_cast<std::uint8_t>(kGetCameraSlot) })
        || !bytesAt(handler + kCameraEyeReadOffset, { 0xF3, 0x0F, 0x10, 0x40, static_cast<std::uint8_t>(kCameraEye) })) {
        logger.error("EngineKinematics: '{}' at rva 0x{:X} does not have the measured shape; refusing.",
                     kCameraNative, handler - base);
        return false;
    }

    _players = readAt<std::uint32_t>(handler + kCameraPlayersOffset + 2);
    return true;
}

std::optional<EngineKinematics::Position> EngineKinematics::cameraEye(int playerId)
{
    resolveOnce();
    if (!_players)
        return std::nullopt;

    std::optional<Position> eye;
    const bool completed = CrashHandler::runGuarded(
        [&] {
            void* players = readAt<void*>(_players);
            if (!players)
                return;
            const auto vtable = readAt<std::uintptr_t>(reinterpret_cast<std::uintptr_t>(players));
            const auto getCamera = readAt<GetCameraFn>(vtable + kGetCameraSlot);
            void* camera = getCamera(players, playerId, 0);
            if (!camera)
                return;
            const auto cameraAddress = reinterpret_cast<std::uintptr_t>(camera);
            eye = Position{ readAt<float>(cameraAddress + kCameraEye), readAt<float>(cameraAddress + kCameraEye + 4),
                            readAt<float>(cameraAddress + kCameraEye + 8) };
        },
        "EngineKinematics::cameraEye");
    return completed ? eye : std::nullopt;
}

std::optional<EngineKinematics::Position> EngineKinematics::actorPosition(std::uint32_t actorHandle)
{
    resolveOnce();
    if (!_statePosition || !actorHandle)
        return std::nullopt;

    std::optional<Position> position;
    const bool completed = CrashHandler::runGuarded(
        [&] {
            const std::uintptr_t state = fullKinematicState(actorHandle);
            if (!state)
                return;
            // A stale position is recomputed first, exactly as the script native does.
            if (readAt<std::uint8_t>(state + kStateStaleFlags) & 0x01)
                reinterpret_cast<RefreshFn>(_refreshState)(reinterpret_cast<void*>(state));
            const std::uintptr_t at = state + _statePosition;
            position = Position{ readAt<float>(at), readAt<float>(at + 4), readAt<float>(at + 8) };
        },
        "EngineKinematics::actorPosition");
    return completed ? position : std::nullopt;
}

bool EngineKinematics::placeActor(std::uint32_t actorHandle, const Position& position)
{
    resolveOnce();
    if (!_place || !actorHandle)
        return false;

    bool placed = false;
    const bool completed = CrashHandler::runGuarded(
        [&] {
            const std::uintptr_t state = fullKinematicState(actorHandle);
            if (!state)
                return;
            // Padded and aligned like the engine's Vector3d, in case Place loads it whole.
            alignas(16) const float padded[4] = { position[0], position[1], position[2], 0.0f };
            reinterpret_cast<PlaceFn>(_place)(reinterpret_cast<void*>(state), padded, 0);
            placed = true;
        },
        "EngineKinematics::placeActor");
    return completed && placed;
}

} // namespace crabe::infrastructure
