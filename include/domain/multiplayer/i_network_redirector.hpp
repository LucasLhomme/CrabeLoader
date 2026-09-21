/*
** CrabeLoader
** File description:
** The domain port for rerouting the game HTTP and HTTPS traffic to a reachable host.
** Declares intent only; which API is detoured, and how, is deliberately not visible here.
** Implemented by infrastructure/multiplayer/winhttp_redirector.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef INETWORK_REDIRECTOR_HPP_
#define INETWORK_REDIRECTOR_HPP_

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace crabe::multiplayer::domain {

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

} // namespace crabe::multiplayer::domain

#endif /* !INETWORK_REDIRECTOR_HPP_ */

