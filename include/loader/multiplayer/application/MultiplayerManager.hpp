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

        std::atomic<bool> _initialized{ false };
        std::atomic<bool> _workerStop{ false };
        std::thread _workerThread;
    };

} // namespace Multiplayer::Application

#endif /* !MULTIPLAYER_MANAGER_HPP_ */

