/*
** CrabeLoader
** File description:
** Declares the bridge to RealmManager_LoadSkyDomeInZone, the Script VM native that swaps a
** world's sky and lighting for a realm from realms/. No shipped script calls it.
** Knows no gameplay: it loads the realm it is given, Lua decides which and when.
**
** Authors: @LucasLhomme
*/

#ifndef ENGINE_SKY_HPP_
#define ENGINE_SKY_HPP_

#include <cstdint>
#include <string>

namespace crabe::infrastructure {

class EngineSky final {
public:
    static EngineSky& get();

    /// Clears the loaded skies and loads the realm `realmName` ("tbx_ala_skydome"), as the
    /// script native does. False when the build lacks the native or the call faulted (no
    /// world loaded, for one). Script thread only.
    [[nodiscard]] bool loadSkyDome(const std::string& realmName);

private:
    EngineSky() = default;

    void resolveOnce();

    bool _attempted{false};
    std::uintptr_t _handler{0};
};

} // namespace crabe::infrastructure

#endif
