/*
** CrabeLoader
** File description:
** ISteamworksService - Domain interface for Steamworks P2P matchmaking & friend integration
*/

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Crabe::Multiplayer {

    struct SteamFriendInfo {
        uint64_t steamId{0};
        std::string name;
        bool isPlayingThisGame{false};
    };

    struct SteamLobbyStatus {
        bool inLobby{false};
        bool isHost{false};
        uint64_t lobbyId{0};
        uint64_t hostSteamId{0};
        int memberCount{0};
        int memberLimit{4};
        std::string locationData;
    };

    class ISteamworksService {
    public:
        virtual ~ISteamworksService() = default;

        [[nodiscard]] virtual bool initialize() = 0;
        virtual void shutdown() = 0;
        virtual void runCallbacks() = 0;

        [[nodiscard]] virtual bool isAvailable() const noexcept = 0;
        [[nodiscard]] virtual uint64_t getLocalSteamId() const noexcept = 0;
        [[nodiscard]] virtual std::string getPersonaName() const = 0;

        [[nodiscard]] virtual bool createLobby(bool friendsOnly = true, int maxMembers = 4) = 0;
        virtual void leaveLobby() = 0;
        virtual bool joinLobby(uint64_t lobbyId) = 0;
        virtual bool setLobbyData(std::string_view key, std::string_view value) = 0;
        [[nodiscard]] virtual std::string getLobbyData(std::string_view key) const = 0;

        virtual bool openInviteOverlay() = 0;
        [[nodiscard]] virtual int getFriendCount() const noexcept = 0;
        [[nodiscard]] virtual std::vector<SteamFriendInfo> getFriends() const = 0;
        [[nodiscard]] virtual SteamLobbyStatus getLobbyStatus() const noexcept = 0;
    };

} // namespace Crabe::Multiplayer

