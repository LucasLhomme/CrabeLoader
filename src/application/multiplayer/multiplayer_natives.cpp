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

        if (ok) {
            Logger::getInstance().info("MultiplayerNatives: successfully bound Crabe._mp* natives.");
        } else {
            Logger::getInstance().warning("MultiplayerNatives: some native bindings failed.");
        }
        return ok;
    }

} // namespace Multiplayer::Natives
