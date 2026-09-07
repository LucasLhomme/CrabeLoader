/*
** CrabeLoader
** File description:
** freecam.hpp — 6-DOF Free Camera & Photo Mode Engine (C++23)
*/

#ifndef FREECAM_HPP_
#define FREECAM_HPP_

#include <cmath>
#include <cstdint>
#include <windows.h>

class Freecam final {
public:
    static Freecam& get();

    Freecam(const Freecam&) = delete;
    Freecam& operator=(const Freecam&) = delete;

    void update(float dt = 0.016f);

    [[nodiscard]] bool isEnabled() const noexcept { return _enabled; }
    void setEnabled(bool enabled);
    void toggle() { setEnabled(!_enabled); }

    void setSpeed(float speed) noexcept;
    [[nodiscard]] float getSpeed() const noexcept { return _speed; }

    void setSensitivity(float sens) noexcept;
    [[nodiscard]] float getSensitivity() const noexcept { return _sensitivity; }

    void getPosition(float& x, float& y, float& z) const noexcept {
        x = _x; y = _y; z = _z;
    }

    void setPosition(float x, float y, float z) noexcept;

    void getRotation(float& pitch, float& yaw, float& roll) const noexcept {
        pitch = _pitch; yaw = _yaw; roll = _roll;
    }

    void setRotation(float pitch, float yaw, float roll) noexcept;

    [[nodiscard]] bool teleportPlayerToCamera();

    void setWorldFrozen(bool frozen);
    [[nodiscard]] bool isWorldFrozen() const noexcept { return _worldFrozen; }
    void toggleWorldFrozen() { setWorldFrozen(!_worldFrozen); }

    [[nodiscard]] uintptr_t getCameraObject() const noexcept;

private:
    Freecam() = default;
    ~Freecam() = default;

    void handleKeyboardInput(float dt);
    void handleMouseInput();

    bool _enabled{false};
    bool _worldFrozen{false};
    float _x{0.0f};
    float _y{0.0f};
    float _z{0.0f};

    float _pitch{0.0f}; // Radians (-89 deg to +89 deg)
    float _yaw{0.0f};   // Radians (0 to 2*PI)
    float _roll{0.0f};  // Radians

    float _speed{25.0f};         // Units per second
    float _sensitivity{0.003f};  // Radians per pixel

    POINT _lastMousePos{};
    bool _mouseInitialized{false};
};

namespace FreecamNatives {
    int __cdecl toggle(void* L);
    int __cdecl setEnabled(void* L);
    int __cdecl isEnabled(void* L);
    int __cdecl setSpeed(void* L);
    int __cdecl getSpeed(void* L);
    int __cdecl update(void* L);
    int __cdecl getPosition(void* L);
    int __cdecl setPosition(void* L);
    int __cdecl getRotation(void* L);
    int __cdecl setRotation(void* L);
    int __cdecl teleportPlayer(void* L);
    int __cdecl setWorldFrozen(void* L);
    int __cdecl isWorldFrozen(void* L);
    int __cdecl setSensitivity(void* L);
    int __cdecl getSensitivity(void* L);
    bool registerAll(void* L);
}

#endif /* !FREECAM_HPP_ */

