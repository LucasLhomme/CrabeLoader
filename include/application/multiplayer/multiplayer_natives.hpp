/*
** CrabeLoader
** File description:
** multiplayer_natives - Lua C function bindings for Multiplayer subsystem
*/

#ifndef MULTIPLAYER_NATIVES_HPP_
#define MULTIPLAYER_NATIVES_HPP_

namespace Multiplayer::Natives {

    bool registerAll(void* L);

    int __cdecl getStatus(void* L);
    int __cdecl setTarget(void* L);
    int __cdecl applyPatches(void* L);
    int __cdecl getNatInfo(void* L);
    int __cdecl triggerPortForward(void* L);
    int __cdecl setDirectConnect(void* L);
    int __cdecl buildLocation(void* L);
    int __cdecl checkServerReachability(void* L);

    // Steamworks Natives
    int __cdecl steamIsAvailable(void* L);
    int __cdecl steamGetPersonaName(void* L);
    int __cdecl steamGetLocalId(void* L);
    int __cdecl steamCreateLobby(void* L);
    int __cdecl steamLeaveLobby(void* L);
    int __cdecl steamOpenInviteOverlay(void* L);
    int __cdecl steamGetLobbyStatus(void* L);
    int __cdecl steamGetFriends(void* L);

} // namespace Multiplayer::Natives

#endif /* !MULTIPLAYER_NATIVES_HPP_ */

