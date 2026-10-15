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

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace crabe::domain {

    inline constexpr std::uint32_t kUnmeasured = 0u;

    struct SymbolRva {
        std::string_view name;
        std::uint32_t rva{kUnmeasured};
    };

    struct PatchSite {
        std::string_view name;
        std::uint32_t rva{kUnmeasured};
        std::uint32_t offset{0};
    };

    struct PeIdentity {
        std::uint32_t timeDateStamp{kUnmeasured};
        std::uint32_t sizeOfImage{kUnmeasured};
        std::uint32_t checkSum{kUnmeasured};
    };

    struct GameProfile {
        std::string_view id;
        std::string_view displayName;
        std::uint32_t timeDateStamp{kUnmeasured};
        std::uint32_t sizeOfImage{kUnmeasured};
        std::uint32_t checkSum{kUnmeasured};
        std::span<const SymbolRva> luaSymbols{};
        std::span<const PatchSite> patchSites{};
        std::span<const SymbolRva> engineSymbols{};

        // Returns true if the profile contains at least one field identifying a build.
        [[nodiscard]] bool hasDiscriminant() const noexcept;

        // Returns the RVA for a Lua symbol, or kUnmeasured if absent.
        [[nodiscard]] std::uint32_t luaSymbolRva(std::string_view name) const noexcept;

        // Returns the patch site for a patch name, or nullptr if absent.
        [[nodiscard]] const PatchSite* patchSite(std::string_view name) const noexcept;

        // Returns the RVA for an engine symbol, or kUnmeasured if absent.
        [[nodiscard]] std::uint32_t engineSymbolRva(std::string_view name) const noexcept;
    };

    enum class LoadDecision : std::uint8_t {
        Supported,
        Degraded,
        Refuse,
    };

    // Returns a short noun phrase describing a load decision.
    [[nodiscard]] std::string_view describe(LoadDecision decision) noexcept;

    // Decides whether to load normally, degraded, or refuse based on profile match and scan results.
    [[nodiscard]] LoadDecision decideLoad(bool profileMatched,
                                          bool allPatternScansResolved) noexcept;

    // Reads TimeDateStamp, SizeOfImage and CheckSum from a mapped PE image buffer.
    [[nodiscard]] std::optional<PeIdentity> readPeIdentity(
        std::span<const unsigned char> image) noexcept;

    // Returns all game profiles known to CrabeLoader.
    [[nodiscard]] std::span<const GameProfile> knownProfiles() noexcept;

    // Matches a PE identity against a list of game profiles.
    [[nodiscard]] std::optional<GameProfile> matchProfile(
        const PeIdentity& identity, std::span<const GameProfile> profiles) noexcept;

    // Matches a PE identity against the list of known game profiles.
    [[nodiscard]] std::optional<GameProfile> matchProfile(const PeIdentity& identity) noexcept;

    // Reads the PE header of the main module and matches it against the known profile table.
    [[nodiscard]] std::optional<GameProfile> detectRunningGame();

    // Returns the PE identity of the running game image.
    [[nodiscard]] PeIdentity runningGameIdentity();

    // Returns the active GameProfile for the running executable, or nullptr if degraded.
    [[nodiscard]] const GameProfile* activeProfile();

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_GAME_PROFILE_HPP_ */
