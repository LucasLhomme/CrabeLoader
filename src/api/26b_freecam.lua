Crabe = Crabe or {}
Crabe.Freecam = Crabe.Freecam or {}

local Freecam = Crabe.Freecam

function Freecam.toggle()
    if type(Crabe._freecamToggle) == "function" then
        return Crabe._freecamToggle() == 1
    end
    return false
end

function Freecam.setEnabled(on)
    if type(Crabe._freecamSetEnabled) == "function" then
        local flag = (on == true or on == 1) and 1 or 0
        Crabe._freecamSetEnabled(flag)
        return on
    end
    return false
end

function Freecam.isEnabled()
    if type(Crabe._freecamIsEnabled) == "function" then
        return Crabe._freecamIsEnabled() == 1
    end
    return false
end

function Freecam.setSpeed(spd)
    if type(Crabe._freecamSetSpeed) == "function" then
        Crabe._freecamSetSpeed(tonumber(spd) or 25.0)
    end
    return tonumber(spd) or 25.0
end

function Freecam.getSpeed()
    if type(Crabe._freecamGetSpeed) == "function" then
        return Crabe._freecamGetSpeed()
    end
    return 25.0
end

function Freecam.setSensitivity(sens)
    if type(Crabe._freecamSetSensitivity) == "function" then
        Crabe._freecamSetSensitivity(tonumber(sens) or 0.2)
    end
end

function Freecam.getSensitivity()
    if type(Crabe._freecamGetSensitivity) == "function" then
        return Crabe._freecamGetSensitivity()
    end
    return 0.2
end

function Freecam.update(dt)
    if type(Crabe._freecamUpdate) == "function" then
        Crabe._freecamUpdate(tonumber(dt) or 0.0)
    end
end

function Freecam.getPosition()
    if type(Crabe._freecamGetPosition) == "function" then
        return Crabe._freecamGetPosition()
    end
    return 0, 0, 0
end

function Freecam.setPosition(x, y, z)
    if type(Crabe._freecamSetPosition) == "function" then
        Crabe._freecamSetPosition(tonumber(x) or 0, tonumber(y) or 0, tonumber(z) or 0)
    end
end

function Freecam.getRotation()
    if type(Crabe._freecamGetRotation) == "function" then
        return Crabe._freecamGetRotation()
    end
    return 0, 0, 0
end

function Freecam.setRotation(pitch, yaw, roll)
    if type(Crabe._freecamSetRotation) == "function" then
        Crabe._freecamSetRotation(tonumber(pitch) or 0, tonumber(yaw) or 0, tonumber(roll) or 0)
    end
end

function Freecam.teleportPlayer()
    if type(Crabe._freecamTeleportPlayer) == "function" then
        Crabe._freecamTeleportPlayer()
    end
end

function Freecam.setWorldFrozen(frozen)
    if type(Crabe._freecamSetWorldFrozen) == "function" then
        local flag = (frozen == true or frozen == 1) and 1 or 0
        Crabe._freecamSetWorldFrozen(flag)
    end
end

function Freecam.isWorldFrozen()
    if type(Crabe._freecamIsWorldFrozen) == "function" then
        return Crabe._freecamIsWorldFrozen() == 1
    end
    return false
end
