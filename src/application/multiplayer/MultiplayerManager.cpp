/*
** CrabeLoader
** File description:
** MultiplayerManager implementation
*/

#include "application/multiplayer/MultiplayerManager.hpp"

#include <chrono>
#include <ws2tcpip.h>
#include "infrastructure/multiplayer/MemoryPatcher.hpp"
#include "infrastructure/multiplayer/WinHttpRedirector.hpp"
#include "infrastructure/multiplayer/UpnpNatService.hpp"
#include "infrastructure/multiplayer/SteamworksService.hpp"
#include "shared/logger.hpp"

namespace Multiplayer::Application {

    MultiplayerManager& MultiplayerManager::getInstance() noexcept {
        static MultiplayerManager instance;
        return instance;
    }

    MultiplayerManager::MultiplayerManager()
        : _patcher(std::make_unique<Infrastructure::MemoryPatcher>()),
          _redirector(std::make_unique<Infrastructure::WinHttpRedirector>()),
          _natService(std::make_unique<Crabe::Multiplayer::UpnpNatService>()),
          _steamService(std::make_unique<Crabe::Multiplayer::SteamworksService>())
    {
    }

    MultiplayerManager::~MultiplayerManager() {
        uninitialize();
    }

    bool MultiplayerManager::initialize() {
        if (_initialized.load()) {
            return true;
        }

        Logger::getInstance().info("MultiplayerManager: initializing subsystem...");

        // 1. Install WinHttp Redirector
        if (_redirector) {
            auto res = _redirector->install();
            if (!res) {
                Logger::getInstance().warning("MultiplayerManager: redirector install warning: {}", res.error());
            }
        }

        // 2. Start background worker for memory patches (retrying until unpacked)
        _workerStop = false;
        _workerThread = std::thread(&MultiplayerManager::patchWorkerThread, this);

        // 3. Request automatic UPnP port forwarding for Quazal Net-Z P2P (3074 UDP) in background
        if (_natService) {
            std::thread([this]() {
                (void)_natService->forwardPort(3074, "UDP", "Disney Infinity 3.0 P2P");
            }).detach();
        }

        // 4. Initialize Steamworks Integration
        if (_steamService) {
            _steamService->initialize();
            (void)_steamService->initialize();
        }

        _initialized = true;
        Logger::getInstance().info("MultiplayerManager: initialized successfully.");
        return true;
    }

    void MultiplayerManager::uninitialize() {
        if (!_initialized.load()) {
            return;
        }

        _workerStop = true;
        if (_workerThread.joinable()) {
            _workerThread.join();
        }

        if (_steamService) {
            _steamService->shutdown();
        }

        if (_natService) {
            (void)_natService->releasePort(3074, "UDP");
        }

        if (_patcher) {
            _patcher->restorePatches();
        }

        if (_redirector) {
            _redirector->uninstall();
        }

        _initialized = false;
        Logger::getInstance().info("MultiplayerManager: uninitialized.");
    }

    void MultiplayerManager::patchWorkerThread() {
        Logger::getInstance().debug("MultiplayerManager: patch worker thread started");

        // Try applying patches over a 30-second window
        for (int i = 0; i < 60 && !_workerStop.load(); ++i) {
            if (_patcher) {
                auto res = _patcher->applyPatches();
                if (res && _patcher->arePatchesActive() && _patcher->getPatchedCount() >= 4) {
                    Logger::getInstance().info("MultiplayerManager: all core memory patches confirmed active (pass {})", i + 1);
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    void MultiplayerManager::setTargetServer(std::wstring_view host, uint16_t port) {
        if (_redirector) {
            _redirector->setTarget(host, port);
        }
    }

    std::wstring MultiplayerManager::getTargetHost() const {
        return _redirector ? std::wstring(_redirector->getTargetHost()) : L"127.0.0.1";
    }

    uint16_t MultiplayerManager::getTargetPort() const {
        return _redirector ? _redirector->getTargetPort() : 3000;
    }

    bool MultiplayerManager::isServerReachable(std::string_view host, uint16_t port, uint32_t timeoutMs) const {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            return false;
        }

        u_long nonBlocking = 1;
        ioctlsocket(sock, FIONBIO, &nonBlocking);

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        std::string hostStr(host);
        if (inet_pton(AF_INET, hostStr.c_str(), &addr.sin_addr) != 1) {
            addrinfo hints{}, *res = nullptr;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            if (getaddrinfo(hostStr.c_str(), std::to_string(port).c_str(), &hints, &res) != 0 || !res) {
                closesocket(sock);
                return false;
            }
            addr = *reinterpret_cast<sockaddr_in*>(res->ai_addr);
            freeaddrinfo(res);
        }

        connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

        fd_set writeFds, errFds;
        FD_ZERO(&writeFds);
        FD_ZERO(&errFds);
        FD_SET(sock, &writeFds);
        FD_SET(sock, &errFds);

        timeval tv{};
        tv.tv_sec = static_cast<long>(timeoutMs / 1000);
        tv.tv_usec = static_cast<long>((timeoutMs % 1000) * 1000);

        int sel = select(0, nullptr, &writeFds, &errFds, &tv);
        bool reachable = false;
        if (sel > 0 && FD_ISSET(sock, &writeFds) && !FD_ISSET(sock, &errFds)) {
            int optVal = 0;
            int optLen = sizeof(optVal);
            if (getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&optVal), &optLen) == 0) {
                reachable = (optVal == 0);
            }
        }

        closesocket(sock);
        return reachable;
    }

    bool MultiplayerManager::arePatchesActive() const noexcept {
        return _patcher && _patcher->arePatchesActive();
    }

    bool MultiplayerManager::isRedirectorActive() const noexcept {
        return _redirector && _redirector->isInstalled();
    }

    unsigned MultiplayerManager::getPatchedCount() const noexcept {
        return _patcher ? _patcher->getPatchedCount() : 0;
    }

    Crabe::Multiplayer::NatStatus MultiplayerManager::getNatStatus() const noexcept {
        if (_natService) {
            return _natService->getStatus();
        }
        return {};
    }

    bool MultiplayerManager::triggerPortForward(uint16_t port, std::string_view protocol) {
        if (_natService) {
            auto res = _natService->forwardPort(port, protocol);
            return res.has_value() && res->portForwarded;
        }
        return false;
    }

    void MultiplayerManager::releasePortForward(uint16_t port, std::string_view protocol) {
        if (_natService) {
            (void)_natService->releasePort(port, protocol);
        }
    }

    std::string MultiplayerManager::ipv4ToHex(std::string_view ip) {
        std::string ipStr(ip);
        in_addr addr{};
        if (inet_pton(AF_INET, ipStr.c_str(), &addr) == 1) {
            auto bytes = reinterpret_cast<const uint8_t*>(&addr.s_addr);
            uint32_t val = static_cast<uint32_t>(bytes[0]) |
                           (static_cast<uint32_t>(bytes[1]) << 8) |
                           (static_cast<uint32_t>(bytes[2]) << 16) |
                           (static_cast<uint32_t>(bytes[3]) << 24);
            char buf[16]{};
            snprintf(buf, sizeof(buf), "%X", val);
            return buf;
        }
        return "100007F";
    }

    std::string MultiplayerManager::portToHex(uint16_t port) {
        char buf[16]{};
        snprintf(buf, sizeof(buf), "%X", port);
        return buf;
    }

    std::string MultiplayerManager::formatLocationString(
        std::string_view pubIp, uint16_t pubPort,
        std::string_view privIp, uint16_t privPort,
        std::string_view hostDid, std::string_view gameName) const
    {
        std::string hexPubIp = ipv4ToHex(pubIp);
        std::string hexPubPort = portToHex(pubPort);
        std::string hexPrivIp = ipv4ToHex(privIp.empty() ? pubIp : privIp);
        std::string hexPrivPort = portToHex(privPort == 0 ? pubPort : privPort);
        std::string did(hostDid.empty() ? "{00000000-0000-0000-0000-000000000000}" : hostDid);
        std::string gName(gameName.empty() ? "IN2PC" : gameName);

        char buf[512]{};
        snprintf(buf, sizeof(buf),
            "{\"Pu\":{\"IP\":\"%s\",\"P\":\"%s\"},\"Pr\":{\"IP\":\"%s\",\"P\":\"%s\"},\"Host\":{\"DID\":\"%s\"},\"J\":1,\"L\":0,\"GameName\":\"%s\"}",
            hexPubIp.c_str(), hexPubPort.c_str(),
            hexPrivIp.c_str(), hexPrivPort.c_str(),
            did.c_str(), gName.c_str());
        return buf;
    }

    void MultiplayerManager::setDirectConnectTarget(std::string_view friendName, std::string_view ip, uint16_t port, std::string_view hostDid) {
        std::string location = formatLocationString(ip, port, ip, port, hostDid, "IN2PC");
        auto* redirector = dynamic_cast<Infrastructure::WinHttpRedirector*>(_redirector.get());
        if (redirector) {
            redirector->setDirectConnectPayload(friendName, location);
        }
        Logger::getInstance().info("MultiplayerManager: primed Direct Connect target '{}' at {}:{}", friendName, ip, port);
    }

    bool MultiplayerManager::isSteamAvailable() const noexcept {
        return _steamService && _steamService->isAvailable();
    }

    std::string MultiplayerManager::getSteamPersonaName() const {
        return _steamService ? _steamService->getPersonaName() : "Player";
    }

    uint64_t MultiplayerManager::getSteamId() const noexcept {
        return _steamService ? _steamService->getLocalSteamId() : 0;
    }

    bool MultiplayerManager::createSteamLobby(bool friendsOnly, int maxMembers) {
        if (_steamService) {
            return _steamService->createLobby(friendsOnly, maxMembers);
        }
        return false;
    }

    void MultiplayerManager::leaveSteamLobby() {
        if (_steamService) {
            _steamService->leaveLobby();
        }
    }

    bool MultiplayerManager::openSteamInviteOverlay() {
        if (_steamService) {
            return _steamService->openInviteOverlay();
        }
        return false;
    }

    Crabe::Multiplayer::SteamLobbyStatus MultiplayerManager::getSteamLobbyStatus() const noexcept {
        if (_steamService) {
            return _steamService->getLobbyStatus();
        }
        return {};
    }

    int MultiplayerManager::getSteamFriendCount() const noexcept {
        return _steamService ? _steamService->getFriendCount() : 0;
    }

    std::vector<Crabe::Multiplayer::SteamFriendInfo> MultiplayerManager::getSteamFriends() const {
        if (_steamService) {
            return _steamService->getFriends();
        }
        return {};
    }

    void MultiplayerManager::updateSteamLobbyLocation(std::string_view locationData) {
        if (_steamService) {
            _steamService->setLobbyData("location", locationData);
        }
    }

} // namespace Multiplayer::Application

