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

#include "loader/multiplayer/domain/IEnginePatcher.hpp"
#include "loader/multiplayer/domain/INetworkRedirector.hpp"
#include "loader/multiplayer/domain/INatService.hpp"

namespace Multiplayer::Application {

    class MultiplayerManager final {
    public:
        static MultiplayerManager& getInstance() noexcept;

        bool initialize();
        void uninitialize();

        void setTargetServer(std::wstring_view host, uint16_t port);
        [[nodiscard]] std::wstring getTargetHost() const;
        [[nodiscard]] uint16_t getTargetPort() const;

        [[nodiscard]] bool arePatchesActive() const noexcept;
        [[nodiscard]] bool isRedirectorActive() const noexcept;
        [[nodiscard]] unsigned getPatchedCount() const noexcept;

        [[nodiscard]] Crabe::Multiplayer::NatStatus getNatStatus() const noexcept;
        bool triggerPortForward(uint16_t port = 3074, std::string_view protocol = "UDP");
        void releasePortForward(uint16_t port = 3074, std::string_view protocol = "UDP");

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

        std::unique_ptr<Domain::IEnginePatcher> _patcher;
        std::unique_ptr<Domain::INetworkRedirector> _redirector;
        std::unique_ptr<Crabe::Multiplayer::INatService> _natService;

        std::atomic<bool> _initialized{ false };
        std::atomic<bool> _workerStop{ false };
        std::thread _workerThread;
    };

} // namespace Multiplayer::Application

#endif /* !MULTIPLAYER_MANAGER_HPP_ */

