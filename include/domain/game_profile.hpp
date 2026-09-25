/*
** CrabeLoader
** File description:
** Declares the supported game builds and every RVA the loader hooks or patches.
** An address applied to the wrong build is silently wrong, so each one belongs to a profile.
** Reads no process memory; detection is src/infrastructure/game_profile_detect.cpp.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_DOMAIN_GAME_PROFILE_HPP_
#define CRABELOADER_DOMAIN_GAME_PROFILE_HPP_

// Which build of the game is running, and where its addresses are.
//
// Every address the loader hooks or patches is an RVA measured by hand against
// one specific shipped executable. Applied to a different build they are not
// merely wrong, they are silently wrong: a patch lands mid-instruction and the
// game misbehaves somewhere else entirely. So no address lives loose in the
// code any more; each one belongs to a GameProfile, and a profile only applies
// once the running image has been identified as the build it was measured
// against.
//
// The split, for the same reason dependency_resolver.hpp is split:
//
//  * readPeIdentity() and matchProfile() are pure. They take bytes and a table
//    and return an answer -- no process, no module handle, no Win32 -- which is
//    what makes every rule below a unit test against a synthetic PE header
//    instead of a play session. They live in src/domain/game_profile.cpp,
//    whose foot carries the same compile-time purity guards
//    dependency_resolver.cpp does.
//  * detectRunningGame() and activeProfile() read the live process. They are
//    declared here because they are the contract callers want, but they are
//    implemented in src/infrastructure/game_profile_detect.cpp so that
//    windows.h never reaches src/domain/ at all.

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace crabe::domain {

    // A profile field nobody has measured yet.
    //
    // Zero, because a shipped PE header never carries zero in TimeDateStamp or
    // SizeOfImage -- so it cannot be mistaken for a measurement, and it is
    // already the documented "skip this one" value for CheckSum. A profile
    // whose TimeDateStamp is still kUnmeasured has no usable discriminant and
    // therefore never matches: an unfilled table degrades, it does not
    // misidentify.
    inline constexpr std::uint32_t kUnmeasured = 0u;

    // One named address inside the game image.
    //
    // `rva` is the contract. It was read off the shipped binary by a human and
    // is the only thing that proves a scan landed where it was meant to; a
    // value that merely looks plausible is worse than no value at all.
    struct SymbolRva {
        std::string_view name;
        std::uint32_t rva{kUnmeasured};
    };

    // One byte patch's location: the function, plus how far into it the bytes
    // being rewritten sit.
    //
    // The two are kept apart rather than pre-added because that is how they
    // were measured -- the function's RVA is what a disassembler shows, the
    // offset is what a reader counts from its first instruction -- and a
    // pre-added constant would be checkable against neither.
    struct PatchSite {
        std::string_view name;
        std::uint32_t rva{kUnmeasured};
        std::uint32_t offset{0};
    };

    // The identity fields of a PE image, as read out of its headers.
    struct PeIdentity {
        std::uint32_t timeDateStamp{kUnmeasured};
        std::uint32_t sizeOfImage{kUnmeasured};
        std::uint32_t checkSum{kUnmeasured};
    };

    // One supported build of the game, and every address measured against it.
    //
    // Copyable: the spans point at static storage that outlives the process, so
    // a returned GameProfile stays valid.
    struct GameProfile {
        std::string_view id;          // "di3-gold-steam-1.0"
        std::string_view displayName;

        std::uint32_t timeDateStamp{kUnmeasured}; // PE header, primary discriminant
        std::uint32_t sizeOfImage{kUnmeasured};   // secondary confirmation
        std::uint32_t checkSum{kUnmeasured};      // 0 to skip

        // The Lua C API entry points, keyed on the symbol name
        // src/infrastructure/lua_symbols.cpp resolves.
        std::span<const SymbolRva> luaSymbols{};

        // The multiplayer byte-patch sites, keyed on the patch name
        // src/infrastructure/multiplayer/memory_patcher.cpp gives them.
        std::span<const PatchSite> patchSites{};

        // Engine entry points the Lua API calls into, keyed on the name
        // src/infrastructure/engine_free_camera.cpp resolves.
        std::span<const SymbolRva> engineSymbols{};

        // False when the profile carries no field that could identify a build,
        // which is what an unfilled table looks like. Such a profile is skipped
        // by matchProfile() rather than matching everything.
        [[nodiscard]] bool hasDiscriminant() const noexcept;

        // kUnmeasured when this profile does not carry `name`. Callers must
        // treat that as "this build is not described here", never as "no check
        // needed": under a matched profile a missing symbol is a refusal.
        [[nodiscard]] std::uint32_t luaSymbolRva(std::string_view name) const noexcept;

        // nullptr when this profile does not carry `name`.
        [[nodiscard]] const PatchSite* patchSite(std::string_view name) const noexcept;

        // kUnmeasured when this profile does not carry `name`, which under a
        // matched profile is a refusal, exactly as for luaSymbolRva().
        [[nodiscard]] std::uint32_t engineSymbolRva(std::string_view name) const noexcept;
    };

    // The three ways a launch can go once detection has had its say.
    enum class LoadDecision : std::uint8_t {
        // The running image is a build we have measured. Everything applies.
        Supported,

        // Unrecognised build, but every address was found by scanning anyway.
        // Lua mods load; multiplayer does not, because its patches are RVA-only
        // and an RVA from the wrong profile is a guess.
        Degraded,

        // Unrecognised build and a scan came up empty. Nothing is hooked and
        // nothing is patched: the player loses their mods, not their session.
        Refuse,
    };

    // A short noun phrase, lower case and without trailing punctuation, meant
    // to be printed inside a sentence.
    [[nodiscard]] std::string_view describe(LoadDecision decision) noexcept;

    // The outcome table, stated once so the three cases cannot drift apart.
    //
    // A matched profile is Supported whatever the scans did -- the per-symbol
    // refusal in lua_symbols.cpp is the finer-grained answer there, and it
    // stays strict. Without a profile the scans are the only evidence left, so
    // they decide between Degraded and Refuse.
    [[nodiscard]] LoadDecision decideLoad(bool profileMatched,
                                          bool allPatternScansResolved) noexcept;

    // ---------------------------------------------------------------------
    // Pure half: bytes and tables in, answers out.
    // ---------------------------------------------------------------------

    // Reads TimeDateStamp, SizeOfImage and CheckSum out of a mapped or on-disk
    // PE image.
    //
    // std::nullopt for anything that is not a PE32/PE32+ image the fields can
    // be read from: too short for a DOS header, a bad MZ or PE signature, an
    // e_lfanew pointing outside the buffer, an optional header that stops
    // before CheckSum. Parsed by documented offset rather than through
    // IMAGE_NT_HEADERS on purpose -- that is what keeps this function, and so
    // the whole matching path, testable without Win32.
    [[nodiscard]] std::optional<PeIdentity> readPeIdentity(
        std::span<const unsigned char> image) noexcept;

    // Every build the loader knows about.
    [[nodiscard]] std::span<const GameProfile> knownProfiles() noexcept;

    // The first profile in `profiles` that `identity` satisfies.
    //
    // TimeDateStamp is the discriminant and must be equal. SizeOfImage and
    // CheckSum are confirmations: each is compared only when the profile states
    // it, and a stated one that disagrees rejects the profile -- a rebuilt
    // executable can keep a timestamp while changing size, and matching on the
    // timestamp alone would then apply addresses that have moved.
    //
    // An identity whose timeDateStamp is kUnmeasured matches nothing: it means
    // the header could not be read, not that any build will do.
    [[nodiscard]] std::optional<GameProfile> matchProfile(
        const PeIdentity& identity, std::span<const GameProfile> profiles) noexcept;

    // Same, against knownProfiles().
    [[nodiscard]] std::optional<GameProfile> matchProfile(const PeIdentity& identity) noexcept;

    // ---------------------------------------------------------------------
    // Impure half: the live process.
    // ---------------------------------------------------------------------

    // Reads the PE header of the main module and matches it against the known
    // profile table. std::nullopt when the header cannot be read or no profile
    // matches, which is degraded mode -- never a failure to report upwards.
    [[nodiscard]] std::optional<GameProfile> detectRunningGame();

    // What the running image looks like, whether or not a profile matched.
    // Detected once and cached, so the log can name the TimeDateStamp of a
    // build nobody has profiled yet.
    [[nodiscard]] PeIdentity runningGameIdentity();

    // detectRunningGame(), detected once and cached; nullptr in degraded mode.
    // This is what lua_symbols and MemoryPatcher consult, so that a process
    // asks the question once and every consumer gets the same answer.
    [[nodiscard]] const GameProfile* activeProfile();

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_GAME_PROFILE_HPP_ */
