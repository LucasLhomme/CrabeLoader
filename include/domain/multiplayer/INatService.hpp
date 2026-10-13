#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace Crabe::Multiplayer {

    struct NatStatus {
        bool upnpAvailable{false};
        bool portForwarded{false};
        uint16_t externalPort{0};
        uint16_t internalPort{0};
        std::string protocol{"UDP"};
        std::string localIp{"127.0.0.1"};
        std::string externalIp{""};
        std::string statusMessage{"Not initialized"};
    };

    class INatService {
    public:
        virtual ~INatService() = default;

        [[nodiscard]] virtual std::expected<NatStatus, std::string> forwardPort(
            uint16_t port,
            std::string_view protocol = "UDP",
            std::string_view description = "Disney Infinity 3.0 P2P"
        ) = 0;

        [[nodiscard]] virtual std::expected<void, std::string> releasePort(
            uint16_t port,
            std::string_view protocol = "UDP"
        ) = 0;

        [[nodiscard]] virtual NatStatus getStatus() const noexcept = 0;
    };

} // namespace Crabe::Multiplayer
