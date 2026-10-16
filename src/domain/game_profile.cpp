/*
** CrabeLoader
** File description:
** Holds every supported build profile, its measured RVAs, and the pure half of identifying one.
** PE field offsets come from the PE/COFF specification; the identity is read from a byte span.
** Reads no process; the running image is fetched by src/infrastructure/game_profile_detect.cpp.
**
** Authors: @LucasLhomme
*/

#include "domain/game_profile.hpp"

#include <cstddef>
#include <cstdint>

namespace {

    constexpr std::size_t kDosMagicOffset = 0x00;
    constexpr std::size_t kLfanewOffset = 0x3C;
    constexpr std::size_t kDosHeaderSize = 0x40;

    constexpr std::uint16_t kDosMagic = 0x5A4D;
    constexpr std::uint32_t kPeSignature = 0x00004550;

    constexpr std::uint16_t kOptionalMagicPe32 = 0x010B;
    constexpr std::uint16_t kOptionalMagicPe32Plus = 0x020B;

    constexpr std::size_t kFileHeaderOffset = 4;
    constexpr std::size_t kFileHeaderSize = 20;
    constexpr std::size_t kTimeDateStampOffset = kFileHeaderOffset + 4;
    constexpr std::size_t kSizeOfOptionalHeaderOffset = kFileHeaderOffset + 16;
    constexpr std::size_t kOptionalHeaderOffset = kFileHeaderOffset + kFileHeaderSize;

    constexpr std::size_t kOptionalMagicOffset = 0;
    constexpr std::size_t kSizeOfImageOffset = 56;
    constexpr std::size_t kCheckSumOffset = 64;

    constexpr std::size_t kOptionalHeaderNeeded = kCheckSumOffset + 4;

    // Checks whether the slice fits within the image bounds without wrapping.
    [[nodiscard]] bool fits(std::span<const unsigned char> image,
                            std::size_t offset, std::size_t size) noexcept
    {
        return offset <= image.size() && size <= image.size() - offset;
    }

    // Reads a 16-bit little-endian unsigned integer from the image.
    [[nodiscard]] std::uint16_t readU16(std::span<const unsigned char> image,
                                        std::size_t offset) noexcept
    {
        return static_cast<std::uint16_t>(static_cast<std::uint16_t>(image[offset])
                                         | static_cast<std::uint16_t>(
                                             static_cast<std::uint16_t>(image[offset + 1]) << 8));
    }

    // Reads a 32-bit little-endian unsigned integer from the image.
    [[nodiscard]] std::uint32_t readU32(std::span<const unsigned char> image,
                                        std::size_t offset) noexcept
    {
        return static_cast<std::uint32_t>(image[offset])
             | (static_cast<std::uint32_t>(image[offset + 1]) << 8)
             | (static_cast<std::uint32_t>(image[offset + 2]) << 16)
             | (static_cast<std::uint32_t>(image[offset + 3]) << 24);
    }

    constexpr crabe::domain::SymbolRva kDi3GoldLuaSymbols[] = {
        { "luaL_loadfile",     0xF0EBF0 },
        { "luaL_loadbuffer",   0xF0EDE0 },
        { "lua_pcall",         0xF0DF60 },

        { "lua_gettop",        0xF0D0E0 },
        { "lua_getfield",      0xF0DA00 },
        { "lua_pushvalue",     0xF0D2A0 },
        { "lua_call",          0xF0DF00 },
        { "lua_tolstring",     0xF0D5A0 },
        { "lua_settop",        0xF0D0F0 },

        { "lua_pushcclosure",  0xF0D8E0 },

        { "luaL_checktype",    0xF0EFE0 },
        { "luaL_checkany",     0xF0F010 },
        { "lua_rawset",        0xF0DCA0 },
        { "lua_rawget",        0xF0DA60 },

        { "luaL_checklstring", 0xF0F040 },
        { "lua_toboolean",     0xF0D570 },

        { "lua_isnumber",      0xF0D350 },
        { "lua_tonumber",      0xF0D4F0 },
        { "lua_pushnumber",    0xF0D7C0 },

        { "lua_touserdata",    0xF0D6D0 },
        { "lua_getmetatable",  0xF0DB20 },
        { "lua_rawequal",      0xF0D3F0 },
        { "lua_pushlstring",   0xF0D800 },
        { "lua_pushnil",       0xF0D7A0 },

        { "lua_pushboolean",   0xF0D960 },
    };

    constexpr crabe::domain::PatchSite kDi3GoldPatchSites[] = {
        { "IsSignedInToDisneyId",           0x00F62550, 0x4A },
        { "IsOnline",                       0x00F5EE90, 0x249 },
        { "IsOnlineContentAllowed",         0x00F630F0, 0x1DD },
        { "VerifyResponseSignature",        0x00F35790 },
        { "SteamAPI_RestartAppIfNecessary", 0x00032C9E },
        { "HostingSessionGate",             0x0074E97A },
        { "PlayerListAllNegotiated",        0x00F29850 },
        { "OctaneAppMutex",                 0x0003F0DE },
    };

    constexpr crabe::domain::SymbolRva kDi3GoldEngineSymbols[] = {
        { "LoopScenes::ActivateFreeCamera", 0x002A1000 },
        { "LoopScenes::CreateFreeCameras",  0x002A45C0 },
        { "BaseLoop::s_Scenes",             0x01E02940 },
        { "Script_KinematicStatePlaceWithPosition", 0x014A23F0 },
        { "Script_CameraGetPosition",               0x00F7A4C0 },
        { "Script_KinematicStateGetActualPosition", 0x01490200 },
        { "Script_RealmManager_LoadSkyDomeInZone",  0x006934C0 },
        { "Script_CreateFromInitString",            0x0035A3F0 },
        { "Script_AddActorStateByName",             0x0030F9C0 },
        { "Script_RemoveActorStateByName",          0x0030FA10 },
        { "Script_MovementSetCumulativeSpeedMultiplier", 0x01468400 },
        { "StartupLogos::Dispatch",                 0x00F8D6CD },
        { "VirtualReaderSystem::Players_AvatarLevelUp", 0x00749110 },
        { "Script_TestActorStateByName",            0x0030FAD0 },
        { "Script_DamageRadiusExceptActor",         0x00F76330 },
        { "Asset_LoadFile",                         0x0001FAF0 },
        { "Engine_Alloc",                           0x00006E80 },
        { "Meridian_DynArrayAppend",                0x002A9E90 },
        { "Shader_LookupCrc",                       0x000509D0 },
        { "Shader_ParseBindings",                   0x0005D5C0 },
        { "Resolution_IsSupported",                 0x000459B0 },
        { "ResolutionList_Contains",                0x0065A010 },
        { "Flash_Movie_SetViewport",                0x003F3C20 },
        { "g_Gateway",                              0x01D3E278 },
        { "g_VirtualReader",                        0x01E1CEB8 },
        { "Gateway::AddTag8",                       0x0003A320 },
        { "Gateway::RemoveTagByUid",                0x0002AF60 },
        { "Gateway::MapUIDToTag",                   0x000271E0 },
        { "Gateway::GetOpenTagSlot",                0x0002A630 },
        { "VirtualReaderSystem::ChangeAvatar",      0x00773060 },
        { "VirtualReaderSystem::ReadAvatarTag",     0x007495BC },
        { "Gateway::WriteExperience",               0x0002A2A0 },
        { "Gateway::WriteLevel",                    0x0002A320 },
        { "Gateway::WriteLastPlayedTime",           0x0002A3A0 },
        { "Gateway::WriteAction",                   0x0002A420 },
        { "Gateway::WriteOwner",                    0x0002A4A0 },
        { "Gateway::WriteProgressionData",          0x0002A5B0 },
        { "MifareMini::WriteExperience",            0x00026C00 },
        { "MifareMini::WriteLevel",                 0x00026CA0 },
        { "MifareMini::WriteLastPlayedTime",        0x00026D50 },
        { "MifareMini::WriteAction",                0x00026DF0 },
        { "MifareMini::WriteOwner",                 0x00026E70 },
        { "MifareMini::WriteProgressionData",       0x00026F40 },
    };

    constexpr crabe::domain::GameProfile kKnownProfiles[] = {
        {
            .id = "di3-gold-steam-1.0",
            .displayName = "Disney Infinity 3.0: Gold Edition (PC, Win32)",
            .timeDateStamp = 0x5851C5D1,
            .sizeOfImage   = 0x0209B000,
            .checkSum      = 0x00000000,

            .luaSymbols = kDi3GoldLuaSymbols,
            .patchSites = kDi3GoldPatchSites,
            .engineSymbols = kDi3GoldEngineSymbols,
        },
    };

} // namespace

namespace crabe::domain {

    // Returns true if this profile carries a measured TimeDateStamp.
    bool GameProfile::hasDiscriminant() const noexcept
    {
        return timeDateStamp != kUnmeasured;
    }

    // Resolves the RVA of a Lua C API function by name.
    std::uint32_t GameProfile::luaSymbolRva(std::string_view name) const noexcept
    {
        for (const SymbolRva& entry : luaSymbols) {
            if (entry.name == name)
                return entry.rva;
        }
        return kUnmeasured;
    }

    // Resolves the RVA of an engine entry point by name.
    std::uint32_t GameProfile::engineSymbolRva(std::string_view name) const noexcept
    {
        for (const SymbolRva& entry : engineSymbols) {
            if (entry.name == name)
                return entry.rva;
        }
        return kUnmeasured;
    }

    // Finds a patch site specification by its descriptive name.
    const PatchSite* GameProfile::patchSite(std::string_view name) const noexcept
    {
        for (const PatchSite& entry : patchSites) {
            if (entry.name == name)
                return &entry;
        }
        return nullptr;
    }

    // Returns a readable description of a build support decision.
    std::string_view describe(LoadDecision decision) noexcept
    {
        switch (decision) {
            case LoadDecision::Supported: return "supported build";
            case LoadDecision::Degraded:  return "unrecognised build, running degraded";
            case LoadDecision::Refuse:    return "unrecognised build, refusing to patch";
        }
        return "unknown decision";
    }

    // Determines the loader mode from profile matching and pattern scan status.
    LoadDecision decideLoad(bool profileMatched, bool allPatternScansResolved) noexcept
    {
        if (profileMatched)
            return LoadDecision::Supported;
        return allPatternScansResolved ? LoadDecision::Degraded : LoadDecision::Refuse;
    }

    // Reads PE identity fields from a memory or file image byte buffer.
    std::optional<PeIdentity> readPeIdentity(std::span<const unsigned char> image) noexcept
    {
        if (!fits(image, kDosMagicOffset, kDosHeaderSize))
            return std::nullopt;
        if (readU16(image, kDosMagicOffset) != kDosMagic)
            return std::nullopt;

        const std::uint32_t lfanew = readU32(image, kLfanewOffset);
        const auto peOffset = static_cast<std::size_t>(lfanew);

        if (!fits(image, peOffset, kFileHeaderOffset + kFileHeaderSize))
            return std::nullopt;
        if (readU32(image, peOffset) != kPeSignature)
            return std::nullopt;

        const std::uint16_t sizeOfOptionalHeader =
            readU16(image, peOffset + kSizeOfOptionalHeaderOffset);
        if (sizeOfOptionalHeader < kOptionalHeaderNeeded)
            return std::nullopt;

        const std::size_t optionalHeader = peOffset + kOptionalHeaderOffset;
        if (!fits(image, optionalHeader, kOptionalHeaderNeeded))
            return std::nullopt;

        const std::uint16_t optionalMagic = readU16(image, optionalHeader + kOptionalMagicOffset);
        if (optionalMagic != kOptionalMagicPe32 && optionalMagic != kOptionalMagicPe32Plus)
            return std::nullopt;

        PeIdentity identity;
        identity.timeDateStamp = readU32(image, peOffset + kTimeDateStampOffset);
        identity.sizeOfImage = readU32(image, optionalHeader + kSizeOfImageOffset);
        identity.checkSum = readU32(image, optionalHeader + kCheckSumOffset);
        return identity;
    }

    // Returns the static list of built-in game profiles.
    std::span<const GameProfile> knownProfiles() noexcept
    {
        return kKnownProfiles;
    }

    // Matches a PE header identity against a list of game profiles.
    std::optional<GameProfile> matchProfile(const PeIdentity& identity,
                                            std::span<const GameProfile> profiles) noexcept
    {
        if (identity.timeDateStamp == kUnmeasured)
            return std::nullopt;

        for (const GameProfile& profile : profiles) {
            if (!profile.hasDiscriminant())
                continue;
            if (profile.timeDateStamp != identity.timeDateStamp)
                continue;
            if (profile.sizeOfImage != kUnmeasured && profile.sizeOfImage != identity.sizeOfImage)
                continue;
            if (profile.checkSum != kUnmeasured && profile.checkSum != identity.checkSum)
                continue;
            return profile;
        }
        return std::nullopt;
    }

    // Matches a PE header identity against known profiles.
    std::optional<GameProfile> matchProfile(const PeIdentity& identity) noexcept
    {
        return matchProfile(identity, knownProfiles());
    }

} // namespace crabe::domain

#if defined(_WINDOWS_) || defined(_INC_WINDOWS)
#error "game_profile.cpp is not pure: windows.h reached it transitively"
#endif
#if defined(_WINNT_) || defined(_WINDEF_) || defined(_MINWINDEF_) || defined(_WINBASE_)
#error "game_profile.cpp is not pure: a Win32 API header reached it transitively"
#endif
#if defined(_WINUSER_) || defined(_WINGDI_) || defined(_WINREG_) || defined(_MINWINBASE_)
#error "game_profile.cpp is not pure: a Win32 API header reached it transitively"
#endif
#if defined(_WINSOCKAPI_) || defined(_WINSOCK2API_) || defined(_OBJBASE_H_) || defined(__RPC_H__)
#error "game_profile.cpp is not pure: a Win32 API header reached it transitively"
#endif
#if defined(lua_h) || defined(LUA_VERSION) || defined(LUA_REGISTRYINDEX) || defined(lauxlib_h)
#error "game_profile.cpp is not pure: a Lua header reached it transitively"
#endif
#if defined(IMGUI_VERSION) || defined(MH_ALL_HOOKS)
#error "game_profile.cpp is not pure: the renderer or the hook engine reached it"
#endif
