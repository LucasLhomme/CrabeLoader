/*
** CrabeLoader
** File description:
** player_movement -- character locomotion and position manipulation
*/

#include "domain/player_movement.hpp"

#include <cmath>
#include <cstring>
#include <mutex>
#include <vector>
#include <array>

#include "infrastructure/codecave.hpp"
#include "infrastructure/memory.hpp"
#include "shared/logger.hpp"

namespace {

    // The four analog-stick-to-velocity stores.
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

    constexpr float kMinMultiplier = 0.1f;
    constexpr float kMaxMultiplier = 50.0f;

    float g_multiplier = 1.0f;
    uint32_t g_speedHits[4] = {};
    uintptr_t g_moveObject = 0;

    constexpr uintptr_t kPositionOffset = 0x28;

    constexpr uintptr_t kProbeSpan = 0x400;
    constexpr size_t kProbeCount = kProbeSpan / sizeof(float);
    std::array<float, kProbeCount> g_probe = {};
    bool g_probeTaken = false;

    CodeCave g_speedCaves[kSpeedSiteCount];
    bool g_speedReady = false;
    std::mutex g_installMutex;

    std::vector<uint8_t> buildSpeedBody(size_t site)
    {
        constexpr size_t kHitsAt = 2;
        constexpr size_t kObjectAt = 7;
        constexpr size_t kMultiplierAt = 15;

        std::vector<uint8_t> body = {
            0xFF, 0x05, 0x00, 0x00, 0x00, 0x00,             // inc dword ptr [g_speedHits[site]]
            0xA3, 0x00, 0x00, 0x00, 0x00,                   // mov [g_moveObject], eax
            0xF3, 0x0F, 0x59, 0x05, 0x00, 0x00, 0x00, 0x00, // mulss xmm0, [g_multiplier]
        };

        CodeCave::putU32(body, kHitsAt, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_speedHits[site])));
        CodeCave::putU32(body, kObjectAt, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_moveObject)));
        CodeCave::putU32(body, kMultiplierAt, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_multiplier)));
        return body;
    }

} // namespace

bool PlayerMovement::installSpeed()
{
    std::lock_guard<std::mutex> lock(g_installMutex);
    if (g_speedReady) return true;

    uintptr_t sites[kSpeedSiteCount] = {};
    for (size_t i = 0; i < kSpeedSiteCount; ++i) {
        uintptr_t base = Memory::patternScan(kSpeedSites[i].pattern);
        sites[i] = base ? base + kSpeedSites[i].storeOffset : 0;

        if (!base) {
            Logger::getInstance().error("PlayerMovement: speed AOB not found for {}.", kSpeedSites[i].name);
            return false;
        }
    }

    for (size_t i = 0; i < kSpeedSiteCount; ++i) {
        if (g_speedCaves[i].installLogged(sites[i], buildSpeedBody(i), kSpeedStoreLength,
                                          "PlayerMovement", kSpeedSites[i].name)) {
            continue;
        }
        for (size_t j = 0; j < i; ++j) g_speedCaves[j].remove();
        return false;
    }

    g_speedReady = true;
    Logger::getInstance().info("PlayerMovement: speed caves installed ({} sites).", kSpeedSiteCount);
    return true;
}

bool PlayerMovement::setSpeedMultiplier(float multiplier)
{
    if (multiplier < kMinMultiplier) multiplier = kMinMultiplier;
    if (multiplier > kMaxMultiplier) multiplier = kMaxMultiplier;

    if (multiplier == 1.0f && !g_speedReady) {
        g_multiplier = 1.0f;
        return true;
    }
    if (!installSpeed()) return false;

    g_multiplier = multiplier;
    return true;
}

float PlayerMovement::speedMultiplier()
{
    return g_multiplier;
}

bool PlayerMovement::trackPosition()
{
    if (g_speedReady) return true;
    if (!installSpeed()) return false;

    g_multiplier = 1.0f;
    return true;
}

uintptr_t PlayerMovement::moveObject()
{
    return g_moveObject;
}

bool PlayerMovement::position(float& x, float& y, float& z)
{
    if (!g_moveObject) return false;
    if (!Memory::isReadable(g_moveObject + kPositionOffset, 3 * sizeof(float))) return false;

    const auto* at = reinterpret_cast<const float*>(g_moveObject + kPositionOffset);
    x = at[0]; y = at[1]; z = at[2];
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}

bool PlayerMovement::snapshotMove()
{
    if (!g_moveObject) return false;
    if (!Memory::isReadable(g_moveObject, kProbeSpan)) return false;

    std::memcpy(g_probe.data(), reinterpret_cast<const void*>(g_moveObject), kProbeSpan);
    g_probeTaken = true;
    return true;
}

std::string PlayerMovement::diffMove(float minimumChange)
{
    if (!g_probeTaken) return "no snapshot taken yet";
    if (!g_moveObject || !Memory::isReadable(g_moveObject, kProbeSpan)) return "movement object is gone";

    std::array<float, kProbeCount> current = {};
    std::memcpy(current.data(), reinterpret_cast<const void*>(g_moveObject), kProbeSpan);

    std::string out;
    size_t reported = 0;

    for (size_t i = 0; i < kProbeCount && reported < 20; ++i) {
        if (!std::isfinite(current[i]) || !std::isfinite(g_probe[i])) continue;

        float delta = current[i] - g_probe[i];
        if (delta < 0.0f) delta = -delta;
        if (delta < minimumChange) continue;

        char row[80] = {};
        _snprintf_s(row, _TRUNCATE, "%s0x%X:%+.1f(now %.1f)", reported ? " " : "",
                    static_cast<unsigned>(i * sizeof(float)),
                    current[i] - g_probe[i], current[i]);
        out += row;
        ++reported;
    }
    return reported ? out : "nothing moved by more than the floor";
}

std::string PlayerMovement::speedHitReport()
{
    std::string out;
    for (size_t i = 0; i < kSpeedSiteCount; ++i) {
        if (i) out += "/";
        out += std::to_string(g_speedHits[i]);
    }
    return out;
}

bool PlayerMovement::setPosition(float x, float y, float z)
{
    if (!g_moveObject) return false;
    if (!Memory::isReadable(g_moveObject + kPositionOffset, 3 * sizeof(float))) return false;

    auto* at = reinterpret_cast<float*>(g_moveObject + kPositionOffset);
    at[0] = x; at[1] = y; at[2] = z;
    return true;
}

bool PlayerMovement::teleportDelta(float dx, float dy, float dz)
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
    if (!position(x, y, z)) return false;
    return setPosition(x + dx, y + dy, z + dz);
}
