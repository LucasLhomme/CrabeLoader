/*
** CrabeLoader
** File description:
** freecam.cpp — Industry-Standard 6-DOF Free Camera & Photo Mode Engine (C++23)
*/

#include "loader/freecam.hpp"
#include "loader/cheats.hpp"
#include "loader/speedhack.hpp"
#include "loader/codecave.hpp"
#include "loader/luacall.hpp"
#include "loader/memory.hpp"
#include "logger/logger.hpp"

#include <algorithm>
#include <memory>
#include <numbers>

namespace {
    constexpr float kPi = std::numbers::pi_v<float>;
    constexpr float kTwoPi = 2.0f * kPi;
    constexpr float kHalfPi = kPi * 0.5f;
    constexpr float kMaxPitch = kHalfPi - 0.05f; // ~86 degrees

    // Camera position store site inside the camera controller function (0x8E190 -> 0x8E311):
    // 50                      push eax
    // F3 0F 11 5C 24 24       movss [esp+24h], xmm3 ; Camera X
    // F3 0F 11 64 24 28       movss [esp+28h], xmm4 ; Camera Y
    // F3 0F 11 6C 24 2C       movss [esp+2Ch], xmm5 ; Camera Z
    // E8 18 E8 FF FF          call 0x8CB40
    constexpr const char* kCameraStorePattern = "50 F3 0F 11 5C 24 24 F3 0F 11 64 24 28 F3 0F 11 6C 24 2C E8";
    constexpr uintptr_t kCameraStoreFallbackOffset = 0x8E311;
    constexpr size_t kStolenBytes = 18;

    bool isKeyDown(int vk) noexcept
    {
        return (GetAsyncKeyState(vk) & 0x8000) != 0;
    }

    uint8_t g_freecamEnabled = 0;
    float g_freecamX = 0.0f;
    float g_freecamY = 0.0f;
    float g_freecamZ = 0.0f;
    uintptr_t g_cameraObject = 0;

    std::unique_ptr<CodeCave> g_cameraCave;

    void installCheatEngineFreecamCave()
    {
        static bool installed = false;
        if (installed) return;

        uintptr_t fn = Memory::patternScan(kCameraStorePattern);
        uintptr_t site = fn ? (fn + 1) : 0; // Skip '50' (push eax) to hook the 18-byte movss instructions
        if (!site) {
            uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
            if (base && Memory::isReadable(base + kCameraStoreFallbackOffset, kStolenBytes)) {
                site = base + kCameraStoreFallbackOffset;
                Logger::getInstance().info("Freecam: pattern scan missed, using verified fallback offset 0x{:X}", site);
            }
        } else {
            Logger::getInstance().info("Freecam: unique camera controller pattern resolved site @ 0x{:X}", site);
        }

        if (site && Memory::isReadable(site, kStolenBytes)) {
            std::vector<uint8_t> body;

            // 1. mov [g_cameraObject], ebx (6 bytes)
            body.push_back(0x89); body.push_back(0x1D);
            CodeCave::putU32(body, body.size(), reinterpret_cast<uint32_t>(&g_cameraObject));

            // 2. cmp byte ptr [g_freecamEnabled], 1 (7 bytes)
            body.push_back(0x80); body.push_back(0x3D);
            CodeCave::putU32(body, body.size(), reinterpret_cast<uint32_t>(&g_freecamEnabled));
            body.push_back(0x01);

            // 3. je freecam_override (placeholder, 2 bytes)
            const size_t jeIndex = body.size();
            body.push_back(0x74); body.push_back(0x00);

            // --- NORMAL GAME EXECUTION & COORD SYNC ---
            // movss [esp+24h], xmm3
            body.push_back(0xF3); body.push_back(0x0F); body.push_back(0x11);
            body.push_back(0x5C); body.push_back(0x24); body.push_back(0x24);
            // movss [esp+28h], xmm4
            body.push_back(0xF3); body.push_back(0x0F); body.push_back(0x11);
            body.push_back(0x64); body.push_back(0x24); body.push_back(0x28);
            // movss [esp+2Ch], xmm5
            body.push_back(0xF3); body.push_back(0x0F); body.push_back(0x11);
            body.push_back(0x6C); body.push_back(0x24); body.push_back(0x2C);

            // Sync coordinates:
            // movss [g_freecamX], xmm3
            body.push_back(0xF3); body.push_back(0x0F); body.push_back(0x11); body.push_back(0x1D);
            CodeCave::putU32(body, body.size(), reinterpret_cast<uint32_t>(&g_freecamX));
            // movss [g_freecamY], xmm4
            body.push_back(0xF3); body.push_back(0x0F); body.push_back(0x11); body.push_back(0x25);
            CodeCave::putU32(body, body.size(), reinterpret_cast<uint32_t>(&g_freecamY));
            // movss [g_freecamZ], xmm5
            body.push_back(0xF3); body.push_back(0x0F); body.push_back(0x11); body.push_back(0x2D);
            CodeCave::putU32(body, body.size(), reinterpret_cast<uint32_t>(&g_freecamZ));

            // Jump past override block and past stolen bytes (placeholder, 2 bytes)
            const size_t normalJmpIndex = body.size();
            body.push_back(0xEB); body.push_back(0x00);

            // --- FREECAM OVERRIDE BLOCK ---
            const size_t overrideStartIndex = body.size();
            body[jeIndex + 1] = static_cast<uint8_t>(overrideStartIndex - (jeIndex + 2));

            // fld dword ptr [g_freecamX] ; fstp dword ptr [esp+24h]
            body.push_back(0xD9); body.push_back(0x05);
            CodeCave::putU32(body, body.size(), reinterpret_cast<uint32_t>(&g_freecamX));
            body.push_back(0xD9); body.push_back(0x5C); body.push_back(0x24); body.push_back(0x24);

            // fld dword ptr [g_freecamY] ; fstp dword ptr [esp+28h]
            body.push_back(0xD9); body.push_back(0x05);
            CodeCave::putU32(body, body.size(), reinterpret_cast<uint32_t>(&g_freecamY));
            body.push_back(0xD9); body.push_back(0x5C); body.push_back(0x24); body.push_back(0x28);

            // fld dword ptr [g_freecamZ] ; fstp dword ptr [esp+2Ch]
            body.push_back(0xD9); body.push_back(0x05);
            CodeCave::putU32(body, body.size(), reinterpret_cast<uint32_t>(&g_freecamZ));
            body.push_back(0xD9); body.push_back(0x5C); body.push_back(0x24); body.push_back(0x2C);

            // Jump past stolen bytes (placeholder, 2 bytes)
            const size_t overrideJmpIndex = body.size();
            body.push_back(0xEB); body.push_back(0x00);

            // Dynamically resolve jump offsets:
            const size_t endOfCaveTarget = body.size() + kStolenBytes;
            body[normalJmpIndex + 1] = static_cast<uint8_t>(endOfCaveTarget - (normalJmpIndex + 2));
            body[overrideJmpIndex + 1] = static_cast<uint8_t>(endOfCaveTarget - (overrideJmpIndex + 2));

            g_cameraCave = std::make_unique<CodeCave>();
            if (g_cameraCave->install(site, body, kStolenBytes)) {
                Logger::getInstance().info(
                    "Freecam: Camera Controller CodeCave successfully installed at 0x{:X}", site);
            } else {
                Logger::getInstance().error("Freecam: failed to install camera code cave at 0x{:X}", site);
            }
        } else {
            Logger::getInstance().warning("Freecam: camera store site could not be located in memory.");
        }

        installed = true;
    }
}

Freecam& Freecam::get()
{
    static Freecam instance;
    return instance;
}

uintptr_t Freecam::getCameraObject() const noexcept
{
    return g_cameraObject;
}

void Freecam::setSpeed(float speed) noexcept
{
    _speed = std::clamp(speed, 1.0f, 1000.0f);
}

void Freecam::setSensitivity(float sens) noexcept
{
    _sensitivity = std::clamp(sens, 0.0001f, 0.1f);
}

void Freecam::setRotation(float pitch, float yaw, float roll) noexcept
{
    _pitch = std::clamp(pitch, -kMaxPitch, kMaxPitch);

    while (yaw >= kTwoPi) yaw -= kTwoPi;
    while (yaw < 0.0f) yaw += kTwoPi;
    _yaw = yaw;

    _roll = roll;
}

void Freecam::setEnabled(bool enabled)
{
    if (_enabled == enabled) return;

    installCheatEngineFreecamCave();

    _enabled = enabled;
    g_freecamEnabled = enabled ? 1 : 0;

    if (_enabled) {
        _x = g_freecamX;
        _y = g_freecamY;
        _z = g_freecamZ;

        _mouseInitialized = false;

        Logger::getInstance().info("Freecam: Detached seamlessly at position ({:.2f}, {:.2f}, {:.2f})", _x, _y, _z);
    } else {
        if (_worldFrozen) {
            setWorldFrozen(false);
        }
        Logger::getInstance().info("Freecam: Re-attached to player.");
    }
}

void Freecam::setWorldFrozen(bool frozen)
{
    _worldFrozen = frozen;
    SpeedHack::setSpeed(frozen ? 0.0001f : 1.0f);
    Logger::getInstance().info("Freecam: World freeze state set to {}", frozen ? "FROZEN (Photo Mode)" : "NORMAL");
}

bool Freecam::teleportPlayerToCamera()
{
    Cheats::trackPosition();
    const bool ok = Cheats::setPosition(_x, _y, _z);
    if (ok) {
        Logger::getInstance().info(
            "Freecam: Teleported avatar to camera position ({:.2f}, {:.2f}, {:.2f})", _x, _y, _z);
    } else {
        Logger::getInstance().warning("Freecam: Failed to teleport avatar (movement object not yet captured)");
    }
    return ok;
}

void Freecam::handleMouseInput()
{
    POINT currentPos;
    if (!GetCursorPos(&currentPos)) return;

    if (!_mouseInitialized) {
        _lastMousePos = currentPos;
        _mouseInitialized = true;
        return;
    }

    if (isKeyDown(VK_RBUTTON)) {
        const float dx = static_cast<float>(currentPos.x - _lastMousePos.x);
        const float dy = static_cast<float>(currentPos.y - _lastMousePos.y);

        _yaw += dx * _sensitivity;
        _pitch -= dy * _sensitivity;

        while (_yaw >= kTwoPi) _yaw -= kTwoPi;
        while (_yaw < 0.0f) _yaw += kTwoPi;

        _pitch = std::clamp(_pitch, -kMaxPitch, kMaxPitch);
    }

    _lastMousePos = currentPos;
}

void Freecam::handleKeyboardInput(float dt)
{
    const float cosP = std::cos(_pitch);
    const float sinP = std::sin(_pitch);
    const float cosY = std::cos(_yaw);
    const float sinY = std::sin(_yaw);

    // Forward vector (along horizontal & vertical view direction)
    const float fwdX = cosP * sinY;
    const float fwdY = sinP;
    const float fwdZ = cosP * cosY;

    // Right vector (perpendicular to view on XZ plane)
    const float rightX = cosY;
    const float rightY = 0.0f;
    const float rightZ = -sinY;

    // Up vector (world vertical axis)
    constexpr float upX = 0.0f;
    constexpr float upY = 1.0f;
    constexpr float upZ = 0.0f;

    float moveX = 0.0f, moveY = 0.0f, moveZ = 0.0f;

    // Forward: Z / W / Up
    if (isKeyDown('Z') || isKeyDown('W') || isKeyDown(VK_UP)) {
        moveX += fwdX; moveY += fwdY; moveZ += fwdZ;
    }
    // Backward: S / Down
    if (isKeyDown('S') || isKeyDown(VK_DOWN)) {
        moveX -= fwdX; moveY -= fwdY; moveZ -= fwdZ;
    }

    // Right: D / Right
    if (isKeyDown('D') || isKeyDown(VK_RIGHT)) {
        moveX += rightX; moveY += rightY; moveZ += rightZ;
    }
    // Left: Q / A / Left
    if (isKeyDown('Q') || isKeyDown('A') || isKeyDown(VK_LEFT)) {
        moveX -= rightX; moveY -= rightY; moveZ -= rightZ;
    }

    // Up: Space / E
    if (isKeyDown(VK_SPACE) || isKeyDown('E')) {
        moveX += upX; moveY += upY; moveZ += upZ;
    }
    // Down: Ctrl / C
    if (isKeyDown(VK_CONTROL) || isKeyDown('C')) {
        moveX -= upX; moveY -= upY; moveZ -= upZ;
    }

    // Roll: Numpad 7 / [ or Numpad 9 / ]
    if (isKeyDown(VK_NUMPAD7) || isKeyDown(VK_OEM_4)) {
        _roll -= 1.5f * dt;
    }
    if (isKeyDown(VK_NUMPAD9) || isKeyDown(VK_OEM_6)) {
        _roll += 1.5f * dt;
    }
    // Reset Roll: Numpad 8
    if (isKeyDown(VK_NUMPAD8)) {
        _roll = 0.0f;
    }

    // Dynamic speed adjustment (+ / -)
    if (isKeyDown(VK_ADD) || isKeyDown(VK_OEM_PLUS)) {
        setSpeed(_speed + 15.0f * dt);
    }
    if (isKeyDown(VK_SUBTRACT) || isKeyDown(VK_OEM_MINUS)) {
        setSpeed(_speed - 15.0f * dt);
    }

    float currentSpeed = _speed;
    if (isKeyDown(VK_SHIFT)) currentSpeed *= 3.0f;
    if (isKeyDown(VK_MENU))  currentSpeed *= 0.25f;

    const float lenSq = moveX * moveX + moveY * moveY + moveZ * moveZ;
    if (lenSq > 0.00001f) {
        const float invLen = 1.0f / std::sqrt(lenSq);
        moveX *= invLen;
        moveY *= invLen;
        moveZ *= invLen;

        _x += moveX * currentSpeed * dt;
        _y += moveY * currentSpeed * dt;
        _z += moveZ * currentSpeed * dt;

        g_freecamX = _x;
        g_freecamY = _y;
        g_freecamZ = _z;
    }
}

void Freecam::update(float dt)
{
    if (!_enabled) return;

    handleMouseInput();
    handleKeyboardInput(dt);
}

// ---------------------------------------------------------------------------
// Lua Native Bindings
// ---------------------------------------------------------------------------

int __cdecl FreecamNatives::toggle(void* L)
{
    LuaCall& lua = LuaCall::get();
    Freecam::get().toggle();
    lua.pushNumber(L, Freecam::get().isEnabled() ? 1.0 : 0.0);
    return 1;
}

int __cdecl FreecamNatives::setEnabled(void* L)
{
    LuaCall& lua = LuaCall::get();
    const bool enabled = lua.argToBoolean(L, 1, false) || (lua.argToNumber(L, 1, 0.0) != 0.0);
    Freecam::get().setEnabled(enabled);
    lua.pushNumber(L, Freecam::get().isEnabled() ? 1.0 : 0.0);
    return 1;
}

int __cdecl FreecamNatives::isEnabled(void* L)
{
    LuaCall& lua = LuaCall::get();
    lua.pushNumber(L, Freecam::get().isEnabled() ? 1.0 : 0.0);
    return 1;
}

int __cdecl FreecamNatives::setSpeed(void* L)
{
    LuaCall& lua = LuaCall::get();
    const auto spd = static_cast<float>(lua.argToNumber(L, 1, 25.0));
    Freecam::get().setSpeed(spd);
    lua.pushNumber(L, Freecam::get().getSpeed());
    return 1;
}

int __cdecl FreecamNatives::getSpeed(void* L)
{
    LuaCall& lua = LuaCall::get();
    lua.pushNumber(L, Freecam::get().getSpeed());
    return 1;
}

int __cdecl FreecamNatives::setSensitivity(void* L)
{
    LuaCall& lua = LuaCall::get();
    const auto sens = static_cast<float>(lua.argToNumber(L, 1, 0.003));
    Freecam::get().setSensitivity(sens);
    lua.pushNumber(L, Freecam::get().getSensitivity());
    return 1;
}

int __cdecl FreecamNatives::getSensitivity(void* L)
{
    LuaCall& lua = LuaCall::get();
    lua.pushNumber(L, Freecam::get().getSensitivity());
    return 1;
}

int __cdecl FreecamNatives::update(void* L)
{
    LuaCall& lua = LuaCall::get();
    const auto dt = static_cast<float>(lua.argToNumber(L, 1, 0.033));
    Freecam::get().update(dt);
    lua.pushNumber(L, 1.0);
    return 1;
}

int __cdecl FreecamNatives::getPosition(void* L)
{
    LuaCall& lua = LuaCall::get();
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Freecam::get().getPosition(x, y, z);
    lua.pushNumber(L, x);
    lua.pushNumber(L, y);
    lua.pushNumber(L, z);
    return 3;
}

int __cdecl FreecamNatives::setPosition(void* L)
{
    LuaCall& lua = LuaCall::get();
    const auto x = static_cast<float>(lua.argToNumber(L, 1, 0.0));
    const auto y = static_cast<float>(lua.argToNumber(L, 2, 0.0));
    const auto z = static_cast<float>(lua.argToNumber(L, 3, 0.0));
    Freecam::get().setPosition(x, y, z);
    lua.pushNumber(L, 1.0);
    return 1;
}

int __cdecl FreecamNatives::getRotation(void* L)
{
    LuaCall& lua = LuaCall::get();
    float pitch = 0.0f, yaw = 0.0f, roll = 0.0f;
    Freecam::get().getRotation(pitch, yaw, roll);
    lua.pushNumber(L, pitch);
    lua.pushNumber(L, yaw);
    lua.pushNumber(L, roll);
    return 3;
}

int __cdecl FreecamNatives::setRotation(void* L)
{
    LuaCall& lua = LuaCall::get();
    const auto pitch = static_cast<float>(lua.argToNumber(L, 1, 0.0));
    const auto yaw = static_cast<float>(lua.argToNumber(L, 2, 0.0));
    const auto roll = static_cast<float>(lua.argToNumber(L, 3, 0.0));
    Freecam::get().setRotation(pitch, yaw, roll);
    lua.pushNumber(L, 1.0);
    return 1;
}

int __cdecl FreecamNatives::teleportPlayer(void* L)
{
    LuaCall& lua = LuaCall::get();
    const bool ok = Freecam::get().teleportPlayerToCamera();
    lua.pushNumber(L, ok ? 1.0 : 0.0);
    return 1;
}

int __cdecl FreecamNatives::setWorldFrozen(void* L)
{
    LuaCall& lua = LuaCall::get();
    const bool frozen = lua.argToBoolean(L, 1, false) || (lua.argToNumber(L, 1, 0.0) != 0.0);
    Freecam::get().setWorldFrozen(frozen);
    lua.pushNumber(L, Freecam::get().isWorldFrozen() ? 1.0 : 0.0);
    return 1;
}

int __cdecl FreecamNatives::isWorldFrozen(void* L)
{
    LuaCall& lua = LuaCall::get();
    lua.pushNumber(L, Freecam::get().isWorldFrozen() ? 1.0 : 0.0);
    return 1;
}
