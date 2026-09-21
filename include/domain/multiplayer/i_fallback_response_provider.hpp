/*
** CrabeLoader
** File description:
** The domain port for the backend HTTP responses the retired Disney servers used to send.
** Declares shape only: what a response must contain, never how it is produced or carried.
** Implemented by infrastructure/multiplayer/fallback_response_provider.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef IFALLBACK_RESPONSE_PROVIDER_HPP_
#define IFALLBACK_RESPONSE_PROVIDER_HPP_

#include <string>
#include <string_view>

namespace crabe::multiplayer::domain {

    class IFallbackResponseProvider {
    public:
        virtual ~IFallbackResponseProvider() = default;

        [[nodiscard]] virtual std::string buildResponse(std::wstring_view objectName) const = 0;
        virtual void setDirectConnectPayload(std::string_view friendName, std::string_view locationString) = 0;
        virtual void clearDirectConnectPayload() = 0;
        [[nodiscard]] virtual bool hasDirectConnectPayload() const noexcept = 0;
    };

} // namespace crabe::multiplayer::domain

#endif /* !IFALLBACK_RESPONSE_PROVIDER_HPP_ */

