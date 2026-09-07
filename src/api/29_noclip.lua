-- Real-Time NoClip & Free Fly Engine for Disney Infinity 3.0
--
-- Lets the player fly freely in full 3D space, pass through walls, rise into
-- the sky, and explore out-of-bounds map limits in real-time.
--
-- Controls while active:
--   W / Up Arrow        : Fly Forward
--   S / Down Arrow      : Fly Backward
--   A / Left Arrow      : Fly Left
--   D / Right Arrow     : Fly Right
--   Space / E           : Fly UP
--   Left Ctrl / Q / C   : Fly DOWN
--   Left Shift          : Turbo Speed Boost (x3)

Crabe = Crabe or {}
Crabe.NoClip = Crabe.NoClip or {}

local NoClip = Crabe.NoClip

NoClip.active = false
NoClip.speed = 25.0       -- meters per second
NoClip.lastTick = nil

local KEY = {
    W = 0x57, S = 0x53, A = 0x41, D = 0x44,
    E = 0x45, Q = 0x51, C = 0x43,
    UP = 0x26, DOWN = 0x28, LEFT = 0x25, RIGHT = 0x27,
    SPACE = 0x20, CTRL = 0x11, SHIFT = 0x10,
}

local function isKeyDown(vk)
    return Crabe._keyDown and Crabe._keyDown(vk) == 1
end

local function onNoClipTick()
    if not NoClip.active then return end

    -- Ensure movement structure is captured
    if not Crabe.Cheats.trackPosition() then return end

    local dx, dy, dz = 0, 0, 0

    -- Forward / Backward (Z axis)
    if isKeyDown(KEY.W) or isKeyDown(KEY.UP) then dz = dz + 1.0 end
    if isKeyDown(KEY.S) or isKeyDown(KEY.DOWN) then dz = dz - 1.0 end

    -- Left / Right (X axis)
    if isKeyDown(KEY.A) or isKeyDown(KEY.LEFT) then dx = dx - 1.0 end
    if isKeyDown(KEY.D) or isKeyDown(KEY.RIGHT) then dx = dx + 1.0 end

    -- Up / Down (Y height axis)
    if isKeyDown(KEY.SPACE) or isKeyDown(KEY.E) then dy = dy + 1.0 end
    if isKeyDown(KEY.CTRL) or isKeyDown(KEY.Q) or isKeyDown(KEY.C) then dy = dy - 1.0 end

    if dx == 0 and dy == 0 and dz == 0 then return end

    local currentSpeed = NoClip.speed
    if isKeyDown(KEY.SHIFT) then
        currentSpeed = currentSpeed * 3.0
    end

    -- Fixed timestep step (~16ms for 60fps)
    local dt = 0.033
    local stepX = dx * currentSpeed * dt
    local stepY = dy * currentSpeed * dt
    local stepZ = dz * currentSpeed * dt

    Crabe.Cheats.teleportDelta(stepX, stepY, stepZ)
end

function NoClip.toggle()
    NoClip.active = not NoClip.active
    if NoClip.active then
        -- Automatically arm movement tracking and god mode
        Crabe.Cheats.trackPosition()
        Crabe.Cheats.setGodMode(true)
    end
    return NoClip.active
end

function NoClip.setSpeed(value)
    NoClip.speed = tonumber(value) or 25.0
    return NoClip.speed
end

-- Initialize tick hook when Game.onTick is ready
if type(Game) == "table" and type(Game.onTick) == "function" then
    Game.onTick(onNoClipTick)
end
