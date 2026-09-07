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
#include "loader/player_movement.hpp"
#include "loader/engine_patches.hpp"
#include "loader/entity_registry.hpp"
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

    constexpr size_t kDamageStoreLength = 5;

    // Read by the caves, written from here, so both must be plain addresses
    // the assembled instructions can reach.
    volatile LONG g_enabled = 0;

    // Incremented by the clamp cave itself, once per cancelled hit. This is
    // the only direct evidence that the cheat did something.
    uint32_t g_blocked = 0;

    // The caves reach the recorders through these, rather than through a
    // relative call: an indirect call needs no fixing up once the cave has
    // been allocated at whatever address the system handed back.
    void (__cdecl* g_recordSeen)(uintptr_t) = &CrabeRecordSeen;
    void (__cdecl* g_recordDamage)(uintptr_t, float) = &CrabeRecordDamage;

    CodeCave g_captureCave;
    CodeCave g_clampCave;

    bool g_godReady = false;
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
    return PlayerMovement::installSpeed();
}

bool Cheats::setSpeedMultiplier(float multiplier)
{
    return PlayerMovement::setSpeedMultiplier(multiplier);
}

float Cheats::speedMultiplier()
{
    return PlayerMovement::speedMultiplier();
}

bool Cheats::trackPosition()
{
    return PlayerMovement::trackPosition();
}

uintptr_t Cheats::moveObject()
{
    return PlayerMovement::moveObject();
}

bool Cheats::position(float& x, float& y, float& z)
{
    return PlayerMovement::position(x, y, z);
}

bool Cheats::snapshotMove()
{
    return PlayerMovement::snapshotMove();
}

std::string Cheats::diffMove(float minimumChange)
{
    return PlayerMovement::diffMove(minimumChange);
}

std::string Cheats::speedHitReport()
{
    return PlayerMovement::speedHitReport();
}

bool Cheats::setPosition(float x, float y, float z)
{
    return PlayerMovement::setPosition(x, y, z);
}

bool Cheats::teleportDelta(float dx, float dy, float dz)
{
    return PlayerMovement::teleportDelta(dx, dy, dz);
}

bool Cheats::unlockEditorEverywhere()
{
    return EnginePatches::unlockEditorEverywhere();
}

