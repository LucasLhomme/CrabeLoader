/*
** CrabeLoader
** File description:
** cheats -- god mode and movement speed
*/

#include "loader/cheats.hpp"

#include <cstring>
#include <mutex>
#include <vector>
#include <windows.h>

#include "loader/codecave.hpp"
#include "loader/luacall.hpp"
#include "loader/memory.hpp"
#include "logger/logger.hpp"

namespace {

    // Entity struct, as seen at both hook sites: a float health at offset
    // 0x08, followed by a float maximum health at offset 0x0C. Only the
    // player's maximum ever reaches 150, which is what the capture cave
    // tests -- there is no type tag to key off.
    constexpr uintptr_t kHealthOffset = 0x08;
    constexpr float kPlayerMaxHealth = 150.0f;

    // movss xmm0, [eax+0x08] -- the HUD reads the player's health here, so eax
    // is the object we want. 5 bytes, unique in .text.
    const char* kCapturePattern = "F3 0F 10 40 08 F3 0F 5E 40 0C 0F 2F C8 77";

    // movss xmm1, [esi+0x0C] -- damage apply: esi takes the hit, xmm0 already
    // holds its new health. 5 bytes, unique in .text.
    const char* kClampPattern = "F3 0F 10 4E 0C 0F 2F C1 57";

    // The four analog-stick-to-velocity stores, movss [eax+0x298], xmm0 (Y)
    // and movss [eax+0x29C], xmm0 (X), 8 bytes each. The "start moving" pair
    // begins with the store; in the "already moving" pair the store sits 6
    // bytes in, after a movss xmm0, [esp+0x08].
    struct SpeedSite {
        const char* pattern;
        size_t storeOffset;
        const char* name;
    };

    constexpr SpeedSite kSpeedSites[] = {
        { "F3 0F 11 80 98 02 00 00 8B 4E 0C", 0, "velocity Y (start)" },
        { "F3 0F 11 80 9C 02 00 00 8B 4E 0C", 0, "velocity X (start)" },
        { "F3 0F 10 44 24 08 F3 0F 11 80 98 02 00 00 C2 0C 00", 6, "velocity Y (moving)" },
        { "F3 0F 10 44 24 08 F3 0F 11 80 9C 02 00 00 C2 0C 00", 6, "velocity X (moving)" },
    };

    constexpr size_t kSpeedSiteCount = sizeof(kSpeedSites) / sizeof(kSpeedSites[0]);
    constexpr size_t kSpeedStoreLength = 8;
    constexpr size_t kDamageStoreLength = 5;

    // A wild multiplier launches the avatar out of the level geometry and the
    // stream never recovers, so refuse anything past a usable range.
    constexpr float kMinMultiplier = 0.1f;
    constexpr float kMaxMultiplier = 50.0f;

    // Read by the caves, written from here. g_player is stored by cave code,
    // so it must be a plain address the assembled `mov` can reach.
    uintptr_t g_player = 0;
    volatile LONG g_enabled = 0;
    float g_multiplier = 1.0f;

    CodeCave g_captureCave;
    CodeCave g_clampCave;
    CodeCave g_speedCaves[kSpeedSiteCount];

    bool g_godReady = false;
    bool g_speedReady = false;
    std::mutex g_installMutex;

    // Capture cave: if maxHealth >= 150, remember eax as the player object.
    // Clobbers xmm0 and the flags; the stolen movss reloads xmm0 right after,
    // and pushfd/popfd puts the flags back for the comiss that follows.
    std::vector<uint8_t> buildCaptureBody()
    {
        constexpr size_t kThresholdAt = 7;
        constexpr size_t kPlayerAt = 23;

        std::vector<uint8_t> body = {
            0x9C,                                     // pushfd
            0x83, 0xEC, 0x04,                         // sub esp, 4
            0xC7, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00, // mov dword ptr [esp], 150.0f
            0xF3, 0x0F, 0x10, 0x40, 0x0C,             // movss xmm0, [eax+0Ch]
            0x0F, 0x2F, 0x04, 0x24,                   // comiss xmm0, [esp]
            0x72, 0x05,                               // jb over the store
            0xA3, 0x00, 0x00, 0x00, 0x00,             // mov [g_player], eax
            0x83, 0xC4, 0x04,                         // add esp, 4
            0x9D,                                     // popfd
        };

        uint32_t threshold = 0;
        std::memcpy(&threshold, &kPlayerMaxHealth, sizeof(threshold));
        CodeCave::putU32(body, kThresholdAt, threshold);
        CodeCave::putU32(body, kPlayerAt, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_player)));
        return body;
    }

    // Clamp cave: when enabled and esi is the captured player, replace the
    // incoming health (xmm0) with the current one, so the store that follows
    // is a no-op. A higher xmm0 is left alone -- healing still works.
    std::vector<uint8_t> buildClampBody()
    {
        constexpr size_t kEnabledAt = 4;
        constexpr size_t kPlayerAt = 12;

        std::vector<uint8_t> body = {
            0x50,                                     // push eax
            0x9C,                                     // pushfd
            0x83, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, // cmp dword ptr [g_enabled], 0
            0x74, 0x18,                               // je done
            0xA1, 0x00, 0x00, 0x00, 0x00,             // mov eax, [g_player]
            0x85, 0xC0,                               // test eax, eax
            0x74, 0x0F,                               // je done
            0x3B, 0xC6,                               // cmp eax, esi
            0x75, 0x0B,                               // jne done
            0x0F, 0x2F, 0x46, 0x08,                   // comiss xmm0, [esi+08h]
            0x73, 0x05,                               // jae done
            0xF3, 0x0F, 0x10, 0x46, 0x08,             // movss xmm0, [esi+08h]
            0x9D,                                     // done: popfd
            0x58,                                     // pop eax
        };

        CodeCave::putU32(body, kEnabledAt, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_enabled)));
        CodeCave::putU32(body, kPlayerAt, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_player)));
        return body;
    }

    // Speed cave: scale the velocity the game just computed, before its store.
    std::vector<uint8_t> buildSpeedBody()
    {
        constexpr size_t kMultiplierAt = 4;

        std::vector<uint8_t> body = {
            0xF3, 0x0F, 0x59, 0x05, 0x00, 0x00, 0x00, 0x00, // mulss xmm0, [g_multiplier]
        };

        CodeCave::putU32(body, kMultiplierAt, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_multiplier)));
        return body;
    }

} // namespace

bool Cheats::installGodMode()
{
    std::lock_guard<std::mutex> lock(g_installMutex);
    if (g_godReady) return true;

    uintptr_t capture = Memory::patternScan(kCapturePattern);
    uintptr_t clamp = Memory::patternScan(kClampPattern);

    if (!capture || !clamp) {
        Logger::getInstance().error("Cheats: god mode AOB not found (capture=0x{:X}, clamp=0x{:X}).",
                                    capture, clamp);
        return false;
    }

    // Both or neither: a capture without a clamp silently does nothing, and a
    // clamp without a capture would compare against a null player pointer.
    if (!g_captureCave.installLogged(capture, buildCaptureBody(), kDamageStoreLength,
                                     "Cheats", "player capture")) {
        return false;
    }
    if (!g_clampCave.installLogged(clamp, buildClampBody(), kDamageStoreLength,
                                   "Cheats", "damage clamp")) {
        g_captureCave.remove();
        return false;
    }

    g_godReady = true;
    Logger::getInstance().info("Cheats: god mode caves installed.");
    return true;
}

bool Cheats::setGodMode(bool enabled)
{
    if (!installGodMode()) return false;

    InterlockedExchange(&g_enabled, enabled ? 1 : 0);
    return true;
}

bool Cheats::godMode()
{
    return g_enabled != 0;
}

uintptr_t Cheats::playerObject()
{
    return g_player;
}

bool Cheats::readPlayerFloat(uintptr_t offset, float& out)
{
    uintptr_t object = g_player;

    if (!object) return false;
    if (!Memory::isReadable(object + offset, sizeof(float))) return false;

    std::memcpy(&out, reinterpret_cast<const void*>(object + offset), sizeof(float));
    return true;
}

bool Cheats::installSpeed()
{
    std::lock_guard<std::mutex> lock(g_installMutex);
    if (g_speedReady) return true;

    uintptr_t sites[kSpeedSiteCount] = {};
    for (size_t i = 0; i < kSpeedSiteCount; ++i) {
        uintptr_t base = Memory::patternScan(kSpeedSites[i].pattern);
        sites[i] = base ? base + kSpeedSites[i].storeOffset : 0;

        if (!base) {
            Logger::getInstance().error("Cheats: speed AOB not found for {}.", kSpeedSites[i].name);
            return false;
        }
    }

    // Partial installation would scale one axis and not the other, which is
    // worse than no cheat at all, so roll back if any site refuses.
    for (size_t i = 0; i < kSpeedSiteCount; ++i) {
        if (g_speedCaves[i].installLogged(sites[i], buildSpeedBody(), kSpeedStoreLength,
                                          "Cheats", kSpeedSites[i].name)) {
            continue;
        }
        for (size_t j = 0; j < i; ++j) g_speedCaves[j].remove();
        return false;
    }

    g_speedReady = true;
    Logger::getInstance().info("Cheats: speed caves installed ({} sites).", kSpeedSiteCount);
    return true;
}

bool Cheats::setSpeedMultiplier(float multiplier)
{
    if (multiplier < kMinMultiplier) multiplier = kMinMultiplier;
    if (multiplier > kMaxMultiplier) multiplier = kMaxMultiplier;

    // Nothing to patch while the multiplier is 1.0: leave the game's code
    // alone until the player actually asks for a change.
    if (multiplier == 1.0f && !g_speedReady) {
        g_multiplier = 1.0f;
        return true;
    }
    if (!installSpeed()) return false;

    g_multiplier = multiplier;
    return true;
}

float Cheats::speedMultiplier()
{
    return g_multiplier;
}

// Crabe._setGodMode(enabled) -> 1, or nil when the caves could not be
// installed. Lua turns that nil into a named error.
int __cdecl CheatNatives::setGodMode(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    bool enabled = lua.argToNumber(L, 1, 0.0) != 0.0;
    if (!Cheats::setGodMode(enabled)) return 0;

    lua.pushNumber(L, 1.0);
    return 1;
}

// Crabe._getGodMode() -> 1 or 0.
int __cdecl CheatNatives::getGodMode(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    lua.pushNumber(L, Cheats::godMode() ? 1.0 : 0.0);
    return 1;
}

// Crabe._setSpeedMultiplier(multiplier) -> the multiplier actually applied
// (it is clamped), or nil when the caves could not be installed.
int __cdecl CheatNatives::setSpeedMultiplier(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    auto multiplier = static_cast<float>(lua.argToNumber(L, 1, 1.0));
    if (!Cheats::setSpeedMultiplier(multiplier)) return 0;

    lua.pushNumber(L, static_cast<double>(Cheats::speedMultiplier()));
    return 1;
}

// Crabe._getSpeedMultiplier() -> number.
int __cdecl CheatNatives::getSpeedMultiplier(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    lua.pushNumber(L, static_cast<double>(Cheats::speedMultiplier()));
    return 1;
}

// Crabe._playerObject() -> address, or nil until the capture cave has run.
int __cdecl CheatNatives::playerObject(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    uintptr_t object = Cheats::playerObject();
    if (!object) return 0;

    lua.pushNumber(L, static_cast<double>(object));
    return 1;
}

// Crabe._playerFloat(offset) -> float at [player+offset], or nil. The whole
// entity struct is walkable this way, which is how offsets past the known
// health/velocity ones get identified without a rebuild.
int __cdecl CheatNatives::playerFloat(void* L)
{
    LuaCall& lua = LuaCall::get();
    if (!lua.hasReturnSupport()) return 0;

    auto offset = static_cast<uintptr_t>(lua.argToNumber(L, 1, static_cast<double>(kHealthOffset)));
    float value = 0.0f;

    if (!Cheats::readPlayerFloat(offset, value)) return 0;

    lua.pushNumber(L, static_cast<double>(value));
    return 1;
}
