/*
** CrabeLoader
** File description:
** IFallbackResponseProvider - Interface for generating synthetic backend HTTP responses
*/

#ifndef IFALLBACK_RESPONSE_PROVIDER_HPP_
#define IFALLBACK_RESPONSE_PROVIDER_HPP_

#include <string>
#include <string_view>

namespace Multiplayer::Domain {

    class IFallbackResponseProvider {
    public:
        virtual ~IFallbackResponseProvider() = default;

        [[nodiscard]] virtual std::string buildResponse(std::wstring_view objectName) const = 0;
        virtual void setDirectConnectPayload(std::string_view friendName, std::string_view locationString) = 0;
        virtual void clearDirectConnectPayload() = 0;
        [[nodiscard]] virtual bool hasDirectConnectPayload() const noexcept = 0;
    };

} // namespace Multiplayer::Domain

#endif /* !IFALLBACK_RESPONSE_PROVIDER_HPP_ */

