-- Game speed, by lying to the clock.
--
-- Crabe.Cheats.setSpeed is a different thing and does not work: it multiplies a
-- velocity the physics recomputes immediately, on a copy of the transform the
-- engine overwrites every frame. This one changes what the game believes about
-- elapsed time, so the whole simulation moves with it.
--
-- That is also its limitation, and it is not a defect to be fixed later: there
-- is no player-only version at this layer. Enemies, animations and physics
-- speed up together, which is why it is called game speed and not run speed.

Crabe = Crabe or {}
Crabe.GameSpeed = Crabe.GameSpeed or {}

local GameSpeed = Crabe.GameSpeed

local function required(name)
    local fn = Crabe[name]
    if type(fn) ~= "function" then
        error("Crabe.GameSpeed: " .. name .. " is missing -- the loader was built without speedhack.cpp", 3)
    end
    return fn
end

-- What the menu cycles through. 1.0 stays first because leaving the cheat has
-- to be the shortest path back, and the slow end is genuinely useful: at 0.25
-- a fight can be read frame by frame.
GameSpeed.PRESETS = { 1, 0.25, 0.5, 2, 3, 5 }

GameSpeed.MIN = 0.1
GameSpeed.MAX = 10.0

function GameSpeed.set(multiplier)
    if type(multiplier) ~= "number" or multiplier ~= multiplier then
        error("Crabe.GameSpeed.set: expected a number", 2)
    end

    if not required("_setGameSpeed")(multiplier) then
        error("Crabe.GameSpeed.set: the clock hooks could not be installed", 2)
    end
    return GameSpeed.get()
end

-- multiplier, installed
function GameSpeed.get()
    local multiplier, active = required("_getGameSpeed")()
    return multiplier or 1.0, (active or 0) ~= 0
end

function GameSpeed.reset()
    return GameSpeed.set(1.0)
end

-- Steps through PRESETS. Returns the new multiplier so a menu entry can show
-- it without a second call.
function GameSpeed.cycle()
    local current = GameSpeed.get()

    local index = 1
    for position, value in ipairs(GameSpeed.PRESETS) do
        if math.abs(value - current) < 0.001 then
            index = position
            break
        end
    end

    local nextValue = GameSpeed.PRESETS[index + 1] or GameSpeed.PRESETS[1]
    GameSpeed.set(nextValue)
    return nextValue
end

-- The call counts are the diagnostic that matters. A clock the game never
-- reads stays at zero, and a speed that appears to do nothing while every
-- counter climbs means the multiplier is being applied to a clock this build
-- does not steer by.
function GameSpeed.status()
    local multiplier, active = GameSpeed.get()
    if not active then
        return string.format("x%.2f (horloges non accrochees)", multiplier)
    end
    return required("_gameSpeedReport")()
end
