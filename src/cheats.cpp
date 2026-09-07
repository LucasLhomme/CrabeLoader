/*
** CrabeLoader
** File description:
** cheats -- god mode and movement speed
*/

#include "loader/cheats.hpp"

#include <cmath>
#include <cstring>
#include <string>
#include <mutex>
#include <vector>
#include <windows.h>

#include "loader/codecave.hpp"
#include "loader/entity_registry.hpp"
#include "loader/luacall.hpp"
#include "loader/memory.hpp"
#include "logger/logger.hpp"

namespace {

    // Health component, identical at both sites: a float health at offset
    // 0x08, a float maximum health at 0x0C. Nothing in the struct says whether
    // it belongs to the player, which is why EntityRegistry decides that by
    // watching rather than by reading.
    constexpr uintptr_t kHealthOffset = 0x08;

    // movss xmm0, [eax+0x08] then divss by [eax+0x0C] -- a HUD meter turning a
    // component into a bar fill. eax is the component. Unique in .text.
    const char* kCapturePattern = "F3 0F 10 40 08 F3 0F 5E 40 0C 0F 2F C8 77";

    // Inside the engine's apply-health-delta routine. By this point esi is the
    // component, xmm0 is health plus the delta, and the delta itself is still
    // readable at [ebp+8]. Unique in .text.
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

    // Read by the caves, written from here, so both must be plain addresses
    // the assembled instructions can reach.
    volatile LONG g_enabled = 0;
    float g_multiplier = 1.0f;

    // Incremented by the clamp cave itself, once per cancelled hit. This is
    // the only direct evidence that the cheat did something.
    uint32_t g_blocked = 0;

    // One per velocity site, incremented by that site's own cave.
    uint32_t g_speedHits[4] = {};

    // The player's movement structure, stored by the velocity caves. Position
    // is somewhere inside it; finding it is what the snapshot below is for.
    uintptr_t g_moveObject = 0;

    // A window of the structure, captured on demand so it can be compared
    // against itself after the player has moved. Three adjacent floats that
    // all change together are the position.
    // Position candidate, from the walk test: 0x28 and 0x30 both moved while
    // 0x2C stayed put, which is X and Z changing with the height between them.
    constexpr uintptr_t kPositionOffset = 0x28;

    constexpr uintptr_t kProbeSpan = 0x400;
    constexpr size_t kProbeCount = kProbeSpan / sizeof(float);
    float g_probe[kProbeCount] = {};
    bool g_probeTaken = false;

    // The caves reach the recorders through these, rather than through a
    // relative call: an indirect call needs no fixing up once the cave has
    // been allocated at whatever address the system handed back.
    void (__cdecl* g_recordSeen)(uintptr_t) = &CrabeRecordSeen;
    void (__cdecl* g_recordDamage)(uintptr_t, float) = &CrabeRecordDamage;

    CodeCave g_captureCave;
    CodeCave g_clampCave;
    CodeCave g_speedCaves[kSpeedSiteCount];

    bool g_godReady = false;
    bool g_speedReady = false;
    std::mutex g_installMutex;

    // Preamble shared by both recorder shims: save the flags, the general
    // registers, and the four xmm registers a cdecl call is free to destroy.
    // pushad does not cover xmm, and both sites hold live floats across our
    // insertion, so skipping this corrupts health values rather than crashing
    // -- the kind of fault that looks like a broken cheat.
    void appendRecorderPrologue(std::vector<uint8_t>& body)
    {
        const uint8_t prologue[] = {
            0x9C,                         // pushfd
            0x60,                         // pushad
            0x83, 0xEC, 0x40,             // sub esp, 40h
            0x0F, 0x11, 0x44, 0x24, 0x00, // movups [esp], xmm0
            0x0F, 0x11, 0x4C, 0x24, 0x10, // movups [esp+10h], xmm1
            0x0F, 0x11, 0x54, 0x24, 0x20, // movups [esp+20h], xmm2
            0x0F, 0x11, 0x5C, 0x24, 0x30, // movups [esp+30h], xmm3
        };
        body.insert(body.end(), prologue, prologue + sizeof(prologue));
    }

    void appendRecorderEpilogue(std::vector<uint8_t>& body, uint8_t argBytes)
    {
        const uint8_t epilogue[] = {
            0x83, 0xC4, argBytes,         // add esp, <arguments>
            0x0F, 0x10, 0x44, 0x24, 0x00, // movups xmm0, [esp]
            0x0F, 0x10, 0x4C, 0x24, 0x10, // movups xmm1, [esp+10h]
            0x0F, 0x10, 0x54, 0x24, 0x20, // movups xmm2, [esp+20h]
            0x0F, 0x10, 0x5C, 0x24, 0x30, // movups xmm3, [esp+30h]
            0x83, 0xC4, 0x40,             // add esp, 40h
            0x61,                         // popad
            0x9D,                         // popfd
        };
        body.insert(body.end(), epilogue, epilogue + sizeof(epilogue));
    }

    // Calls through a function pointer held in a global. A direct call would
    // need a displacement relative to the cave, which is not known until the
    // allocation has already happened.
    void appendIndirectCall(std::vector<uint8_t>& body, void* slot)
    {
        body.push_back(0xFF);
        body.push_back(0x15);
        const auto address = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(slot));
        for (int shift = 0; shift < 32; shift += 8) {
            body.push_back(static_cast<uint8_t>((address >> shift) & 0xFF));
        }
    }

    // Capture cave: hand the component in eax to the registry. It decides
    // nothing here -- no comparison, no threshold, no store to a player
    // pointer. Everything the old body tried to do in assembly now happens in
    // C++, where it can be corrected without rebuilding an instruction stream.
    std::vector<uint8_t> buildCaptureBody()
    {
        std::vector<uint8_t> body;

        appendRecorderPrologue(body);
        body.push_back(0x50); // push eax -- the health component
        appendIndirectCall(body, &g_recordSeen);
        appendRecorderEpilogue(body, 4);
        return body;
    }

    // Damage cave: report the delta the engine is about to apply, then clamp.
    //
    // Reporting first is what makes the cheat diagnosable. The registry ends up
    // knowing which component just lost health, so "who is the player" stops
    // being a guess: take a hit and read it back.
    //
    // The clamp itself is unchanged and still narrow. When enabled and esi is
    // the component the registry points at, the incoming health in xmm0 is
    // replaced by the current one, making the store that follows a no-op. A
    // higher xmm0 is left alone, so healing and pickups still work, and every
    // other component in the game takes damage normally.
    std::vector<uint8_t> buildClampBody()
    {
        std::vector<uint8_t> body;

        appendRecorderPrologue(body);
        body.push_back(0xFF); // push dword ptr [ebp+08h] -- the delta
        body.push_back(0x75);
        body.push_back(0x08);
        body.push_back(0x56); // push esi -- the health component
        appendIndirectCall(body, &g_recordDamage);
        appendRecorderEpilogue(body, 8);

        const size_t clampAt = body.size();

        // The increment counts cancellations at the only place that can know
        // one happened. Without it, a cheat that does nothing cannot be told
        // apart from a clamp that is never reached. That distinction is what
        // cost the most time on this feature, because nothing recorded it.
        const uint8_t clamp[] = {
            0x50,                                     // push eax
            0x9C,                                     // pushfd
            0x83, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, // cmp dword ptr [g_enabled], 0
            0x74, 0x1E,                               // je done
            0xA1, 0x00, 0x00, 0x00, 0x00,             // mov eax, [target]
            0x85, 0xC0,                               // test eax, eax
            0x74, 0x15,                               // je done
            0x3B, 0xC6,                               // cmp eax, esi
            0x75, 0x11,                               // jne done
            0x0F, 0x2F, 0x46, 0x08,                   // comiss xmm0, [esi+08h]
            0x73, 0x0B,                               // jae done -- a heal, leave it
            0xFF, 0x05, 0x00, 0x00, 0x00, 0x00,       // inc dword ptr [g_blocked]
            0xF3, 0x0F, 0x10, 0x46, 0x08,             // movss xmm0, [esi+08h]
            0x9D,                                     // done: popfd
            0x58,                                     // pop eax
        };
        body.insert(body.end(), clamp, clamp + sizeof(clamp));

        CodeCave::putU32(body, clampAt + 4,
                         static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_enabled)));
        CodeCave::putU32(body, clampAt + 12,
                         static_cast<uint32_t>(reinterpret_cast<uintptr_t>(EntityRegistry::targetSlot())));
        CodeCave::putU32(body, clampAt + 32,
                         static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_blocked)));
        return body;
    }

    // Speed cave: count the pass, then scale the value before its store.
    //
    // The counter is per site and not optional. These four AOBs were inherited
    // from a project that never validated them -- its own source says so -- so
    // "the multiplier does nothing" has two very different causes: the site is
    // never executed, or it is executed and is not the movement path. Only a
    // per-site count separates them.
    std::vector<uint8_t> buildSpeedBody(size_t site)
    {
        constexpr size_t kHitsAt = 2;
        constexpr size_t kObjectAt = 7;
        constexpr size_t kMultiplierAt = 15;

        // The middle instruction is the valuable one. eax here is the player's
        // movement structure, and nothing else in the loader has ever held a
        // pointer to it -- which is exactly why teleport and noclip have been
        // out of reach. It writes a global and touches neither flags nor any
        // register, so it cannot disturb the store that follows.
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
    Logger::getInstance().info("Cheats: god mode {} (protecting 0x{:X}, {} hits cancelled so far).",
                               enabled ? "ON" : "OFF", EntityRegistry::target(), g_blocked);
    return true;
}

uint32_t Cheats::blockedHits()
{
    return g_blocked;
}

bool Cheats::godMode()
{
    return g_enabled != 0;
}

uintptr_t Cheats::playerObject()
{
    return EntityRegistry::target();
}

bool Cheats::readPlayerFloat(uintptr_t offset, float& out)
{
    uintptr_t object = EntityRegistry::target();

    if (!object) return false;
    if (!Memory::isReadable(object + offset, sizeof(float))) return false;

    std::memcpy(&out, reinterpret_cast<const void*>(object + offset), sizeof(float));
    return true;
}

// Writing an attribute is how a candidate offset gets confirmed. Reading alone
// cannot tell a run speed apart from any other number near it; changing it and
// watching the character is the only test that settles it.
bool Cheats::writePlayerFloat(uintptr_t offset, float value)
{
    uintptr_t object = EntityRegistry::target();

    if (!object) return false;
    if (!Memory::isReadable(object + offset, sizeof(float))) return false;

    std::memcpy(reinterpret_cast<void*>(object + offset), &value, sizeof(float));
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
        if (g_speedCaves[i].installLogged(sites[i], buildSpeedBody(i), kSpeedStoreLength,
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

// The multiply is left as a no-op: what this is really after is the store
// beside it that records the structure pointer.
bool Cheats::trackPosition()
{
    if (g_speedReady) return true;
    if (!installSpeed()) return false;

    g_multiplier = 1.0f;
    return true;
}

uintptr_t Cheats::moveObject()
{
    return g_moveObject;
}

// The three floats at 0x28, 0x2C and 0x30, which a walk on flat ground marked
// out as the position: the outer two moved together and the middle one, the
// height, did not. Reading them back is how that gets confirmed or dropped.
bool Cheats::position(float& x, float& y, float& z)
{
    if (!g_moveObject) return false;
    if (!Memory::isReadable(g_moveObject + kPositionOffset, 3 * sizeof(float))) return false;

    const auto* at = reinterpret_cast<const float*>(g_moveObject + kPositionOffset);
    x = at[0]; y = at[1]; z = at[2];
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}

// Records the first 0x400 bytes of the movement structure as floats.
bool Cheats::snapshotMove()
{
    if (!g_moveObject) return false;
    if (!Memory::isReadable(g_moveObject, kProbeSpan)) return false;

    std::memcpy(g_probe, reinterpret_cast<const void*>(g_moveObject), kProbeSpan);
    g_probeTaken = true;
    return true;
}

// Offsets whose float moved since the snapshot, biggest change first. Values
// that barely move are noise -- timers, blend weights -- so a floor keeps the
// report to things that actually track the player.
std::string Cheats::diffMove(float minimumChange)
{
    if (!g_probeTaken) return "no snapshot taken yet";
    if (!g_moveObject || !Memory::isReadable(g_moveObject, kProbeSpan)) return "movement object is gone";

    float current[kProbeCount] = {};
    std::memcpy(current, reinterpret_cast<const void*>(g_moveObject), kProbeSpan);

    std::string out;
    size_t reported = 0;

    for (size_t i = 0; i < kProbeCount && reported < 20; ++i) {
        // NaN compares false against everything, so an unfiltered comparison
        // lets every uninitialised slot through and buries the real hits. The
        // structure has a run of them from 0x168 onward.
        if (!std::isfinite(current[i]) || !std::isfinite(g_probe[i])) continue;

        float delta = current[i] - g_probe[i];
        if (delta < 0.0f) delta = -delta;
        if (delta < minimumChange) continue;

        // The value matters as much as the delta: a coordinate reads as tens
        // or hundreds, a normalised direction never leaves -1..1.
        char row[80] = {};
        _snprintf_s(row, _TRUNCATE, "%s0x%X:%+.1f(now %.1f)", reported ? " " : "",
                    static_cast<unsigned>(i * sizeof(float)),
                    current[i] - g_probe[i], current[i]);
        out += row;
        ++reported;
    }
    return reported ? out : "nothing moved by more than the floor";
}

// "12/0/340/0" -- one count per velocity site, in the order they are listed.
std::string Cheats::speedHitReport()
{
    std::string out;
    for (size_t i = 0; i < kSpeedSiteCount; ++i) {
        if (i) out += "/";
        out += std::to_string(g_speedHits[i]);
    }
    return out;
}

bool Cheats::setPosition(float x, float y, float z)
{
    if (!g_moveObject) return false;
    if (!Memory::isReadable(g_moveObject + kPositionOffset, 3 * sizeof(float))) return false;

    auto* at = reinterpret_cast<float*>(g_moveObject + kPositionOffset);
    at[0] = x; at[1] = y; at[2] = z;
    return true;
}

bool Cheats::teleportDelta(float dx, float dy, float dz)
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
    if (!position(x, y, z)) return false;
    return setPosition(x + dx, y + dy, z + dz);
}

