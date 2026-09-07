-- Camera control.
--
-- Read this before adding anything here: Disney Infinity 3.0 exposes NO free
-- camera, debug camera, photo mode, or field-of-view native. An exhaustive
-- sweep of the 1811 shipped scripts found exactly twelve identifiers with
-- "cam" in the name that are ever called, and the 902-native runtime dump has
-- no camera entry point beyond the Customize_* group below.
--
-- Two traps that look like the answer and are not:
--
--   VirtualControllers:ToggleDebugCamera is a Scaleform call into a mobile-only
--   SWF. Its backing VirtualController_* natives do not exist in the PC build.
--
--   RBN_TB_CAMERA_DIST_TOGGLE and friends are keymap constants the shipped
--   handlers repurpose -- one of them deletes an object.
--
-- So what is here is the customize camera, which is the only thing in the game
-- that takes camera motion from Lua, plus the Toy Box editor camera, which is
-- the only shipped camera genuinely detached from the avatar.

Crabe = Crabe or {}
Crabe.Camera = Crabe.Camera or {}

local Camera = Crabe.Camera

local function native(name, caller)
    local fn = _G[name]
    if type(fn) ~= "function" then
        error(caller .. ": " .. name .. " is not available in this Lua state", 3)
    end
    return fn
end

local function hostPlayer(playerId)
    if playerId then return playerId end
    return native("Players_GetHostPlayerID", "Crabe.Camera")()
end

-- ---------------------------------------------------------------------------
-- Editor camera -- fully confirmed
-- ---------------------------------------------------------------------------

-- The three states placebase.lua uses, at lines 643 to 647. These are the only
-- "Editor::" literals anywhere in the shipped scripts, so there is no fourth.
Camera.EDITOR_STATES = { "Editor::IdleMode", "Editor::ObjectMode", "Editor::SparkMode" }

-- Puts the player into a Toy Box editor mode. In editor modes the camera is
-- detached from the avatar, which is the closest thing to a free camera that
-- the game ships and reaches in a single confirmed call.
function Camera.SetEditorState(state, playerId)
    local known = false
    for _, name in ipairs(Camera.EDITOR_STATES) do
        if name == state then known = true end
    end
    if not known then
        error("Crabe.Camera.SetEditorState: unknown state '" .. tostring(state) .. "'", 2)
    end

    native("Place_SetEditorState", "Crabe.Camera.SetEditorState")(hostPlayer(playerId), state)
    return state
end

-- ---------------------------------------------------------------------------
-- Customize camera -- the only camera motion native in the game
-- ---------------------------------------------------------------------------

-- What the customize screen is holding for this player, as five values:
-- actor handle, object name, thrown-in-building flag, rumpus flag, filter.
-- Shape taken from customizebase.lua:148.
--
-- This is the only shipped source of an actor handle that
-- StartCustomizeCamera is known to accept, which is why the free camera below
-- refuses to start without it. Feeding that native a handle from somewhere
-- else is untested and can take the process down.
function Camera.StartupData(playerId)
    return native("Customize_GetStartupData", "Crabe.Camera.StartupData")(hostPlayer(playerId))
end

-- Switches the player's view to the customize camera rig, aimed at `handle`.
function Camera.StartCustomizeCamera(handle, isRumpusObject, playerId)
    if handle == nil then
        error("Crabe.Camera.StartCustomizeCamera: an actor handle is required", 2)
    end

    native("Customize_StartCustomizeCamera", "Crabe.Camera.StartCustomizeCamera")(
        handle, isRumpusObject == true, hostPlayer(playerId))
    return true
end

function Camera.StopCustomizeCamera(playerId)
    native("Customize_StopCustomizeCamera", "Crabe.Camera.StopCustomizeCamera")(hostPlayer(playerId))
    return true
end

-- Feeds the customize camera two analog axes. customizebase.lua:670 passes the
-- right stick as (playerNum, x, y, 0). The fourth argument is a literal zero at
-- the one and only call site, so its meaning is unknown -- it is left at zero
-- rather than guessed at.
function Camera.Move(dx, dy, playerId)
    native("Customize_MoveCamera", "Crabe.Camera.Move")(
        hostPlayer(playerId), tonumber(dx) or 0, tonumber(dy) or 0, 0)
end

-- ---------------------------------------------------------------------------
-- Free camera
-- ---------------------------------------------------------------------------

-- Virtual-key codes, so the bindings below read as names.
local KEY = {
    LEFT = 0x25, UP = 0x26, RIGHT = 0x27, DOWN = 0x28,
    SHIFT = 0x10,
}

Camera.freeCam = { active = false, speed = 1.0, playerId = nil }

local function keyDown(vk)
    return Crabe._keyDown and Crabe._keyDown(vk) == 1
end

-- Steers the customize camera from the arrow keys, once per tick.
--
-- Held-key state has to come from the loader rather than the engine: the game
-- never hands key state to Lua, and while the menu is open the overlay owns
-- the keyboard anyway.
local function driveFreeCam()
    if not Camera.freeCam.active then return end

    local dx, dy = 0, 0
    if keyDown(KEY.LEFT) then dx = dx - 1 end
    if keyDown(KEY.RIGHT) then dx = dx + 1 end
    if keyDown(KEY.UP) then dy = dy + 1 end
    if keyDown(KEY.DOWN) then dy = dy - 1 end

    if dx == 0 and dy == 0 then return end

    local speed = Camera.freeCam.speed
    if keyDown(KEY.SHIFT) then speed = speed * 3 end

    Camera.Move(dx * speed, dy * speed, Camera.freeCam.playerId)
end

-- Starts the free camera on whatever the customize screen is currently holding.
--
-- EXPERIMENTAL, and honestly so: the rig this borrows is an orbit camera built
-- for inspecting one object. Whether it can be flown, and whether it clamps to
-- a distance around its target, is unverified -- no shipped script uses it for
-- anything but customization.
function Camera.StartFreeCam(playerId)
    if Camera.freeCam.active then return true end

    local handle, name, thrown, isRumpus = Camera.StartupData(playerId)
    if handle == nil then
        error("Crabe.Camera.StartFreeCam: no actor handle is available -- this " ..
              "camera can only attach to what the customize screen is holding", 2)
    end

    Camera.StartCustomizeCamera(handle, isRumpus, playerId)
    Camera.freeCam.active = true
    Camera.freeCam.playerId = playerId

    if type(Game) == "table" and type(Game.SuppressHud) == "function" then
        Game.SuppressHud(true, playerId)
    end
    return true, name, thrown
end

function Camera.StopFreeCam()
    if not Camera.freeCam.active then return false end

    local playerId = Camera.freeCam.playerId
    Camera.freeCam.active = false
    Camera.freeCam.playerId = nil

    Camera.StopCustomizeCamera(playerId)
    if type(Game) == "table" and type(Game.SuppressHud) == "function" then
        Game.SuppressHud(false, playerId)
    end
    return true
end

function Camera.SetSpeed(value)
    Camera.freeCam.speed = tonumber(value) or 1.0
    return Camera.freeCam.speed
end

-- The tick hook is registered from a mod, not here: this file is injected
-- before Game.onTick exists.
function Camera.arm()
    if type(Game) ~= "table" or type(Game.onTick) ~= "function" then
        error("Crabe.Camera.arm: Game.onTick is not available yet", 2)
    end
    Game.onTick(driveFreeCam)
    return true
end
