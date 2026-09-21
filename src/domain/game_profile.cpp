/*
** CrabeLoader
** File description:
** game_profile -- the supported builds, and the pure half of identifying one
*/

#include "domain/game_profile.hpp"

#include <cstddef>
#include <cstdint>

namespace {

    // -----------------------------------------------------------------------
    // PE header field offsets, from the PE/COFF specification.
    //
    // Spelled out rather than taken from IMAGE_NT_HEADERS32 because reaching
    // for that header would drag winnt.h into src/domain/ and cost the
    // matching path its testability -- the whole point of the split. The
    // offsets below are the same for PE32 and PE32+: the optional header's
    // 64-bit ImageBase absorbs the 32-bit BaseOfData field, so everything from
    // SectionAlignment onwards sits where it does in PE32.
    // -----------------------------------------------------------------------
    constexpr std::size_t kDosMagicOffset = 0x00;
    constexpr std::size_t kLfanewOffset = 0x3C;
    constexpr std::size_t kDosHeaderSize = 0x40;

    constexpr std::uint16_t kDosMagic = 0x5A4D;      // "MZ"
    constexpr std::uint32_t kPeSignature = 0x00004550; // "PE\0\0"

    constexpr std::uint16_t kOptionalMagicPe32 = 0x010B;
    constexpr std::uint16_t kOptionalMagicPe32Plus = 0x020B;

    // From the start of the PE signature.
    constexpr std::size_t kFileHeaderOffset = 4;
    constexpr std::size_t kFileHeaderSize = 20;
    constexpr std::size_t kTimeDateStampOffset = kFileHeaderOffset + 4;
    constexpr std::size_t kSizeOfOptionalHeaderOffset = kFileHeaderOffset + 16;
    constexpr std::size_t kOptionalHeaderOffset = kFileHeaderOffset + kFileHeaderSize;

    // From the start of the optional header.
    constexpr std::size_t kOptionalMagicOffset = 0;
    constexpr std::size_t kSizeOfImageOffset = 56;
    constexpr std::size_t kCheckSumOffset = 64;

    // Everything up to and including CheckSum, which is the last field read.
    constexpr std::size_t kOptionalHeaderNeeded = kCheckSumOffset + 4;

    [[nodiscard]] bool fits(std::span<const unsigned char> image,
                            std::size_t offset, std::size_t size) noexcept
    {
        // offset + size cannot wrap: size is a small literal at every call
        // site, so the subtraction below is the safe form of the comparison.
        return offset <= image.size() && size <= image.size() - offset;
    }

    [[nodiscard]] std::uint16_t readU16(std::span<const unsigned char> image,
                                        std::size_t offset) noexcept
    {
        return static_cast<std::uint16_t>(static_cast<std::uint16_t>(image[offset])
                                         | static_cast<std::uint16_t>(
                                             static_cast<std::uint16_t>(image[offset + 1]) << 8));
    }

    [[nodiscard]] std::uint32_t readU32(std::span<const unsigned char> image,
                                        std::size_t offset) noexcept
    {
        return static_cast<std::uint32_t>(image[offset])
             | (static_cast<std::uint32_t>(image[offset + 1]) << 8)
             | (static_cast<std::uint32_t>(image[offset + 2]) << 16)
             | (static_cast<std::uint32_t>(image[offset + 3]) << 24);
    }

    // -----------------------------------------------------------------------
    // di3-gold-steam-1.0 -- the build every address in this repository was
    // measured against.
    //
    // These RVAs are measurements, not guesses: a human read each one off the
    // shipped executable, and lua_symbols.cpp refuses to hook a scan result
    // that does not land on the one stated here. They moved into this table
    // unchanged and must keep moving unchanged -- rounding, reformatting or
    // "tidying" one of them breaks a hook with nothing to notice it but a play
    // session.
    // -----------------------------------------------------------------------

    // The Lua C API. Keyed on the name lua_symbols.cpp resolves; that name is
    // also what the log prints, so a refusal names the same thing this table
    // does. The game exports no Lua symbol, so each address is reached through
    // the stdlib wrapper that calls it -- which wrapper, and which call inside
    // it, stays in lua_symbols.cpp, because that is knowledge about how Lua 5.1
    // is registered rather than about where this build put it.
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

    // The multiplayer byte patches. `offset` is how far into the named function
    // the rewritten bytes sit -- zero when the patch starts at its first
    // instruction. Keyed on the name MemoryPatcher gives each record.
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

    constexpr crabe::domain::GameProfile kKnownProfiles[] = {
        {
            .id = "di3-gold-steam-1.0",
            .displayName = "Disney Infinity 3.0: Gold Edition (PC, Win32)",

            // TODO(human): fill from the shipped binary.
            //
            // All three are still kUnmeasured, so this profile carries no
            // discriminant and matchProfile() never returns it: the loader
            // runs in degraded mode on every machine until they are filled.
            // That is deliberate -- inventing a plausible TimeDateStamp would
            // make the loader claim it had identified a build it had not, and
            // then apply the RVAs above to whatever is actually running.
            //
            // How to obtain them, from the game's DisneyInfinity3.exe:
            //   timeDateStamp -> COFF File Header, TimeDateStamp
            //   sizeOfImage   -> Optional Header, SizeOfImage
            //   checkSum      -> Optional Header, CheckSum (leave at
            //                    kUnmeasured to skip the check)
            // Any PE viewer reports all three: `dumpbin /headers
            // DisneyInfinity3.exe`, CFF Explorer, or PE-bear.
            .timeDateStamp = crabe::domain::kUnmeasured,
            .sizeOfImage = crabe::domain::kUnmeasured,
            .checkSum = crabe::domain::kUnmeasured,

            .luaSymbols = kDi3GoldLuaSymbols,
            .patchSites = kDi3GoldPatchSites,
        },
    };

} // namespace

namespace crabe::domain {

    bool GameProfile::hasDiscriminant() const noexcept
    {
        // TimeDateStamp alone. SizeOfImage and CheckSum are confirmations: two
        // builds of the same executable can share either, so neither can
        // identify one on its own, and treating them as if they could is how a
        // half-filled profile would start matching the wrong game.
        return timeDateStamp != kUnmeasured;
    }

    std::uint32_t GameProfile::luaSymbolRva(std::string_view name) const noexcept
    {
        for (const SymbolRva& entry : luaSymbols) {
            if (entry.name == name)
                return entry.rva;
        }
        return kUnmeasured;
    }

    const PatchSite* GameProfile::patchSite(std::string_view name) const noexcept
    {
        for (const PatchSite& entry : patchSites) {
            if (entry.name == name)
                return &entry;
        }
        return nullptr;
    }

    std::string_view describe(LoadDecision decision) noexcept
    {
        switch (decision) {
            case LoadDecision::Supported: return "supported build";
            case LoadDecision::Degraded:  return "unrecognised build, running degraded";
            case LoadDecision::Refuse:    return "unrecognised build, refusing to patch";
        }
        return "unknown decision";
    }

    LoadDecision decideLoad(bool profileMatched, bool allPatternScansResolved) noexcept
    {
        if (profileMatched)
            return LoadDecision::Supported;
        return allPatternScansResolved ? LoadDecision::Degraded : LoadDecision::Refuse;
    }

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

    std::span<const GameProfile> knownProfiles() noexcept
    {
        return kKnownProfiles;
    }

    std::optional<GameProfile> matchProfile(const PeIdentity& identity,
                                            std::span<const GameProfile> profiles) noexcept
    {
        // A header that could not be read is not a wildcard.
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

    std::optional<GameProfile> matchProfile(const PeIdentity& identity) noexcept
    {
        return matchProfile(identity, knownProfiles());
    }

} // namespace crabe::domain

// ---------------------------------------------------------------------------
// Purity, enforced by the compiler rather than by a comment.
//
// Same guards, and the same reasoning, as the foot of dependency_resolver.cpp:
// they run after every include above has been expanded, so they see the whole
// transitive graph. readPeIdentity() and matchProfile() are what make the
// three load outcomes testable against synthetic headers, and they stop being
// testable the moment this file can reach Win32 -- at which point the honest
// way to read a PE header would be IMAGE_NT_HEADERS32 and the tests would need
// a live process. So the build stops here instead.
//
// The impure half -- GetModuleHandle, reading the live image -- is
// src/infrastructure/game_profile_detect.cpp, which is where windows.h belongs.
// ---------------------------------------------------------------------------

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
#if defined(IMGUI_VERSION) || defined(MINHOOK_H)
#error "game_profile.cpp is not pure: the renderer or the hook engine reached it"
#endif
