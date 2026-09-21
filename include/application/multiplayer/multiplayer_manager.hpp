/*
** CrabeLoader
** File description:
** MultiplayerManager - Application orchestrator for multiplayer subsystem
*/

#ifndef MULTIPLAYER_MANAGER_HPP_
#define MULTIPLAYER_MANAGER_HPP_

#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

#include "domain/multiplayer/i_engine_patcher.hpp"
#include "domain/multiplayer/i_network_redirector.hpp"
#include "domain/multiplayer/i_nat_service.hpp"
#include "domain/multiplayer/i_steamworks_service.hpp"

namespace crabe::multiplayer::application {

    class MultiplayerManager final {
    public:
        static MultiplayerManager& getInstance() noexcept;

        bool initialize();
        void uninitialize();
        void applyMemoryPatchesNow();

        void setTargetServer(std::wstring_view host, uint16_t port);
        [[nodiscard]] std::wstring getTargetHost() const;
        [[nodiscard]] uint16_t getTargetPort() const;
        [[nodiscard]] bool isServerReachable(std::string_view host, uint16_t port, uint32_t timeoutMs = 250) const;

        [[nodiscard]] bool arePatchesActive() const noexcept;
        [[nodiscard]] bool isRedirectorActive() const noexcept;
        [[nodiscard]] unsigned getPatchedCount() const noexcept;

        [[nodiscard]] crabe::multiplayer::NatStatus getNatStatus() const noexcept;
        bool triggerPortForward(uint16_t port = 3074, std::string_view protocol = "UDP");
        void releasePortForward(uint16_t port = 3074, std::string_view protocol = "UDP");

        // Steamworks Integration
        [[nodiscard]] bool isSteamAvailable() const noexcept;
        [[nodiscard]] std::string getSteamPersonaName() const;
        [[nodiscard]] uint64_t getSteamId() const noexcept;
        bool createSteamLobby(bool friendsOnly = true, int maxMembers = 4);
        void leaveSteamLobby();
        bool openSteamInviteOverlay();
        [[nodiscard]] crabe::multiplayer::SteamLobbyStatus getSteamLobbyStatus() const noexcept;
        [[nodiscard]] int getSteamFriendCount() const noexcept;
        [[nodiscard]] std::vector<crabe::multiplayer::SteamFriendInfo> getSteamFriends() const;
        void updateSteamLobbyLocation(std::string_view locationData);

        void setDirectConnectTarget(std::string_view friendName, std::string_view ip, uint16_t port, std::string_view hostDid = "");
        [[nodiscard]] std::string formatLocationString(
            std::string_view pubIp, uint16_t pubPort,
            std::string_view privIp = "", uint16_t privPort = 0,
            std::string_view hostDid = "", std::string_view gameName = "IN2PC") const;
        [[nodiscard]] static std::string ipv4ToHex(std::string_view ip);
        [[nodiscard]] static std::string portToHex(uint16_t port);

    private:
        MultiplayerManager();
        ~MultiplayerManager();

        MultiplayerManager(const MultiplayerManager&) = delete;
        MultiplayerManager& operator=(const MultiplayerManager&) = delete;
        MultiplayerManager(MultiplayerManager&&) = delete;
        MultiplayerManager& operator=(MultiplayerManager&&) = delete;

        void patchWorkerThread();

        std::unique_ptr<domain::IEnginePatcher> _patcher;
        std::unique_ptr<domain::INetworkRedirector> _redirector;
        std::unique_ptr<crabe::multiplayer::INatService> _natService;
        std::unique_ptr<crabe::multiplayer::ISteamworksService> _steamService;

        std::atomic<bool> _initialized{ false };
        std::atomic<bool> _workerStop{ false };
        std::thread _workerThread;
    };

} // namespace crabe::multiplayer::application

#endif /* !MULTIPLAYER_MANAGER_HPP_ */

