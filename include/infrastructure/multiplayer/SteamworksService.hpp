/*
** CrabeLoader
** File description:
** SteamworksService header
*/

#pragma once

#include "domain/multiplayer/ISteamworksService.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include <windows.h>

namespace Crabe::Multiplayer {

    class SteamworksService final : public ISteamworksService {
    public:
        using OnLobbyJoinedCallback = std::function<void(uint64_t lobbyId, const std::string& locationData)>;

        explicit SteamworksService();
        ~SteamworksService() override;

        SteamworksService(const SteamworksService&) = delete;
        SteamworksService& operator=(const SteamworksService&) = delete;
        SteamworksService(SteamworksService&&) = delete;
        SteamworksService& operator=(SteamworksService&&) = delete;

        [[nodiscard]] bool initialize() override;
        void shutdown() override;
        void runCallbacks() override;

        [[nodiscard]] bool isAvailable() const noexcept override;
        [[nodiscard]] uint64_t getLocalSteamId() const noexcept override;
        [[nodiscard]] std::string getPersonaName() const override;

        [[nodiscard]] bool createLobby(bool friendsOnly = true, int maxMembers = 4) override;
        void leaveLobby() override;
        bool joinLobby(uint64_t lobbyId) override;
        bool setLobbyData(std::string_view key, std::string_view value) override;
        [[nodiscard]] std::string getLobbyData(std::string_view key) const override;

        bool openInviteOverlay() override;
        [[nodiscard]] int getFriendCount() const noexcept override;
        [[nodiscard]] std::vector<SteamFriendInfo> getFriends() const override;
        [[nodiscard]] SteamLobbyStatus getLobbyStatus() const noexcept override;

        void setOnLobbyJoinedHandler(OnLobbyJoinedCallback handler);

    private:
        bool bindSteamFunctions();

        // Raw function pointer types from steam_api.dll
        using FnSteamAPI_Init = bool (__cdecl*)();
        using FnSteamAPI_RunCallbacks = void (__cdecl*)();
        using FnSteamFriends = void* (__cdecl*)();
        using FnSteamMatchmaking = void* (__cdecl*)();
        using FnSteamUser = void* (__cdecl*)();
        using FnSteamUtils = void* (__cdecl*)();

        HMODULE _steamApiModule{nullptr};
        FnSteamAPI_Init _fnSteamAPI_Init{nullptr};
        FnSteamAPI_RunCallbacks _fnSteamAPI_RunCallbacks{nullptr};
        FnSteamFriends _fnSteamFriends{nullptr};
        FnSteamMatchmaking _fnSteamMatchmaking{nullptr};
        FnSteamUser _fnSteamUser{nullptr};
        FnSteamUtils _fnSteamUtils{nullptr};

        void* _pSteamFriends{nullptr};
        void* _pSteamMatchmaking{nullptr};
        void* _pSteamUser{nullptr};
        void* _pSteamUtils{nullptr};

        mutable std::mutex _mutex;
        std::atomic<bool> _initialized{false};
        std::atomic<bool> _available{false};

        SteamLobbyStatus _lobbyStatus{};
        OnLobbyJoinedCallback _onLobbyJoinedHandler;
    };

} // namespace Crabe::Multiplayer

