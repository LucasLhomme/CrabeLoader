#pragma once

#include "domain/multiplayer/INatService.hpp"
#include <mutex>

namespace Crabe::Multiplayer {

    class UpnpNatService final : public INatService {
    public:
        explicit UpnpNatService();
        ~UpnpNatService() override;

        UpnpNatService(const UpnpNatService&) = delete;
        UpnpNatService& operator=(const UpnpNatService&) = delete;
        UpnpNatService(UpnpNatService&&) = delete;
        UpnpNatService& operator=(UpnpNatService&&) = delete;

        [[nodiscard]] std::expected<NatStatus, std::string> forwardPort(
            uint16_t port,
            std::string_view protocol = "UDP",
            std::string_view description = "Disney Infinity 3.0 P2P"
        ) override;

        [[nodiscard]] std::expected<void, std::string> releasePort(
            uint16_t port,
            std::string_view protocol = "UDP"
        ) override;

        [[nodiscard]] NatStatus getStatus() const noexcept override;

    private:
        [[nodiscard]] std::string getLocalIpAddress() const;
        [[nodiscard]] std::string resolveExternalIpViaStun() const;

        mutable std::mutex _mutex;
        NatStatus _status;
        uint16_t _forwardedPort{0};
        std::string _forwardedProtocol;
        bool _hasActiveLease{false};
        bool _comInitialized{false};
    };

} // namespace Crabe::Multiplayer
