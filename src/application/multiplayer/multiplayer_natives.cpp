/*
** CrabeLoader
** File description:
** multiplayer_natives implementation
*/

#include "application/multiplayer/multiplayer_natives.hpp"

#include <string>
#include <windows.h>

#include "infrastructure/luacall.hpp"
#include "application/multiplayer/MultiplayerManager.hpp"
#include "shared/logger.hpp"

namespace Multiplayer::Natives {

    int __cdecl getStatus(void* L) {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) {
            return 0;
        }

        auto& mp = Application::MultiplayerManager::getInstance();
        bool patchesActive = mp.arePatchesActive();
        unsigned patchedCount = mp.getPatchedCount();
        bool redirectorActive = mp.isRedirectorActive();
        std::wstring hostW = mp.getTargetHost();
        uint16_t port = mp.getTargetPort();

        char hostA[128]{};
        WideCharToMultiByte(CP_UTF8, 0, hostW.c_str(), -1, hostA, sizeof(hostA), nullptr, nullptr);

        // Return multiple values: patchesActive (1/0), patchedCount (num), redirectorActive (1/0), host (string), port (num)
        lua.pushNumber(L, patchesActive ? 1.0 : 0.0);
        lua.pushNumber(L, static_cast<double>(patchedCount));
        lua.pushNumber(L, redirectorActive ? 1.0 : 0.0);
        lua.pushString(L, hostA);
        lua.pushNumber(L, static_cast<double>(port));
        return 5;
    }

    int __cdecl setTarget(void* L) {
        LuaCall& lua = LuaCall::get();
        const char* hostStr = lua.argToString(L, 1);
        double portNum = lua.argToNumber(L, 2, 3000.0);

        if (!hostStr) {
            return 0;
        }

        wchar_t hostW[128]{};
        MultiByteToWideChar(CP_UTF8, 0, hostStr, -1, hostW, sizeof(hostW) / sizeof(wchar_t));

        auto& mp = Application::MultiplayerManager::getInstance();
        mp.setTargetServer(hostW, static_cast<uint16_t>(portNum));

        Logger::getInstance().info("MultiplayerNatives: updated target server to {}:{}", hostStr, static_cast<int>(portNum));
        if (lua.hasReturnSupport()) {
            lua.pushNumber(L, 1.0);
            return 1;
        }
        return 0;
    }

    int __cdecl applyPatches(void* L) {
        LuaCall& lua = LuaCall::get();
        auto& mp = Application::MultiplayerManager::getInstance();
        bool ok = mp.initialize();

        if (lua.hasReturnSupport()) {
            lua.pushNumber(L, ok ? 1.0 : 0.0);
            return 1;
        }
        return 0;
    }

    int __cdecl getNatInfo(void* L) {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) {
            return 0;
        }

        auto& mp = Application::MultiplayerManager::getInstance();
        auto nat = mp.getNatStatus();

        // Return: upnpAvailable, portForwarded, externalIp, externalPort, localIp, internalPort, statusMsg
        lua.pushNumber(L, nat.upnpAvailable ? 1.0 : 0.0);
        lua.pushNumber(L, nat.portForwarded ? 1.0 : 0.0);
        lua.pushString(L, nat.externalIp.c_str());
        lua.pushNumber(L, static_cast<double>(nat.externalPort));
        lua.pushString(L, nat.localIp.c_str());
        lua.pushNumber(L, static_cast<double>(nat.internalPort));
        lua.pushString(L, nat.statusMessage.c_str());
        return 7;
    }

    int __cdecl triggerPortForward(void* L) {
        LuaCall& lua = LuaCall::get();
        double port = lua.argToNumber(L, 1, 3074.0);
        const char* proto = lua.argToString(L, 2);

        auto& mp = Application::MultiplayerManager::getInstance();
        bool ok = mp.triggerPortForward(static_cast<uint16_t>(port), proto ? proto : "UDP");

        if (lua.hasReturnSupport()) {
            lua.pushNumber(L, ok ? 1.0 : 0.0);
            return 1;
        }
        return 0;
    }

    int __cdecl setDirectConnect(void* L) {
        LuaCall& lua = LuaCall::get();
        const char* friendName = lua.argToString(L, 1);
        const char* ip = lua.argToString(L, 2);
        double port = lua.argToNumber(L, 3, 3074.0);
        const char* hostDid = lua.argToString(L, 4);

        if (!friendName || !ip) {
            return 0;
        }

        auto& mp = Application::MultiplayerManager::getInstance();
        mp.setDirectConnectTarget(friendName, ip, static_cast<uint16_t>(port), hostDid ? hostDid : "");

        if (lua.hasReturnSupport()) {
            lua.pushNumber(L, 1.0);
            return 1;
        }
        return 0;
    }

    int __cdecl buildLocation(void* L) {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) {
            return 0;
        }
        const char* pubIp = lua.argToString(L, 1);
        double pubPort = lua.argToNumber(L, 2, 3074.0);
        const char* privIp = lua.argToString(L, 3);
        double privPort = lua.argToNumber(L, 4, pubPort);
        const char* hostDid = lua.argToString(L, 5);
        const char* gameName = lua.argToString(L, 6);

        auto& mp = Application::MultiplayerManager::getInstance();
        std::string loc = mp.formatLocationString(
            pubIp ? pubIp : "127.0.0.1", static_cast<uint16_t>(pubPort),
            privIp ? privIp : "", static_cast<uint16_t>(privPort),
            hostDid ? hostDid : "", gameName ? gameName : "IN2PC");

        lua.pushString(L, loc.c_str());
        return 1;
    }

    int __cdecl checkServerReachability(void* L) {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        const char* hostStr = lua.argToString(L, 1);
        double portNum = lua.argToNumber(L, 2, 3000.0);
        double timeoutMs = lua.argToNumber(L, 3, 250.0);

        auto& mp = Application::MultiplayerManager::getInstance();
        std::string host = hostStr ? hostStr : "127.0.0.1";
        bool reachable = mp.isServerReachable(host, static_cast<uint16_t>(portNum), static_cast<uint32_t>(timeoutMs));

        lua.pushNumber(L, reachable ? 1.0 : 0.0);
        return 1;
    }

    int __cdecl steamIsAvailable(void* L) {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto& mp = Application::MultiplayerManager::getInstance();
        lua.pushNumber(L, mp.isSteamAvailable() ? 1.0 : 0.0);
        return 1;
    }

    int __cdecl steamGetPersonaName(void* L) {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto& mp = Application::MultiplayerManager::getInstance();
        std::string name = mp.getSteamPersonaName();
        lua.pushString(L, name.c_str());
        return 1;
    }

    int __cdecl steamGetLocalId(void* L) {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto& mp = Application::MultiplayerManager::getInstance();
        uint64_t id = mp.getSteamId();
        lua.pushString(L, std::to_string(id).c_str());
        return 1;
    }

    int __cdecl steamCreateLobby(void* L) {
        LuaCall& lua = LuaCall::get();
        bool friendsOnly = lua.argToNumber(L, 1, 1.0) != 0.0;
        double maxMembers = lua.argToNumber(L, 2, 4.0);

        auto& mp = Application::MultiplayerManager::getInstance();
        bool ok = mp.createSteamLobby(friendsOnly, static_cast<int>(maxMembers));

        if (lua.hasReturnSupport()) {
            lua.pushNumber(L, ok ? 1.0 : 0.0);
            return 1;
        }
        return 0;
    }

    int __cdecl steamLeaveLobby(void* L) {
        (void)L;
        auto& mp = Application::MultiplayerManager::getInstance();
        mp.leaveSteamLobby();
        return 0;
    }

    int __cdecl steamOpenInviteOverlay(void* L) {
        LuaCall& lua = LuaCall::get();
        auto& mp = Application::MultiplayerManager::getInstance();
        bool ok = mp.openSteamInviteOverlay();

        if (lua.hasReturnSupport()) {
            lua.pushNumber(L, ok ? 1.0 : 0.0);
            return 1;
        }
        return 0;
    }

    int __cdecl steamGetLobbyStatus(void* L) {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto& mp = Application::MultiplayerManager::getInstance();
        auto st = mp.getSteamLobbyStatus();

        // Return: inLobby (1/0), isHost (1/0), lobbyId (string), hostId (string), memberCount (num), memberLimit (num)
        lua.pushNumber(L, st.inLobby ? 1.0 : 0.0);
        lua.pushNumber(L, st.isHost ? 1.0 : 0.0);
        lua.pushString(L, std::to_string(st.lobbyId).c_str());
        lua.pushString(L, std::to_string(st.hostSteamId).c_str());
        lua.pushNumber(L, static_cast<double>(st.memberCount));
        lua.pushNumber(L, static_cast<double>(st.memberLimit));
        return 6;
    }

    int __cdecl steamGetFriendCount(void* L) {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto& mp = Application::MultiplayerManager::getInstance();
        int count = mp.getSteamFriendCount();
        lua.pushNumber(L, static_cast<double>(count));
        return 1;
    }

    int __cdecl steamGetFriends(void* L) {
        LuaCall& lua = LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto& mp = Application::MultiplayerManager::getInstance();
        auto friends = mp.getSteamFriends();

        // Return count of friends as simple confirmation
        lua.pushNumber(L, static_cast<double>(friends.size()));
        return 1;
    }

    bool registerAll(void* L) {
        LuaCall& lua = LuaCall::get();
        bool ok = true;

        ok = lua.registerNativeFunction(L, "Crabe", "_mpGetStatus", &getStatus) && ok;
        ok = lua.registerNativeFunction(L, "Crabe", "_mpSetTarget", &setTarget) && ok;
        ok = lua.registerNativeFunction(L, "Crabe", "_mpApplyPatches", &applyPatches) && ok;
        ok = lua.registerNativeFunction(L, "Crabe", "_mpGetNatInfo", &getNatInfo) && ok;
        ok = lua.registerNativeFunction(L, "Crabe", "_mpTriggerPortForward", &triggerPortForward) && ok;
        ok = lua.registerNativeFunction(L, "Crabe", "_mpSetDirectConnect", &setDirectConnect) && ok;
        ok = lua.registerNativeFunction(L, "Crabe", "_mpBuildLocation", &buildLocation) && ok;
        ok = lua.registerNativeFunction(L, "Crabe", "_mpCheckServerReachability", &checkServerReachability) && ok;

        // Steamworks bindings
        ok = lua.registerNativeFunction(L, "Crabe", "_steamIsAvailable", &steamIsAvailable) && ok;
        ok = lua.registerNativeFunction(L, "Crabe", "_steamGetPersonaName", &steamGetPersonaName) && ok;
        ok = lua.registerNativeFunction(L, "Crabe", "_steamGetLocalId", &steamGetLocalId) && ok;
        ok = lua.registerNativeFunction(L, "Crabe", "_steamCreateLobby", &steamCreateLobby) && ok;
        ok = lua.registerNativeFunction(L, "Crabe", "_steamLeaveLobby", &steamLeaveLobby) && ok;
        ok = lua.registerNativeFunction(L, "Crabe", "_steamOpenInviteOverlay", &steamOpenInviteOverlay) && ok;
        ok = lua.registerNativeFunction(L, "Crabe", "_steamGetLobbyStatus", &steamGetLobbyStatus) && ok;
        ok = lua.registerNativeFunction(L, "Crabe", "_steamGetFriendCount", &steamGetFriendCount) && ok;
        ok = lua.registerNativeFunction(L, "Crabe", "_steamGetFriends", &steamGetFriends) && ok;

        if (ok) {
            Logger::getInstance().info("MultiplayerNatives: successfully bound Crabe._mp* and Crabe._steam* natives.");
        } else {
            Logger::getInstance().warning("MultiplayerNatives: some native bindings failed.");
        }
        return ok;
    }

} // namespace Multiplayer::Natives
