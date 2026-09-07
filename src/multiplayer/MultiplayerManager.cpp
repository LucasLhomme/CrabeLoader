/*
** CrabeLoader
** File description:
** MultiplayerManager implementation
*/

#include "loader/multiplayer/application/MultiplayerManager.hpp"

#include <chrono>
#include "loader/multiplayer/infrastructure/MemoryPatcher.hpp"
#include "loader/multiplayer/infrastructure/WinHttpRedirector.hpp"
#include "logger/logger.hpp"

namespace Multiplayer::Application {

    MultiplayerManager& MultiplayerManager::getInstance() noexcept {
        static MultiplayerManager instance;
        return instance;
    }

    MultiplayerManager::MultiplayerManager()
        : _patcher(std::make_unique<Infrastructure::MemoryPatcher>()),
          _redirector(std::make_unique<Infrastructure::WinHttpRedirector>())
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

    bool MultiplayerManager::arePatchesActive() const noexcept {
        return _patcher && _patcher->arePatchesActive();
    }

    bool MultiplayerManager::isRedirectorActive() const noexcept {
        return _redirector && _redirector->isInstalled();
    }

    unsigned MultiplayerManager::getPatchedCount() const noexcept {
        return _patcher ? _patcher->getPatchedCount() : 0;
    }

} // namespace Multiplayer::Application

