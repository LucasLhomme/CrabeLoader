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

} // namespace Multiplayer::Natives

#endif /* !MULTIPLAYER_NATIVES_HPP_ */

