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
