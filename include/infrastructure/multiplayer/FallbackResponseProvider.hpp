/*
** CrabeLoader
** File description:
** FallbackResponseProvider - Synthetic HTTP response generator for Disney Infinity
*/

#ifndef FALLBACK_RESPONSE_PROVIDER_HPP_
#define FALLBACK_RESPONSE_PROVIDER_HPP_

#include <mutex>
#include <string>
#include <string_view>

#include "domain/multiplayer/IFallbackResponseProvider.hpp"

namespace Multiplayer::Infrastructure {

    class FallbackResponseProvider final : public Domain::IFallbackResponseProvider {
    public:
        explicit FallbackResponseProvider();
        ~FallbackResponseProvider() override;

        FallbackResponseProvider(const FallbackResponseProvider&) = delete;
        FallbackResponseProvider& operator=(const FallbackResponseProvider&) = delete;
        FallbackResponseProvider(FallbackResponseProvider&&) = delete;
        FallbackResponseProvider& operator=(FallbackResponseProvider&&) = delete;

        [[nodiscard]] std::string buildResponse(std::wstring_view objectName) const override;
        void setDirectConnectPayload(std::string_view friendName, std::string_view locationString) override;
        void clearDirectConnectPayload() override;
        [[nodiscard]] bool hasDirectConnectPayload() const noexcept override;

    private:
        [[nodiscard]] std::string buildConfigResponse() const;
        [[nodiscard]] std::string buildAuthResponse() const;
        [[nodiscard]] std::string buildFriendsResponse() const;
        [[nodiscard]] std::string buildDefaultResponse() const;

        mutable std::mutex _mutex;
        std::string _directConnectFriend;
        std::string _directConnectLocation;
    };

} // namespace Multiplayer::Infrastructure

#endif /* !FALLBACK_RESPONSE_PROVIDER_HPP_ */

