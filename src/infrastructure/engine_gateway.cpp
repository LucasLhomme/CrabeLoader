/*
** CrabeLoader
** File description:
** Calls Gateway::AddTag / RemoveTag by UID and hooks the six UID-keyed Gateway::Write* wrappers.
** Every address comes from the active game profile and is checked byte by byte before use;
** without a matched profile nothing is resolved, so the bridge simply reports itself absent.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/engine_gateway.hpp"

#include <cstring>
#include <windows.h>

#include "domain/game_profile.hpp"
#include "infrastructure/crash_handler.hpp"
#include "infrastructure/memory.hpp"
#include "infrastructure/script_natives.hpp"
#include "shared/logger.hpp"

namespace {

    using crabe::infrastructure::EngineGateway;
    using crabe::infrastructure::script_natives::bytesAt;
    using crabe::infrastructure::script_natives::readAt;

    struct Writer {
        const char* symbol;
        const char* tagWriterSymbol;
    };

    constexpr Writer kWriters[] = {
        { "Gateway::WriteExperience",      "MifareMini::WriteExperience" },
        { "Gateway::WriteLevel",           "MifareMini::WriteLevel" },
        { "Gateway::WriteLastPlayedTime",  "MifareMini::WriteLastPlayedTime" },
        { "Gateway::WriteAction",          "MifareMini::WriteAction" },
        { "Gateway::WriteOwner",           "MifareMini::WriteOwner" },
        { "Gateway::WriteProgressionData", "MifareMini::WriteProgressionData" },
    };

    constexpr const char* kAddTagSymbol = "Gateway::AddTag8";
    constexpr const char* kRemoveTagSymbol = "Gateway::RemoveTagByUid";
    constexpr const char* kMapUidSymbol = "Gateway::MapUIDToTag";
    constexpr const char* kOpenSlotSymbol = "Gateway::GetOpenTagSlot";
    constexpr const char* kGatewaySymbol = "g_Gateway";
    constexpr const char* kVirtualReaderSymbol = "g_VirtualReader";
    constexpr const char* kChangeAvatarSymbol = "VirtualReaderSystem::ChangeAvatar";
    constexpr const char* kVirtualReaderReadSymbol = "VirtualReaderSystem::ReadAvatarTag";

    constexpr std::uintptr_t kWriterMapCallOffset = 0x0A;
    constexpr std::uintptr_t kWriterTagCallOffset = 0x31;
    constexpr std::uintptr_t kAddTagBodyOffset = 0x18;
    constexpr std::uintptr_t kAddTagSlotCallOffset = 0x1F;
    constexpr std::uintptr_t kRemoveTagBodyOffset = 0x1A;
    constexpr std::uintptr_t kRemoveTagMapCallOffset = 0x26;
    constexpr std::uintptr_t kChangeAvatarReadOffset = 0x08;
    constexpr std::uintptr_t kChangeAvatarGatewayOffset = 0x1A;
    constexpr std::uintptr_t kChangeAvatarRemoveCallOffset = 0x23;
    constexpr std::uintptr_t kChangeAvatarAddCallOffset = 0x41;
    constexpr std::uint8_t kVirtualAvatarTag = 0x2C;
    constexpr std::uint32_t kNoAvatarTag = 0xFFFFFFFF;
    constexpr std::size_t kMaxQueuedWrites = 256;

    using AddTagFn = void(__thiscall*)(void* gateway, std::uint64_t uid, std::uint32_t tagId, std::uint8_t antenna,
                                       std::uint32_t experience, std::uint8_t level, std::uint32_t lastTime,
                                       std::uint8_t action, std::uint8_t owner);
    using RemoveTagFn = void(__thiscall*)(void* gateway, std::uint64_t uid);
    using WriteProgressionFn = bool(__thiscall*)(void* gateway, std::uint64_t uid, const std::uint8_t* bits, bool flush);
    using WriteFn = bool(__fastcall*)(void* gateway, void* edx, std::uint32_t uidLo, std::uint32_t uidHi,
                                      std::uint32_t value, std::uint32_t flush);

    thread_local bool t_ownWrite = false;

    // Returns absolute address of symbol under active profile, or 0 if missing.
    std::uintptr_t profileAddress(std::uintptr_t base, const char* symbol)
    {
        const crabe::domain::GameProfile* profile = crabe::domain::activeProfile();
        if (!profile)
            return 0;
        const std::uint32_t rva = profile->engineSymbolRva(symbol);
        return rva == crabe::domain::kUnmeasured ? 0 : base + rva;
    }

    // Returns whether the instruction at site is a relative call targeting target.
    bool callsTo(std::uintptr_t site, std::uintptr_t target)
    {
        return target && crabe::memory::isReadable(site, 5) && crabe::memory::resolveCall(site) == target;
    }

    // Returns whether the instruction at site starts with opcode and references address.
    bool loadsFrom(std::uintptr_t site, std::initializer_list<std::uint8_t> opcode, std::uintptr_t address)
    {
        const std::uintptr_t operand = site + opcode.size();
        return address && bytesAt(site, opcode) && crabe::memory::isReadable(operand, sizeof(std::uint32_t))
            && readAt<std::uint32_t>(operand) == address;
    }

    // Validates AddTag, RemoveTag and globals against known call site layouts.
    bool registryHasExpectedShape(std::uintptr_t addTag, std::uintptr_t removeTag, std::uintptr_t mapUid,
                                  std::uintptr_t openSlot, std::uintptr_t gatewayPtr, std::uintptr_t virtualReaderPtr,
                                  std::uintptr_t changeAvatar, std::uintptr_t virtualReaderRead)
    {
        return addTag && removeTag && changeAvatar && virtualReaderRead
            && bytesAt(addTag, { 0x6A, 0xFF, 0x68 })
            && bytesAt(addTag + kAddTagBodyOffset, { 0x56, 0x8B, 0xF1 })
            && callsTo(addTag + kAddTagSlotCallOffset, openSlot)
            && bytesAt(removeTag, { 0x6A, 0xFF, 0x68 })
            && bytesAt(removeTag + kRemoveTagBodyOffset, { 0x8B, 0x6C, 0x24, 0x4C })
            && callsTo(removeTag + kRemoveTagMapCallOffset, mapUid)
            && bytesAt(changeAvatar + kChangeAvatarReadOffset, { 0x8B, 0x47, kVirtualAvatarTag })
            && loadsFrom(changeAvatar + kChangeAvatarGatewayOffset, { 0x8B, 0x0D }, gatewayPtr)
            && callsTo(changeAvatar + kChangeAvatarRemoveCallOffset, removeTag)
            && callsTo(changeAvatar + kChangeAvatarAddCallOffset, addTag)
            && loadsFrom(virtualReaderRead, { 0xA1 }, virtualReaderPtr)
            && bytesAt(virtualReaderRead + 5, { 0x8B, 0x48, kVirtualAvatarTag });
    }

    // Validates the structure and prologue of a Gateway write wrapper.
    bool writerHasExpectedShape(std::uintptr_t writer, std::uintptr_t mapUid, std::uintptr_t tagWriter)
    {
        return writer
            && bytesAt(writer, { 0x8B, 0x44, 0x24, 0x08, 0x8B, 0x54, 0x24, 0x04, 0x50, 0x52 })
            && callsTo(writer + kWriterMapCallOffset, mapUid)
            && callsTo(writer + kWriterTagCallOffset, tagWriter);
    }

} // namespace

namespace crabe::infrastructure {

EngineGateway& EngineGateway::get()
{
    static EngineGateway instance;
    return instance;
}

const char* EngineGateway::fieldName(Field field)
{
    switch (field) {
        case Field::Experience: return "experience";
        case Field::Level: return "level";
        case Field::LastPlayed: return "lastPlayed";
        case Field::Action: return "action";
        case Field::Owner: return "owner";
        case Field::Progression: return "progression";
    }
    return "unknown";
}

void EngineGateway::resolveOnce()
{
    if (_attempted)
        return;
    _attempted = true;

    auto& logger = crabe::shared::Logger::getInstance();
    if (!crabe::domain::activeProfile()) {
        logger.info("EngineGateway: no game profile matched; figure tags cannot be added.");
        return;
    }

    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    const std::uintptr_t addTag = profileAddress(base, kAddTagSymbol);
    const std::uintptr_t removeTag = profileAddress(base, kRemoveTagSymbol);
    const std::uintptr_t mapUid = profileAddress(base, kMapUidSymbol);
    const std::uintptr_t gatewayPtr = profileAddress(base, kGatewaySymbol);
    const std::uintptr_t virtualReaderPtr = profileAddress(base, kVirtualReaderSymbol);
    if (!registryHasExpectedShape(addTag, removeTag, mapUid, profileAddress(base, kOpenSlotSymbol), gatewayPtr,
                                  virtualReaderPtr, profileAddress(base, kChangeAvatarSymbol),
                                  profileAddress(base, kVirtualReaderReadSymbol))) {
        logger.warning("EngineGateway: AddTag, RemoveTag or the registry globals do not have the expected shape; refusing.");
        return;
    }

    std::array<std::uintptr_t, 6> writers{};
    for (std::size_t i = 0; i < writers.size(); ++i) {
        writers[i] = profileAddress(base, kWriters[i].symbol);
        if (!writerHasExpectedShape(writers[i], mapUid, profileAddress(base, kWriters[i].tagWriterSymbol))) {
            logger.warning("EngineGateway: {} does not have the expected shape; refusing.", kWriters[i].symbol);
            return;
        }
    }

    _addTag = addTag;
    _removeTag = removeTag;
    _gatewayPtr = gatewayPtr;
    _virtualReaderPtr = virtualReaderPtr;
    _writers = writers;
    _resolved = true;
    logger.info("EngineGateway: resolved (AddTag rva 0x{:X}, RemoveTag rva 0x{:X}, g_Gateway rva 0x{:X}).",
                addTag - base, removeTag - base, gatewayPtr - base);
}

void EngineGateway::initialize()
{
    resolveOnce();
    if (!_resolved)
        return;

    const auto install = [this]<Field F>() {
        constexpr auto index = static_cast<std::size_t>(F);
        Hook& hook = _writeHooks[index];
        if (hook.isInstalled())
            return;
        if (!hook.install(reinterpret_cast<void*>(_writers[index]), reinterpret_cast<void*>(&EngineGateway::hkWrite<F>),
                          kWriters[index].symbol)) {
            crabe::shared::Logger::getInstance().warning(
                "EngineGateway: could not hook {}; the game's writes to figures will not be seen.", kWriters[index].symbol);
        }
    };
    install.template operator()<Field::Experience>();
    install.template operator()<Field::Level>();
    install.template operator()<Field::LastPlayed>();
    install.template operator()<Field::Action>();
    install.template operator()<Field::Owner>();
    install.template operator()<Field::Progression>();
}

void EngineGateway::uninitialize()
{
    for (Hook& hook : _writeHooks)
        hook.remove();
    std::lock_guard lock(_mutex);
    _watched.clear();
    _addedOnAntenna.clear();
    _writes.clear();
}

template <EngineGateway::Field F>
bool __fastcall EngineGateway::hkWrite(void* gateway, void* edx, std::uint32_t uidLo, std::uint32_t uidHi,
                                       std::uint32_t value, std::uint32_t flush)
{
    EngineGateway& self = get();
    const auto original = reinterpret_cast<WriteFn>(self._writeHooks[static_cast<std::size_t>(F)].getOriginal());
    const bool written = original && original(gateway, edx, uidLo, uidHi, value, flush);
    if (written && !t_ownWrite) {
        const std::uint64_t uid = (static_cast<std::uint64_t>(uidHi) << 32) | uidLo;
        const auto* bits = F == Field::Progression ? reinterpret_cast<const std::uint8_t*>(value) : nullptr;
        self.record(uid, F, value, bits);
    }
    return written;
}

void EngineGateway::record(std::uint64_t uid, Field field, std::uint32_t value, const std::uint8_t* progression)
{
    TagWrite write{ uid, field, progression ? 0u : value, {} };
    if (progression)
        std::memcpy(write.progression.data(), progression, write.progression.size());

    std::lock_guard lock(_mutex);
    if (!_watched.contains(uid))
        return;
    if (_writes.size() >= kMaxQueuedWrites)
        _writes.pop_front();
    _writes.push_back(write);
    crabe::shared::Logger::getInstance().debug("EngineGateway: game wrote {} = {} to tag {:014x}.",
                                               fieldName(field), write.value, uid);
}

bool EngineGateway::addTag(std::uint64_t uid, std::uint32_t characterId, std::uint8_t antenna, const TagData& data,
                           bool watch)
{
    resolveOnce();
    if (!_resolved || uid == 0)
        return false;
    void* gateway = readAt<void*>(_gatewayPtr);
    if (!gateway)
        return false;

    std::optional<std::uint64_t> previous;
    {
        std::lock_guard lock(_mutex);
        const auto found = _addedOnAntenna.find(antenna);
        if (found != _addedOnAntenna.end())
            previous = found->second;
    }
    if (previous && !removeTag(*previous))
        crabe::shared::Logger::getInstance().warning("EngineGateway: could not remove tag {:014x} before adding {:014x}.",
                                                     *previous, uid);

    if (watch) {
        std::lock_guard lock(_mutex);
        _watched.insert(uid);
    }

    const auto add = reinterpret_cast<AddTagFn>(_addTag);
    const auto writeProgression = reinterpret_cast<WriteProgressionFn>(_writers[static_cast<std::size_t>(Field::Progression)]);
    bool progressionSet = !data.progression.has_value();
    const bool completed = CrashHandler::runGuarded(
        [&] {
            add(gateway, uid, characterId, antenna, data.experience, data.level, data.lastPlayed, data.action, data.owner);
            if (data.progression) {
                t_ownWrite = true;
                progressionSet = writeProgression(gateway, uid, data.progression->data(), false);
                t_ownWrite = false;
            }
        },
        "EngineGateway::addTag");
    t_ownWrite = false;

    {
        std::lock_guard lock(_mutex);
        if (completed)
            _addedOnAntenna[antenna] = uid;
        else
            _watched.erase(uid);
    }
    crabe::shared::Logger::getInstance().info(
        "EngineGateway: tag {:014x} (character {}, antenna {}, level {}) {}{}.", uid, characterId, antenna, data.level,
        completed ? "added" : "faulted", completed && !progressionSet ? ", but its progression was refused" : "");
    return completed;
}

bool EngineGateway::removeTag(std::uint64_t uid)
{
    resolveOnce();
    if (!_resolved || uid == 0)
        return false;
    void* gateway = readAt<void*>(_gatewayPtr);
    if (!gateway)
        return false;

    const auto remove = reinterpret_cast<RemoveTagFn>(_removeTag);
    const bool completed = CrashHandler::runGuarded([&] { remove(gateway, uid); }, "EngineGateway::removeTag");
    {
        std::lock_guard lock(_mutex);
        _watched.erase(uid);
        std::erase_if(_addedOnAntenna, [uid](const auto& entry) { return entry.second == uid; });
    }
    crabe::shared::Logger::getInstance().info("EngineGateway: tag {:014x} {}.", uid, completed ? "removed" : "faulted");
    return completed;
}

std::optional<std::uint32_t> EngineGateway::virtualAvatarTag() const
{
    if (!_resolved)
        return std::nullopt;
    const auto reader = readAt<std::uintptr_t>(_virtualReaderPtr);
    if (!reader || !crabe::memory::isReadable(reader + kVirtualAvatarTag, sizeof(std::uint32_t)))
        return std::nullopt;
    const auto id = readAt<std::uint32_t>(reader + kVirtualAvatarTag);
    return id == kNoAvatarTag ? 0u : id;
}

std::optional<EngineGateway::TagWrite> EngineGateway::pollWrite()
{
    std::lock_guard lock(_mutex);
    if (_writes.empty())
        return std::nullopt;
    TagWrite write = _writes.front();
    _writes.pop_front();
    return write;
}

} // namespace crabe::infrastructure
