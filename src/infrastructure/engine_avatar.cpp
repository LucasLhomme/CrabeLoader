/*
** CrabeLoader
** File description:
** Implements absolute avatar leveling by calling AvatarLeveling::LevelUp the way the Lua native
** Players_AvatarLevelUp does. The handler is found by its profile RVA and checked byte by byte
** before the singletons and the two callees are read out of it.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/engine_avatar.hpp"

#include <windows.h>

#include "domain/game_profile.hpp"
#include "infrastructure/crash_handler.hpp"
#include "infrastructure/memory.hpp"
#include "infrastructure/script_natives.hpp"
#include "shared/logger.hpp"

namespace {

    using crabe::infrastructure::script_natives::bytesAt;
    using crabe::infrastructure::script_natives::readAt;

    constexpr const char* kHandlerSymbol = "VirtualReaderSystem::Players_AvatarLevelUp";
    constexpr std::uintptr_t kPrologueOffset = 0x00;
    constexpr std::uintptr_t kFlowLoadOffset = 0x27;
    constexpr std::uintptr_t kFlowLevelLoadOffset = 0x53;
    constexpr std::uintptr_t kGetLevelCallOffset = 0x5A;
    constexpr std::uintptr_t kIncrementOffset = 0x5F;
    constexpr std::uintptr_t kLevelingLoadOffset = 0x6A;
    constexpr std::uintptr_t kLevelUpCallOffset = 0x74;
    constexpr std::uintptr_t kLoadOperandSkip = 2;
    constexpr int kMaxLevel = 20;

    using GetLevelFn = int(__thiscall*)(void* flow, int playerId);
    using LevelUpFn = void(__thiscall*)(void* leveling, int playerId, int level, std::uint32_t characterId,
                                        bool isCurrentAvatar, bool showUnlockUi);

} // namespace

namespace crabe::infrastructure {

EngineAvatar& EngineAvatar::get()
{
    static EngineAvatar instance;
    return instance;
}

void EngineAvatar::resolveOnce()
{
    if (_attempted)
        return;
    _attempted = true;

    auto& logger = crabe::shared::Logger::getInstance();
    const auto* profile = crabe::domain::activeProfile();
    const std::uint32_t rva = profile ? profile->engineSymbolRva(kHandlerSymbol) : crabe::domain::kUnmeasured;
    if (rva == crabe::domain::kUnmeasured) {
        logger.warning("EngineAvatar: no measured profile for the avatar level handler; level setting unavailable.");
        return;
    }

    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    const std::uintptr_t handler = base + rva;
    if (!bytesAt(handler + kPrologueOffset, { 0x83, 0xEC, 0x08, 0x53, 0x56, 0x57 })
        || !bytesAt(handler + kFlowLoadOffset, { 0x8B, 0x0D })
        || !bytesAt(handler + kFlowLevelLoadOffset, { 0x8B, 0x0D })
        || !bytesAt(handler + kGetLevelCallOffset, { 0xE8 })
        || !bytesAt(handler + kIncrementOffset, { 0x40 })
        || !bytesAt(handler + kLevelingLoadOffset, { 0x8B, 0x0D })
        || !bytesAt(handler + kLevelUpCallOffset, { 0xE8 })) {
        logger.error("EngineAvatar: handler at rva 0x{:X} does not have the measured shape; refusing.", rva);
        return;
    }

    const std::uintptr_t flow = readAt<std::uint32_t>(handler + kFlowLoadOffset + kLoadOperandSkip);
    const std::uintptr_t flowAgain = readAt<std::uint32_t>(handler + kFlowLevelLoadOffset + kLoadOperandSkip);
    const std::uintptr_t leveling = readAt<std::uint32_t>(handler + kLevelingLoadOffset + kLoadOperandSkip);
    const std::uintptr_t getLevel = crabe::memory::resolveCall(handler + kGetLevelCallOffset);
    const std::uintptr_t levelUp = crabe::memory::resolveCall(handler + kLevelUpCallOffset);
    if (!flow || flow != flowAgain || !leveling || !getLevel || !levelUp) {
        logger.error("EngineAvatar: handler at rva 0x{:X} led to unusable singletons or callees; refusing.", rva);
        return;
    }

    _flowAddress = flow;
    _levelingAddress = leveling;
    _getLevel = getLevel;
    _levelUp = levelUp;
    logger.info("EngineAvatar: avatar level setting resolved.");
}

std::optional<int> EngineAvatar::setAvatarLevel(int playerId, std::uint32_t characterId, int level)
{
    resolveOnce();
    if (!_levelUp || playerId < 0 || level < 0 || level > kMaxLevel)
        return std::nullopt;

    std::optional<int> result;
    const bool completed = CrashHandler::runGuarded(
        [&] {
            void* flow = readAt<void*>(_flowAddress);
            void* leveling = readAt<void*>(_levelingAddress);
            if (!flow || !leveling)
                return;
            reinterpret_cast<LevelUpFn>(_levelUp)(leveling, playerId, level, characterId, true, false);
            result = reinterpret_cast<GetLevelFn>(_getLevel)(flow, playerId);
        },
        "EngineAvatar::setAvatarLevel");
    return completed ? result : std::nullopt;
}

} // namespace crabe::infrastructure
