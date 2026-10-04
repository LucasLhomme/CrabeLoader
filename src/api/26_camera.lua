-- CrabeLoader
-- File description:
-- Camera control: the engine free camera, the Toy Box editor camera and the customize camera.
-- The free camera is the engine's own; its flight and controls never pass through Lua.
-- Moves a player only on request, to the camera, through Game.PlaceActor in src/api/14_world.lua.
--
-- Authors: @LucasLhomme

-- Camera control.
--
-- Read this before adding anything here: the shipped scripts call NO free
-- camera, debug camera, photo mode, or field-of-view native. An exhaustive
-- sweep of the 1811 shipped scripts found exactly twelve identifiers with
-- "cam" in the name that are ever called, and the 902-native runtime dump has
-- no camera entry point beyond the Customize_* group below.
--
-- The engine itself still carries a real free camera: every player gets a
-- "FreeCam" camera at startup, and only the debug shortcuts that switch to it
-- were stripped. Crabe._engineFreeCamera reaches its state machine directly,
-- which is what Camera.StartFreeCam below uses.
--
-- Two traps that look like the answer and are not:
--
--   VirtualControllers:ToggleDebugCamera is a Scaleform call into a mobile-only
--   SWF. Its backing VirtualController_* natives do not exist in the PC build.
--
--   RBN_TB_CAMERA_DIST_TOGGLE and friends are keymap constants the shipped
--   handlers repurpose -- one of them deletes an object.
--
-- The Toy Box editor camera and the customize camera stay here for what they
-- are: an editor mode and an orbit rig around one object. Neither is a free
-- camera, and the free camera no longer borrows either.

Crabe = Crabe or {}
Crabe.Camera = Crabe.Camera or {}

local Camera = Crabe.Camera

-- ---------------------------------------------------------------------------
-- Editor camera -- fully confirmed
-- ---------------------------------------------------------------------------

-- The three states placebase.lua uses, at lines 643 to 647. These are the only
-- "Editor::" literals anywhere in the shipped scripts, so there is no fourth.
Camera.EDITOR_STATES = { "Editor::IdleMode", "Editor::ObjectMode", "Editor::SparkMode" }

-- Puts the player into a Toy Box editor mode. In editor modes the camera is
-- detached from the avatar, but the player is editing the Toy Box, not flying:
-- for a free camera use Camera.StartFreeCam.
function Camera.SetEditorState(state, playerId)
    local known = false
    for _, name in ipairs(Camera.EDITOR_STATES) do
        if name == state then known = true end
    end
    if not known then
        error("Crabe.Camera.SetEditorState: unknown state '" .. tostring(state) .. "'", 2)
    end

    Crabe.native("Place_SetEditorState", "Crabe.Camera.SetEditorState")(Crabe.hostPlayer(playerId), state)
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
-- StartCustomizeCamera is known to accept. Feeding that native a handle from
-- somewhere else is untested and can take the process down.
function Camera.StartupData(playerId)
    return Crabe.native("Customize_GetStartupData", "Crabe.Camera.StartupData")(Crabe.hostPlayer(playerId))
end

-- Switches the player's view to the customize camera rig, aimed at `handle`.
function Camera.StartCustomizeCamera(handle, isRumpusObject, playerId)
    if handle == nil then
        error("Crabe.Camera.StartCustomizeCamera: an actor handle is required", 2)
    end

    Crabe.native("Customize_StartCustomizeCamera", "Crabe.Camera.StartCustomizeCamera")(
        handle, isRumpusObject == true, Crabe.hostPlayer(playerId))
    return true
end

function Camera.StopCustomizeCamera(playerId)
    Crabe.native("Customize_StopCustomizeCamera", "Crabe.Camera.StopCustomizeCamera")(Crabe.hostPlayer(playerId))
    return true
end

-- Feeds the customize camera two analog axes. customizebase.lua:670 passes the
-- right stick as (playerNum, x, y, 0). The fourth argument is a literal zero at
-- the one and only call site, so its meaning is unknown -- it is left at zero
-- rather than guessed at.
function Camera.Move(dx, dy, playerId)
    Crabe.native("Customize_MoveCamera", "Crabe.Camera.Move")(
        Crabe.hostPlayer(playerId), tonumber(dx) or 0, tonumber(dy) or 0, 0)
end

-- ---------------------------------------------------------------------------
-- Free camera -- the engine's own
-- ---------------------------------------------------------------------------

-- Controls are the engine's, on the player's controller: left stick flies,
-- right stick looks, R1 and R2 raise and lower. While it runs, the engine
-- turns the avatar's controls off, and turns them back on when it stops.

Camera.freeCam = { active = false, playerId = nil, generation = nil }

-- The engine rebuilds its camera scenes on every world load. A free camera
-- switched on in an earlier world went away with it, whatever `active` says.
local function sceneGeneration()
    if type(Crabe._engineSceneGeneration) ~= "function" then return nil end
    return Crabe._engineSceneGeneration()
end

-- Clears a free camera left over from an earlier world, so the next start
-- asks the engine instead of trusting the flag. Calls no native: this can run
-- from a tick in the middle of a world load.
local function forgetStaleFreeCam()
    local freeCam = Camera.freeCam
    if not freeCam.active or freeCam.generation == sceneGeneration() then return end
    freeCam.active = false
    freeCam.playerId = nil
    freeCam.generation = nil
end

-- What the engine says: true/false when `playerId` has a camera scene, nil
-- otherwise (front end, mid-load) or when the loader lacks the native.
local function engineFreeCamOn(playerId)
    if type(Crabe._engineFreeCameraActive) ~= "function" then return nil end
    return Crabe._engineFreeCameraActive(playerId)
end

-- One step of the engine state machine. true/false is the state it is now
-- in; nil means the native is missing or refused (loader.log says why).
local function stepEngineFreeCam(playerId)
    if type(Crabe._engineFreeCamera) ~= "function" then
        error("Crabe.Camera: this loader has no engine free camera native", 3)
    end
    return Crabe._engineFreeCamera(playerId, true)
end

-- Drives the engine free camera to `wanted`. The engine only toggles, so a
-- step that lands on the wrong state is followed by a second one; false on
-- both means the player has no active camera scene, as in the front end.
local function setEngineFreeCam(wanted, playerId)
    local state = stepEngineFreeCam(playerId)
    if state == nil then return nil end
    if state ~= wanted then
        state = stepEngineFreeCam(playerId)
    end
    return state
end

local function adopt(playerId)
    Camera.freeCam.active = true
    Camera.freeCam.playerId = playerId
    Camera.freeCam.generation = sceneGeneration()
end

-- Switches `playerId` (default: the host) to the engine free camera.
-- Returns true when it is on, false when the player has no camera to switch.
function Camera.StartFreeCam(playerId)
    playerId = Crabe.hostPlayer(playerId)
    forgetStaleFreeCam()
    if Camera.freeCam.active then return true end
    -- Already flying, but forgotten by a scene rebuild: toggling would stop it.
    if engineFreeCamOn(playerId) == true then
        adopt(playerId)
        return true
    end

    local state = setEngineFreeCam(true, playerId)
    if state == nil then
        error("Crabe.Camera.StartFreeCam: the engine free camera is unavailable (see loader.log)", 2)
    end
    if state ~= true then return false end

    adopt(playerId)
    if type(Game) == "table" and type(Game.SuppressHud) == "function" then
        Game.SuppressHud(true, playerId)
    end
    return true
end

-- Returns the view to the avatar camera. false when nothing was running.
-- The engine is asked as well as the flag: a scene rebuild clears the flag,
-- and some rebuilds (a master zone streaming its children) keep the camera.
function Camera.StopFreeCam()
    forgetStaleFreeCam()
    local playerId = Camera.freeCam.playerId or Crabe.hostPlayer()
    if not Camera.freeCam.active and engineFreeCamOn(playerId) ~= true then return false end

    Camera.freeCam.active = false
    Camera.freeCam.playerId = nil
    Camera.freeCam.generation = nil

    setEngineFreeCam(false, playerId)
    if type(Game) == "table" and type(Game.SuppressHud) == "function" then
        Game.SuppressHud(false, playerId)
    end
    return true
end

function Camera.IsFreeCamActive()
    forgetStaleFreeCam()
    local engine = engineFreeCamOn(Camera.freeCam.playerId or Crabe.hostPlayer())
    if engine ~= nil then return engine end
    return Camera.freeCam.active
end

function Camera.ToggleFreeCam(playerId)
    if Camera.IsFreeCamActive() then
        Camera.StopFreeCam()
        return false
    end
    return Camera.StartFreeCam(playerId)
end

-- ---------------------------------------------------------------------------
-- Camera position
-- ---------------------------------------------------------------------------

-- x, y, z of `playerId`'s current camera -- the free camera while it runs --
-- y being height. nil when the player has no camera, as in the front end.
function Camera.GetPosition(playerId)
    if type(Crabe._cameraEye) ~= "function" then
        error("Crabe.Camera.GetPosition: this loader has no camera position native", 2)
    end
    return Crabe._cameraEye(Crabe.hostPlayer(playerId))
end

-- Drops `playerId`'s avatar where their camera is, then hands the view back
-- to it if the free camera was running. The avatar falls from there. Returns
-- true when it was placed, or false and a reason.
function Camera.MovePlayerToCamera(playerId)
    playerId = Crabe.hostPlayer(playerId)

    local x, y, z = Camera.GetPosition(playerId)
    if not x then return false, "no camera" end

    local handle = Game.GetAvatarHandle(playerId)
    if type(handle) ~= "number" or handle == 0 then return false, "no avatar" end
    if not Game.PlaceActor(handle, x, y, z) then return false, "the engine refused" end

    if Camera.IsFreeCamActive() then
        Camera.StopFreeCam()
    end
    return true
end
