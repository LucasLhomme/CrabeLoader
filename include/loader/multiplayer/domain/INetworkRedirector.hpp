/*
** CrabeLoader
** File description:
** INetworkRedirector - Interface for rerouting game HTTP/HTTPS traffic
*/

#ifndef INETWORK_REDIRECTOR_HPP_
#define INETWORK_REDIRECTOR_HPP_

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace Multiplayer::Domain {

    class INetworkRedirector {
    public:
        virtual ~INetworkRedirector() = default;

        [[nodiscard]] virtual std::expected<void, std::string> install() = 0;
        virtual void uninstall() = 0;

        virtual void setTarget(std::wstring_view host, uint16_t port) = 0;
        [[nodiscard]] virtual std::wstring_view getTargetHost() const noexcept = 0;
        [[nodiscard]] virtual uint16_t getTargetPort() const noexcept = 0;

        [[nodiscard]] virtual bool isInstalled() const noexcept = 0;
    };

} // namespace Multiplayer::Domain

#endif /* !INETWORK_REDIRECTOR_HPP_ */

