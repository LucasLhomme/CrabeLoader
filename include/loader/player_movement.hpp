/*
** CrabeLoader
** File description:
** player_movement
*/

#ifndef PLAYER_MOVEMENT_HPP_
#define PLAYER_MOVEMENT_HPP_

#include <cstdint>
#include <string>

namespace PlayerMovement {
    bool installSpeed();
    bool setSpeedMultiplier(float multiplier);
    float speedMultiplier();
    std::string speedHitReport();
    
    bool trackPosition();
    uintptr_t moveObject();
    bool snapshotMove();
    std::string diffMove(float minimumChange);
    
    bool position(float& x, float& y, float& z);
    bool setPosition(float x, float y, float z);
    bool teleportDelta(float dx, float dy, float dz);
}

#endif /* !PLAYER_MOVEMENT_HPP_ */
