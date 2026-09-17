/*
** CrabeLoader
** File description:
** FallbackResponseProvider implementation
*/

#include "infrastructure/multiplayer/FallbackResponseProvider.hpp"

#include <format>
#include "shared/logger.hpp"

namespace Multiplayer::Infrastructure {

    FallbackResponseProvider::FallbackResponseProvider() = default;

    FallbackResponseProvider::~FallbackResponseProvider() = default;

    std::string FallbackResponseProvider::buildConfigResponse() const {
        return "{\"url_inf_save\":\"http://127.0.0.1:3000/infinity/save/v1/steam/\","
               "\"url_inf_ticker\":\"http://127.0.0.1:3000/infinity/ticker/v1/steam/\","
               "\"url_inf_ugc\":\"http://127.0.0.1:3000/infinity/ugc/v2/steam/\","
               "\"url_inf_profile\":\"http://127.0.0.1:3000/infinity/profile/v2/steam/\","
               "\"url_inf_leaderboard\":\"http://127.0.0.1:3000/infinity/leaderboard/v1/steam/\","
               "\"url_inf_entitlement\":\"http://127.0.0.1:3000/infinity/entitlement/v1/steam\","
               "\"domain_cg_natneg\":\"127.0.0.1\","
               "\"url_cg_did_create\":\"http://127.0.0.1:3000\","
               "\"url_cg_friends\":\"http://127.0.0.1:3000/coregames/friends/v1/steam\","
               "\"url_cg_matchmaking\":\"http://127.0.0.1:3000/coregames/matchmaking/v1\"}";
    }

    std::string FallbackResponseProvider::buildAuthResponse() const {
        return "{\"status\":0,\"token\":\"crabe_mp_guest_token\","
               "\"swid\":\"{00000000-0000-0000-0000-000000000001}\"}";
    }

    std::string FallbackResponseProvider::buildFriendsResponse() const {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_directConnectLocation.empty()) {
            return "{\"status\":0,\"success\":true,\"items\":[],\"friends\":[],\"total\":0,\"count\":0}";
        }

        std::string fname = _directConnectFriend.empty() ? "DirectPeer" : _directConnectFriend;
        std::string escapedLoc;
        for (char c : _directConnectLocation) {
            if (c == '"') {
                escapedLoc += "\\\"";
            } else {
                escapedLoc += c;
            }
        }

        return std::format(
            "{{\"status\":0,\"success\":true,\"items\":[{{\"name\":\"{}\",\"username\":\"{}\","
            "\"inputName\":\"{}\",\"locationString\":\"{}\",\"location\":\"{}\",\"joinable\":1,\"locked\":0}}],"
            "\"friends\":[{{\"name\":\"{}\",\"locationString\":\"{}\"}}],\"total\":1,\"count\":1}}",
            fname, fname, fname, escapedLoc, escapedLoc, fname, escapedLoc);
    }

    std::string FallbackResponseProvider::buildDefaultResponse() const {
        return "{\"status\":0}";
    }

    std::string FallbackResponseProvider::buildResponse(std::wstring_view objectName) const {
        if (objectName.find(L"config") != std::wstring_view::npos) {
            return buildConfigResponse();
        }
        if (objectName.find(L"authenticate") != std::wstring_view::npos ||
            objectName.find(L"did") != std::wstring_view::npos) {
            return buildAuthResponse();
        }
        if (objectName.find(L"friends") != std::wstring_view::npos) {
            return buildFriendsResponse();
        }
        return buildDefaultResponse();
    }

    void FallbackResponseProvider::setDirectConnectPayload(std::string_view friendName, std::string_view locationString) {
        std::lock_guard<std::mutex> lock(_mutex);
        _directConnectFriend = std::string(friendName);
        _directConnectLocation = std::string(locationString);
        Logger::getInstance().info("FallbackResponseProvider: primed payload for friend '{}'", friendName);
    }

    void FallbackResponseProvider::clearDirectConnectPayload() {
        std::lock_guard<std::mutex> lock(_mutex);
        _directConnectFriend.clear();
        _directConnectLocation.clear();
    }

    bool FallbackResponseProvider::hasDirectConnectPayload() const noexcept {
        std::lock_guard<std::mutex> lock(_mutex);
        return !_directConnectLocation.empty();
    }

} // namespace Multiplayer::Infrastructure

