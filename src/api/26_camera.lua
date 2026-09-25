-- CrabeLoader
-- File description:
-- Camera control: the engine free camera, the Toy Box editor camera and the customize camera.
-- The free camera is the engine's own; its flight and controls never pass through Lua.
-- Moves no player -- position is read through src/api/14_world.lua and owned by the engine.
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

Camera.freeCam = { active = false, playerId = nil }

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

-- Switches `playerId` (default: the host) to the engine free camera.
-- Returns true when it is on, false when the player has no camera to switch.
function Camera.StartFreeCam(playerId)
    playerId = Crabe.hostPlayer(playerId)
    if Camera.freeCam.active then return true end

    local state = setEngineFreeCam(true, playerId)
    if state == nil then
        error("Crabe.Camera.StartFreeCam: the engine free camera is unavailable (see loader.log)", 2)
    end
    if state ~= true then return false end

    Camera.freeCam.active = true
    Camera.freeCam.playerId = playerId
    if type(Game) == "table" and type(Game.SuppressHud) == "function" then
        Game.SuppressHud(true, playerId)
    end
    return true
end

-- Returns the view to the avatar camera. false when nothing was running.
function Camera.StopFreeCam()
    if not Camera.freeCam.active then return false end

    local playerId = Camera.freeCam.playerId
    Camera.freeCam.active = false
    Camera.freeCam.playerId = nil

    setEngineFreeCam(false, playerId)
    if type(Game) == "table" and type(Game.SuppressHud) == "function" then
        Game.SuppressHud(false, playerId)
    end
    return true
end

function Camera.IsFreeCamActive()
    return Camera.freeCam.active
end

function Camera.ToggleFreeCam(playerId)
    if Camera.freeCam.active then
        Camera.StopFreeCam()
        return false
    end
    return Camera.StartFreeCam(playerId)
end
