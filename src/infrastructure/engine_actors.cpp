/*
** CrabeLoader
** File description:
** Implements actor creation, states, area damage and move speed through Script VM natives.
** Resolves CreateActor and DamageRadius pointers dynamically from script native entry points.
** Does not rely on Toy Box placer or Rumpus editor, operating directly through engine memory.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/engine_actors.hpp"

#include <windows.h>

#include "infrastructure/crash_handler.hpp"
#include "infrastructure/memory.hpp"
#include "infrastructure/script_natives.hpp"
#include "shared/logger.hpp"

namespace {

    using crabe::infrastructure::script_natives::bytesAt;
    using crabe::infrastructure::script_natives::readAt;
    constexpr const char* kCreateNative = "CreateFromInitString";
    constexpr const char* kCreateSymbol = "Script_CreateFromInitString";
    constexpr std::uintptr_t kConstructLeaOffset = 0x44;
    constexpr std::uintptr_t kConstructCallOffset = 0x48;
    constexpr std::uintptr_t kCreatorFlagsOffset = 0x59;
    constexpr std::uintptr_t kPositionSourceOffset = 0x5E;
    constexpr std::uintptr_t kCreateCallOffset = 0x78;
    constexpr std::uintptr_t kCreateCleanupOffset = 0x7F;
    constexpr std::uintptr_t kDestructLeaOffset = 0x9F;
    constexpr std::uintptr_t kDestructCallOffset = 0xAB;

    constexpr int kPositionSourceValues = 2;
    constexpr std::uint32_t kCreatorFlags = 3;

    constexpr std::uintptr_t kActorHandle = 0x04;

    constexpr std::size_t kParameterSetStorage = 64;

    constexpr const char* kAddStateNative = "AddActorStateByName";
    constexpr const char* kAddStateSymbol = "Script_AddActorStateByName";
    constexpr const char* kRemoveStateNative = "RemoveActorStateByName";
    constexpr const char* kRemoveStateSymbol = "Script_RemoveActorStateByName";
    constexpr const char* kTestStateNative = "TestActorStateByName";
    constexpr const char* kTestStateSymbol = "Script_TestActorStateByName";
    constexpr std::uintptr_t kStateManagerOffset = 0x0D;
    constexpr std::uintptr_t kStateCategoryOffset = 0x14;
    constexpr std::uintptr_t kStateGetBitMaskOffset = 0x19;
    constexpr std::uintptr_t kStateTypeOffset = 0x2B;
    constexpr std::uintptr_t kStateGetAgentDataOffset = 0x37;
    constexpr std::uintptr_t kStateApplyOffset = 0x46;
    constexpr std::uintptr_t kTestGetAgentDataOffset = 0x42;
    constexpr std::uintptr_t kTestMaskReadOffset = 0x4E;
    constexpr std::uintptr_t kTestFilterOffset = 0x57;
    constexpr std::uintptr_t kStateDataMask = 0x08;

    constexpr const char* kSpeedNative = "MovementSetCumulativeSpeedMultiplier";
    constexpr const char* kSpeedSymbol = "Script_MovementSetCumulativeSpeedMultiplier";
    constexpr std::uintptr_t kSpeedTypeOffset = 0x1F;
    constexpr std::uintptr_t kSpeedGetAgentDataOffset = 0x2C;
    constexpr std::uintptr_t kSpeedReadOffset = 0x39;
    constexpr std::uintptr_t kSpeedScaleOffset = 0x3E;
    constexpr std::uintptr_t kSpeedWriteOffset = 0x44;
    constexpr std::uintptr_t kCumulativeSpeedField = 0x78;
    constexpr float kMaxSpeedScale = 20.0f;

    constexpr const char* kDamageNative = "DamageRadiusExceptActor";
    constexpr const char* kDamageSymbol = "Script_DamageRadiusExceptActor";
    constexpr std::uintptr_t kDamagePopActorOffset = 0x0D;
    constexpr std::uintptr_t kDamageCallOffset = 0x86;
    constexpr std::uintptr_t kDamageCleanupOffset = 0x8B;
    constexpr std::uintptr_t kPopActorFromHandleOffset = 0x24;
    constexpr std::uint8_t kPopActorPrologue[] = {
        0x8B, 0x51, 0x20, 0x8B, 0x02, 0x48, 0x89, 0x02, 0x8B, 0x49, 0x0C,
    };
    constexpr float kCylinderHeight = 1.0f;

    using ConstructFn = void*(__thiscall*)(void* self, const char* initString);
    using DestructFn = void(__thiscall*)(void* self);
    using CreateActorFn = void*(__cdecl*)(void* parameters, const float* position, float heading,
                                           int positionSource, const void* worldOffsetTransform,
                                           void* containingChannel, const char* worldRootName,
                                           std::uint32_t creatorFlags, std::uint32_t agentHandle,
                                           void* creationData, void* callback, void* callbackContext);
    using GetBitMaskFn = const void*(__thiscall*)(void* manager, const char* category, const char* name);
    using GetAgentDataFn = void*(__cdecl*)(std::uint32_t agentHandle, std::uint32_t dataType);
    using ApplyStateFn = void(__thiscall*)(void* stateData, const void* mask);
    using FilterAnyFn = bool(__cdecl*)(std::uintptr_t state, const void* mask);
    using FromHandleFn = void*(__cdecl*)(std::int32_t actorHandle);
    using DamageRadiusFn = void(__cdecl*)(const float* center, float radius, float damage, const char* damageType,
                                          void* source, void* owner, void* invincible, void* callback,
                                          void* callbackData, std::int32_t useCylinder, float cylinderHeight);

} // namespace

namespace crabe::infrastructure {

EngineActors& EngineActors::get()
{
    static EngineActors instance;
    return instance;
}

void EngineActors::resolveOnce()
{
    if (_attempted)
        return;
    _attempted = true;

    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    const bool create = resolveCreate(base);
    const bool states = resolveStates(base);
    const bool damage = resolveDamage(base);
    const bool movement = resolveMovement(base);
    crabe::shared::Logger::getInstance().info(
        "EngineActors: actor creation {}, actor states {}, area damage {}, move speed {}.",
        create ? "resolved" : "unavailable", states ? "resolved" : "unavailable",
        damage ? "resolved" : "unavailable", movement ? "resolved" : "unavailable");
}

bool EngineActors::resolveCreate(std::uintptr_t base)
{
    auto& logger = crabe::shared::Logger::getInstance();
    const std::uintptr_t handler = script_natives::find(kCreateNative);
    if (!handler || !script_natives::matchesProfile(base, kCreateSymbol, handler)) {
        logger.error("EngineActors: native '{}' not found or off profile (rva 0x{:X}).",
                     kCreateNative, handler ? handler - base : 0);
        return false;
    }

    const std::uintptr_t construct = crabe::memory::resolveCall(handler + kConstructCallOffset);
    const std::uintptr_t create = crabe::memory::resolveCall(handler + kCreateCallOffset);
    const std::uintptr_t destruct = crabe::memory::resolveCall(handler + kDestructCallOffset);
    if (!bytesAt(handler + kConstructLeaOffset, { 0x8D, 0x4C, 0x24, 0x1C })
        || !bytesAt(handler + kCreatorFlagsOffset, { 0x6A, static_cast<std::uint8_t>(kCreatorFlags) })
        || !bytesAt(handler + kPositionSourceOffset, { 0x6A, static_cast<std::uint8_t>(kPositionSourceValues) })
        || !bytesAt(handler + kCreateCleanupOffset, { 0x83, 0xC4, 0x30 })
        || !bytesAt(handler + kDestructLeaOffset, { 0x8D, 0x4C, 0x24, 0x18 })
        || !construct || !create || !destruct) {
        logger.error("EngineActors: '{}' at rva 0x{:X} does not have the measured shape; refusing.",
                     kCreateNative, handler - base);
        return false;
    }

    _parameterSetConstruct = construct;
    _createActor = create;
    _parameterSetDestruct = destruct;
    return true;
}

// Resolves actor state native entry points and verifies consistency across natives.
bool EngineActors::resolveStates(std::uintptr_t base)
{
    auto& logger = crabe::shared::Logger::getInstance();
    const std::uintptr_t add = script_natives::find(kAddStateNative);
    const std::uintptr_t remove = script_natives::find(kRemoveStateNative);
    const std::uintptr_t test = script_natives::find(kTestStateNative);
    if (!add || !remove || !test || !script_natives::matchesProfile(base, kAddStateSymbol, add)
        || !script_natives::matchesProfile(base, kRemoveStateSymbol, remove)
        || !script_natives::matchesProfile(base, kTestStateSymbol, test)) {
        logger.error("EngineActors: actor state natives not found or off profile.");
        return false;
    }

    const auto sameShape = [](std::uintptr_t handler) {
        return bytesAt(handler + kStateManagerOffset, { 0x8B, 0x0D })
            && bytesAt(handler + kStateCategoryOffset, { 0x68 })
            && bytesAt(handler + kStateTypeOffset, { 0x0F, 0xB7, 0x0D });
    };
    const std::uintptr_t getBitMask = crabe::memory::resolveCall(add + kStateGetBitMaskOffset);
    const std::uintptr_t getAgentData = crabe::memory::resolveCall(add + kStateGetAgentDataOffset);
    const std::uintptr_t addState = crabe::memory::resolveCall(add + kStateApplyOffset);
    const std::uintptr_t removeState = crabe::memory::resolveCall(remove + kStateApplyOffset);
    const std::uintptr_t filterAny = crabe::memory::resolveCall(test + kTestFilterOffset);
    const auto field = [](std::uintptr_t handler, std::uintptr_t offset, std::uintptr_t skip) {
        return script_natives::readAt<std::uint32_t>(handler + offset + skip);
    };
    if (!sameShape(add) || !sameShape(remove)
        || field(remove, kStateManagerOffset, 2) != field(add, kStateManagerOffset, 2)
        || field(remove, kStateCategoryOffset, 1) != field(add, kStateCategoryOffset, 1)
        || field(remove, kStateTypeOffset, 3) != field(add, kStateTypeOffset, 3)
        || crabe::memory::resolveCall(remove + kStateGetAgentDataOffset) != getAgentData
        || crabe::memory::resolveCall(test + kTestGetAgentDataOffset) != getAgentData
        || !bytesAt(test + kTestMaskReadOffset, { 0x8B, 0x40, static_cast<std::uint8_t>(kStateDataMask) })
        || !getBitMask || !getAgentData || !addState || !removeState || !filterAny) {
        logger.error("EngineActors: actor state natives do not have the measured shape; refusing.");
        return false;
    }

    _bitMaskManager = field(add, kStateManagerOffset, 2);
    _actorStateCategory = field(add, kStateCategoryOffset, 1);
    _stateDataType = field(add, kStateTypeOffset, 3);
    _getBitMask = getBitMask;
    _getAgentData = getAgentData;
    _addState = addState;
    _removeState = removeState;
    _filterAny = filterAny;
    return true;
}

bool EngineActors::resolveDamage(std::uintptr_t base)
{
    auto& logger = crabe::shared::Logger::getInstance();
    const std::uintptr_t handler = script_natives::find(kDamageNative);
    if (!handler || !script_natives::matchesProfile(base, kDamageSymbol, handler)) {
        logger.error("EngineActors: native '{}' not found or off profile (rva 0x{:X}).",
                     kDamageNative, handler ? handler - base : 0);
        return false;
    }

    const std::uintptr_t popActor = crabe::memory::resolveCall(handler + kDamagePopActorOffset);
    const std::uintptr_t damage = crabe::memory::resolveCall(handler + kDamageCallOffset);
    if (!popActor || !damage
        || !bytesAt(handler + kDamagePopActorOffset - 2, { 0x8B, 0xCE, 0xE8 })
        || !bytesAt(handler + kDamageCallOffset, { 0xE8 })
        || !bytesAt(handler + kDamageCleanupOffset, { 0x83, 0xC4, 0x2C })
        || !crabe::memory::isReadable(popActor, kPopActorFromHandleOffset + 5)
        || std::memcmp(reinterpret_cast<const void*>(popActor), kPopActorPrologue, sizeof(kPopActorPrologue)) != 0
        || !bytesAt(popActor + kPopActorFromHandleOffset, { 0xE8 })) {
        logger.error("EngineActors: '{}' at rva 0x{:X} does not have the measured shape; refusing.",
                     kDamageNative, handler - base);
        return false;
    }

    const std::uintptr_t fromHandle = crabe::memory::resolveCall(popActor + kPopActorFromHandleOffset);
    if (!fromHandle)
        return false;

    _damageRadius = damage;
    _actorFromHandle = fromHandle;
    return true;
}

bool EngineActors::resolveMovement(std::uintptr_t base)
{
    auto& logger = crabe::shared::Logger::getInstance();
    const std::uintptr_t handler = script_natives::find(kSpeedNative);
    if (!handler || !script_natives::matchesProfile(base, kSpeedSymbol, handler)) {
        logger.error("EngineActors: native '{}' not found or off profile (rva 0x{:X}).",
                     kSpeedNative, handler ? handler - base : 0);
        return false;
    }

    const std::uintptr_t getAgentData = crabe::memory::resolveCall(handler + kSpeedGetAgentDataOffset);
    if (!getAgentData
        || !bytesAt(handler + kSpeedTypeOffset, { 0x0F, 0xB7, 0x0D })
        || !bytesAt(handler + kSpeedGetAgentDataOffset, { 0xE8 })
        || !bytesAt(handler + kSpeedReadOffset, { 0xF3, 0x0F, 0x10, 0x40, static_cast<std::uint8_t>(kCumulativeSpeedField) })
        || !bytesAt(handler + kSpeedScaleOffset, { 0xF3, 0x0F, 0x59, 0x44, 0x24, 0x04 })
        || !bytesAt(handler + kSpeedWriteOffset, { 0xF3, 0x0F, 0x11, 0x40, static_cast<std::uint8_t>(kCumulativeSpeedField) })) {
        logger.error("EngineActors: '{}' at rva 0x{:X} does not have the measured shape; refusing.",
                     kSpeedNative, handler - base);
        return false;
    }

    _movementDataType = script_natives::readAt<std::uint32_t>(handler + kSpeedTypeOffset + 3);
    _movementGetAgentData = getAgentData;
    return true;
}

const void* EngineActors::bitMask(const std::string& state) const
{
    void* manager = script_natives::readAt<void*>(_bitMaskManager);
    if (!manager)
        return nullptr;
    return reinterpret_cast<GetBitMaskFn>(_getBitMask)(
        manager, reinterpret_cast<const char*>(_actorStateCategory), state.c_str());
}

void* EngineActors::stateData(std::uint32_t actorHandle) const
{
    const std::uint32_t type = script_natives::readAt<std::uint16_t>(_stateDataType);
    return reinterpret_cast<GetAgentDataFn>(_getAgentData)(actorHandle, type);
}

std::uint32_t EngineActors::createActor(const std::string& parameters, const Position& position, float heading)
{
    resolveOnce();
    if (!_createActor || parameters.empty())
        return 0;

    std::uint32_t handle = 0;
    const bool completed = CrashHandler::runGuarded(
        [&] {
            alignas(16) std::uint8_t parameterSet[kParameterSetStorage] = {};
            alignas(16) const float padded[4] = { position[0], position[1], position[2], 0.0f };

            reinterpret_cast<ConstructFn>(_parameterSetConstruct)(parameterSet, parameters.c_str());
            void* actor = reinterpret_cast<CreateActorFn>(_createActor)(
                parameterSet, padded, heading, kPositionSourceValues, nullptr, nullptr, nullptr,
                kCreatorFlags, 0, nullptr, nullptr, nullptr);
            reinterpret_cast<DestructFn>(_parameterSetDestruct)(parameterSet);

            if (actor)
                handle = readAt<std::uint32_t>(reinterpret_cast<std::uintptr_t>(actor) + kActorHandle);
        },
        "EngineActors::createActor");

    auto& logger = crabe::shared::Logger::getInstance();
    if (!completed)
        logger.error("EngineActors: CreateActor faulted for '{}'.", parameters);
    else if (!handle)
        logger.warning("EngineActors: the engine built no actor for '{}'.", parameters);
    return completed ? handle : 0;
}

bool EngineActors::setActorState(std::uint32_t actorHandle, const std::string& state, bool on)
{
    resolveOnce();
    if (!_addState || !actorHandle || state.empty())
        return false;

    bool applied = false;
    const bool completed = CrashHandler::runGuarded(
        [&] {
            const void* mask = bitMask(state);
            void* data = stateData(actorHandle);
            if (!mask || !data)
                return;
            reinterpret_cast<ApplyStateFn>(on ? _addState : _removeState)(data, mask);
            applied = true;
        },
        "EngineActors::setActorState");
    return completed && applied;
}

std::optional<bool> EngineActors::testActorState(std::uint32_t actorHandle, const std::string& state)
{
    resolveOnce();
    if (!_filterAny || !actorHandle || state.empty())
        return std::nullopt;

    std::optional<bool> result;
    const bool completed = CrashHandler::runGuarded(
        [&] {
            const void* mask = bitMask(state);
            void* data = stateData(actorHandle);
            if (!mask || !data)
                return;
            const auto current = script_natives::readAt<std::uintptr_t>(
                reinterpret_cast<std::uintptr_t>(data) + kStateDataMask);
            result = reinterpret_cast<FilterAnyFn>(_filterAny)(current, mask);
        },
        "EngineActors::testActorState");
    return completed ? result : std::nullopt;
}

bool EngineActors::setMoveSpeedScale(std::uint32_t actorHandle, float factor)
{
    resolveOnce();
    if (!_movementGetAgentData || !actorHandle || !(factor > 0.0f) || factor > kMaxSpeedScale)
        return false;

    bool applied = false;
    const bool completed = CrashHandler::runGuarded(
        [&] {
            const std::uint32_t type = script_natives::readAt<std::uint16_t>(_movementDataType);
            void* data = reinterpret_cast<GetAgentDataFn>(_movementGetAgentData)(actorHandle, type);
            if (!data)
                return;
            *reinterpret_cast<float*>(reinterpret_cast<std::uintptr_t>(data) + kCumulativeSpeedField) = factor;
            applied = true;
        },
        "EngineActors::setMoveSpeedScale");
    return completed && applied;
}

std::optional<float> EngineActors::moveSpeedScale(std::uint32_t actorHandle)
{
    resolveOnce();
    if (!_movementGetAgentData || !actorHandle)
        return std::nullopt;

    std::optional<float> result;
    const bool completed = CrashHandler::runGuarded(
        [&] {
            const std::uint32_t type = script_natives::readAt<std::uint16_t>(_movementDataType);
            void* data = reinterpret_cast<GetAgentDataFn>(_movementGetAgentData)(actorHandle, type);
            if (!data)
                return;
            result = script_natives::readAt<float>(reinterpret_cast<std::uintptr_t>(data) + kCumulativeSpeedField);
        },
        "EngineActors::moveSpeedScale");
    return completed ? result : std::nullopt;
}

bool EngineActors::damageRadius(const Position& center, float radius, float damage,
                                const std::string& damageType, std::uint32_t exceptActorHandle)
{
    resolveOnce();
    if (!_damageRadius || radius <= 0.0f || damage <= 0.0f || damageType.empty())
        return false;

    const bool completed = CrashHandler::runGuarded(
        [&] {
            alignas(16) const float padded[4] = { center[0], center[1], center[2], 0.0f };
            void* spared = exceptActorHandle
                ? reinterpret_cast<FromHandleFn>(_actorFromHandle)(static_cast<std::int32_t>(exceptActorHandle))
                : nullptr;
            reinterpret_cast<DamageRadiusFn>(_damageRadius)(padded, radius, damage, damageType.c_str(), nullptr,
                                                             nullptr, spared, nullptr, nullptr, 0, kCylinderHeight);
        },
        "EngineActors::damageRadius");

    if (!completed)
        crabe::shared::Logger::getInstance().error("EngineActors: DamageRadius faulted (radius {}, damage {}).",
                                                   radius, damage);
    return completed;
}

} // namespace crabe::infrastructure
