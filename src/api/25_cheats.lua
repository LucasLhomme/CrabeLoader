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

-- Which health component the invulnerability clamp protects.
--
-- The player is not identified by how much health it has. The avatar meter and
-- the NPC meter are the same engine function, so every character streams
-- through one site and no health value separates them. Instead every component
-- is catalogued as it goes past, and the most frequently drawn one is taken as
-- the player: the avatar bar is on screen every frame, an enemy bar only while
-- that enemy is engaged.
--
-- Everything below exists so that pick can be checked and overridden rather
-- than believed.

-- Installs the god-mode caves without enabling anything.
--
-- This has to run early. The clamp protects one component, and the registry
-- only learns which one by watching the HUD draw health bars -- so arming at
-- the first toggle is too late, the frames have gone by.
--
-- A mod calls this at load time. It cannot live in this file's body: api/*.lua
-- is injected before the Crabe._* natives are registered, so nothing here can
-- call one until a mod runs.
function Cheats.arm()
    -- Installing is a side effect of setting the gate; asking for "off" arms
    -- the caves and leaves the game behaving exactly as before.
    if not required("_setGodMode")(false) then
        error("Crabe.Cheats.arm: the god-mode signatures did not resolve in this build", 2)
    end
    return true
end

-- Toggles the damage clamp. Call Cheats.arm() at mod load first, or the caves
-- go in too late for the registry to have seen anything.
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

-- target, manual, count, lastDamaged, lastDelta, blockedHits.
function Cheats.targetInfo()
    return required("_targetInfo")()
end

-- Every catalogued component, most-seen first. Row 1 is the automatic pick.
function Cheats.entities()
    local count = required("_entityCount")()
    local at = required("_entityAt")
    local rows = {}

    for index = 1, (count or 0) do
        local component, maxHealth, health, seen, damaged = at(index)
        if component then
            rows[#rows + 1] = {
                component = component,
                maxHealth = maxHealth,
                health = health,
                seen = seen,
                damaged = damaged,
            }
        end
    end
    return rows
end

-- Pins the clamp to one component. Passing nil or 0 restores the automatic
-- pick.
function Cheats.selectTarget(component)
    return required("_selectTarget")(tonumber(component) or 0)
end

-- Hands the choice back to the automatic pick.
function Cheats.resetTarget()
    return Cheats.selectTarget(0)
end

-- Pins the clamp to the last character that BOTH lost health and is being
-- drawn by the HUD.
--
-- The "seen" condition is the whole point. Locking onto whatever last took
-- damage sounds right and is wrong: in a fight the last thing hurt is the
-- enemy that just died, so this used to pin invulnerability to a corpse and
-- report success. Only the player's health bar is drawn continuously, so
-- requiring seen > 0 is what makes this mean "the player".
function Cheats.lockToLastDamaged()
    local rows = Cheats.entities()
    local best

    for _, row in ipairs(rows) do
        if row.damaged and row.damaged > 0 and row.seen and row.seen > 0
           and row.health and row.health > 0 then
            if not best or row.seen > best.seen then best = row end
        end
    end

    if not best then
        error("Crabe.Cheats.lockToLastDamaged: nothing that the HUD tracks has " ..
              "taken damage yet -- take a hit yourself with invulnerability off", 2)
    end
    return Cheats.selectTarget(best.component)
end

function Cheats.godModeStatus()
    local state = Cheats.godMode() and "ON" or "OFF"
    local target, manual, count, lastDamaged, lastDelta, blocked = Cheats.targetInfo()

    -- Counted by the clamp cave itself. This is the one number that answers
    -- "did the cheat do anything", so it leads.
    state = state .. string.format(" [%d hits cancelled]", blocked or 0)

    if not target or target == 0 then
        if not count or count == 0 then
            return state .. " - no health bar seen yet, enter gameplay"
        end
        return string.format("%s - %d seen but none current", state, count)
    end

    local how = (manual and manual ~= 0) and "PINNED" or "auto"
    local line = string.format("%s - %s 0x%X of %d", state, how, target, count or 0)

    -- A pin can point at something dead or at something the HUD never draws.
    -- Both look like a working cheat and protect nothing, so say it plainly.
    for _, row in ipairs(Cheats.entities()) do
        if row.component == target then
            line = line .. string.format(" (%.0f/%.0f hp, seen %d)",
                row.health or 0, row.maxHealth or 0, row.seen or 0)
            if row.health and row.health <= 0 then
                return line .. "  <- DEAD, use Reset to automatic"
            end
            if (row.seen or 0) == 0 then
                return line .. "  <- never drawn by the HUD, not the player"
            end
            break
        end
    end

    if lastDamaged and lastDamaged ~= 0 and lastDamaged ~= target then
        line = line .. string.format(" | last hit 0x%X %.1f", lastDamaged, lastDelta or 0)
    end
    return line
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

-- multiplier, plus a per-site execution count as "a/b/c/d".
function Cheats.speed()
    return required("_getSpeedMultiplier")()
end

-- Reads whether the four velocity caves are being executed at all. All zeroes
-- means the inherited signatures sit on code this build never runs, which is a
-- different problem from a multiplier that has no visible effect.
function Cheats.speedStatus()
    local multiplier, hits = Cheats.speed()
    return string.format("x%s - site executions %s", tostring(multiplier), tostring(hits))
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

-- ---------------------------------------------------------------------------
function Cheats.setPlayerFloat(offset, value)
    if type(offset) ~= "number" or type(value) ~= "number" then
        error("Crabe.Cheats.setPlayerFloat: expected (offset, value)", 2)
    end
    if not required("_setPlayerFloat")(offset, value) then
        error("Crabe.Cheats.setPlayerFloat: nothing captured, or the address is not writable", 2)
    end
    return true
end

-- Looks for the character's gameplay attributes around the health component.
--
-- The game stores these as named values: the data files give Dash health 150
-- and runspeed 7, and that 150 is exactly the number the old god mode used to
-- recognise the player. So health is an attribute, the registry already points
-- at it, and run speed is very likely a short hop away in the same block.
--
-- Everything outside a plausible range is dropped. A run speed is single digits
-- to low tens; pointers, flags and timers are not.
function Cheats.findAttributes(low, high)
    low = low or -0x200
    high = high or 0x400

    if not Cheats.playerObject() then
        error("Crabe.Cheats.findAttributes: no player captured -- take a hit with " ..
              "invulnerability off, or open the HUD", 2)
    end

    local rows = {}
    for offset = low, high, 4 do
        local value = Cheats.playerFloat(offset)
        if value and value == value and math.abs(value) >= 0.5 and math.abs(value) <= 500 then
            rows[#rows + 1] = { offset = offset, value = value }
        end
    end
    return rows
end

-- Finding the player's position
-- ---------------------------------------------------------------------------

-- The velocity caves hand us a live pointer to the player's movement
-- structure. Position is somewhere inside it at an offset nobody has
-- identified, which is what has blocked teleport and noclip all along.
--
-- The method is the one a Cheat Engine user would use: snapshot the structure,
-- move, then diff. Three adjacent floats that all change together are the
-- position; a single one that changes is more likely a speed or a timer.
-- Captures the movement structure without enabling the speed cheat.
--
-- The pointer is a side effect of the velocity caves, so reaching the player's
-- position used to mean turning on a multiplier that does not work. Position
-- and speed are unrelated problems and this stops pretending otherwise.
function Cheats.trackPosition()
    if not required("_trackPosition")() then
        error("Crabe.Cheats.trackPosition: the movement signatures did not resolve in this build", 2)
    end
    return true
end

function Cheats.moveObject()
    return required("_moveObject")()
end

function Cheats.snapshotMove()
    if not required("_moveSnapshot")() then
        error("Crabe.Cheats.snapshotMove: no movement structure captured yet -- " ..
              "set a movement speed once so the caves install, then walk", 2)
    end
    return true
end

-- `floor` filters out fields that barely move: timers and blend weights drift
-- constantly and would bury the position under noise.
function Cheats.diffMove(floor)
    return required("_moveDiff")(tonumber(floor) or 0.5)
end

-- x, y, z at the offsets the walk test marked out, or nil before the movement
-- caves have captured a structure.
function Cheats.position()
    return required("_position")()
end

function Cheats.setPosition(x, y, z)
    if type(Crabe._setPosition) == "function" then
        return Crabe._setPosition(tonumber(x) or 0, tonumber(y) or 0, tonumber(z) or 0)
    end
    return false
end

function Cheats.teleportDelta(dx, dy, dz)
    if type(Crabe._teleportDelta) == "function" then
        return Crabe._teleportDelta(tonumber(dx) or 0, tonumber(dy) or 0, tonumber(dz) or 0)
    end
    return false
end

function Cheats.teleportUp(dy)
    return Cheats.teleportDelta(0, tonumber(dy) or 10.0, 0)
end

function Cheats.teleportForward(dist)
    dist = tonumber(dist) or 15.0
    if type(Crabe._freecamGetRotation) == "function" then
        local p, yaw, r = Crabe._freecamGetRotation()
        if yaw then
            local dx = math.sin(yaw) * dist
            local dz = math.cos(yaw) * dist
            return Cheats.teleportDelta(dx, 0, dz)
        end
    end
    return Cheats.teleportDelta(0, 0, dist)
end

function Cheats.unlockEditor()
    if type(Crabe._unlockEditor) == "function" then
        return Crabe._unlockEditor()
    end
    return false
end
