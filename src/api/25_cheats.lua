-- Invulnerability and movement speed. Neither exists as a game native: the
-- engine never exposed health or velocity to Lua, so both go through code
-- caves installed by src/cheats.cpp. This file is only the ergonomic face of
-- Crabe._setGodMode / _setSpeedMultiplier and friends.

Crabe = Crabe or {}
Crabe.Cheats = Crabe.Cheats or {}

local Cheats = Crabe.Cheats

-- The C natives return nothing when an AOB failed to resolve, which is the
-- only failure mode that matters: the game build does not match the patterns.
-- Turning that into a named error beats a silent no-op the player cannot see.
local function required(name)
    local fn = Crabe[name]
    if type(fn) ~= "function" then
        error("Crabe.Cheats: " .. name .. " is missing -- the loader was built without cheats.cpp", 3)
    end
    return fn
end

-- Multipliers the menu cycles through. 1.0 restores the stock velocity, which
-- is why it has to stay first: leaving the cheat is the default.
Cheats.SPEEDS = { 1, 2, 5, 10 }

-- Toggles the damage clamp. The caves are installed on first call, so nothing
-- in the game's code is touched until the player actually asks for this.
function Cheats.setGodMode(enabled)
    if type(enabled) ~= "boolean" then
        error("Crabe.Cheats.setGodMode: expected a boolean", 2)
    end

    if not required("_setGodMode")(enabled) then
        error("Crabe.Cheats.setGodMode: the god-mode signatures did not resolve in this build", 2)
    end
    return enabled
end

function Cheats.godMode()
    return required("_getGodMode")() == 1
end

-- Scales the velocity written by the analog-stick stores. Values outside a
-- sane range are refused by the C++ side, which reports the value it kept.
function Cheats.setSpeed(multiplier)
    multiplier = tonumber(multiplier)
    if not multiplier then
        error("Crabe.Cheats.setSpeed: expected a number", 2)
    end

    local applied = required("_setSpeedMultiplier")(multiplier)
    if not applied then
        error("Crabe.Cheats.setSpeed: the movement signatures did not resolve in this build", 2)
    end
    return applied
end

function Cheats.speed()
    return required("_getSpeedMultiplier")()
end

-- Address of the entity the capture cave recognised as the player, or nil
-- until the HUD has drawn a health bar once. Mostly a probe for the console.
function Cheats.playerObject()
    return required("_playerObject")()
end

-- Reads a float out of the player struct. The known layout is health at 0x08
-- and maxHealth at 0x0C; velocity Y/X sit at 0x298/0x29C. Everything else is
-- unmapped, which is exactly what this is for.
function Cheats.playerFloat(offset)
    offset = tonumber(offset)
    if not offset then
        error("Crabe.Cheats.playerFloat: expected a numeric offset", 2)
    end
    return required("_playerFloat")(offset)
end

-- Dumps a window of the player struct as { offset = value } rows. Position has
-- never been located; velocity is at 0x298, so a sweep around it is the cheapest
-- way to spot three consecutive floats that move when the player does.
function Cheats.probe(from, to, step)
    from = tonumber(from) or 0x280
    to = tonumber(to) or 0x2C0
    step = tonumber(step) or 4

    if not Cheats.playerObject() then
        error("Crabe.Cheats.probe: no player object captured yet -- take damage or open the HUD first", 2)
    end

    local rows = {}
    for offset = from, to, step do
        local value = Cheats.playerFloat(offset)
        if value then
            rows[#rows + 1] = { offset = offset, value = value }
        end
    end
    return rows
end
