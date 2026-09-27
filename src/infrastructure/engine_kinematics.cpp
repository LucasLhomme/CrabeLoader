/*
** CrabeLoader
** File description:
** Resolves the engine's camera eye and actor placement from the script natives that use them.
** The natives register as `push handler; push "Name"; call RegisterFunction`, so each name leads
** to a handler whose body is checked byte by byte, and under a matched profile must land on its RVA.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/engine_kinematics.hpp"

#include <cstdio>
#include <cstring>
#include <windows.h>

#include "domain/game_profile.hpp"
#include "infrastructure/crash_handler.hpp"
#include "infrastructure/memory.hpp"
#include "shared/logger.hpp"

namespace {

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

    using LookupStateFn = void*(__thiscall*)(void* manager, std::uint32_t* outIndex, const std::uint32_t* handle);
    using PlaceFn = void(__thiscall*)(void* state, const float* position, std::uint32_t flags);
    using GetCameraFn = void*(__thiscall*)(void* players, int playerId, int flags);

    template <typename T>
    T readAt(std::uintptr_t address)
    {
        return *reinterpret_cast<const T*>(address);
    }

    bool bytesAt(std::uintptr_t address, std::initializer_list<std::uint8_t> expected)
    {
        if (!crabe::memory::isReadable(address, expected.size()))
            return false;
        return std::memcmp(reinterpret_cast<const void*>(address), expected.begin(), expected.size()) == 0;
    }

    // The handler registered under `name`: the `push imm32` right before `push "name"`.
    std::uintptr_t findScriptNative(const char* name)
    {
        const std::size_t length = std::strlen(name);
        for (std::uintptr_t text = crabe::memory::findString(name); text; text = crabe::memory::findString(name, text)) {
            if (!crabe::memory::isReadable(text - 1, length + 2) || readAt<char>(text - 1) != '\0')
                continue;

            char pattern[32];
            std::snprintf(pattern, sizeof(pattern), "68 %02X %02X %02X %02X",
                          static_cast<unsigned>(text & 0xFF), static_cast<unsigned>((text >> 8) & 0xFF),
                          static_cast<unsigned>((text >> 16) & 0xFF), static_cast<unsigned>((text >> 24) & 0xFF));
            for (std::uintptr_t push = crabe::memory::patternScan(pattern); push;
                 push = crabe::memory::patternScan(pattern, nullptr, push)) {
                if (readAt<std::uint8_t>(push - 5) == 0x68)
                    return readAt<std::uint32_t>(push - 4);
            }
        }
        return 0;
    }

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
    const bool camera = resolveCamera(base);
    crabe::shared::Logger::getInstance().info("EngineKinematics: actor placement {}, camera eye {}.",
                                              place ? "resolved" : "unavailable", camera ? "resolved" : "unavailable");
}

bool EngineKinematics::resolvePlace(std::uintptr_t base)
{
    auto& logger = crabe::shared::Logger::getInstance();
    const std::uintptr_t handler = findScriptNative(kPlaceNative);
    if (!handler || !matchesProfile(crabe::domain::activeProfile(), base, kPlaceSymbol, handler)) {
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

bool EngineKinematics::resolveCamera(std::uintptr_t base)
{
    auto& logger = crabe::shared::Logger::getInstance();
    const std::uintptr_t handler = findScriptNative(kCameraNative);
    if (!handler || !matchesProfile(crabe::domain::activeProfile(), base, kCameraSymbol, handler)) {
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

bool EngineKinematics::placeActor(std::uint32_t actorHandle, const Position& position)
{
    resolveOnce();
    if (!_place || !actorHandle)
        return false;

    bool placed = false;
    const bool completed = CrashHandler::runGuarded(
        [&] {
            void* manager = readAt<void*>(_agentStateManager);
            if (!manager)
                return;
            std::uint32_t index = 0;
            const auto entry = reinterpret_cast<std::uintptr_t>(
                reinterpret_cast<LookupStateFn>(_lookupState)(manager, &index, &actorHandle));
            if (!entry)
                return;
            const auto state = readAt<std::uintptr_t>(entry + kStateEntryData);
            if (!state)
                return;
            // Only a full kinematic state carries Place; the script native checks the same.
            const std::uint16_t type = readAt<std::uint16_t>(state + kStateDataType) & kStateDataTypeMask;
            if (type != readAt<std::uint16_t>(_fullKinematicType))
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
