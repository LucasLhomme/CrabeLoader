-- CrabeLoader
-- File description:
-- Camera control: the engine free camera, the Toy Box editor camera and the customize camera.
-- The free camera is the engine's own; its flight and controls never pass through Lua.
-- Moves a player only on request, to the camera, through Game.PlaceActor in src/api/14_world.lua.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Camera = Crabe.Camera or {}

local Camera = Crabe.Camera

Camera.EDITOR_STATES = { "Editor::IdleMode", "Editor::ObjectMode", "Editor::SparkMode" }

-- Puts the player into a Toy Box editor mode.
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

-- Returns customize screen startup data tuple for the player.
function Camera.StartupData(playerId)
    return Crabe.native("Customize_GetStartupData", "Crabe.Camera.StartupData")(Crabe.hostPlayer(playerId))
end

-- Switches player view to customize camera rig aimed at actor handle.
function Camera.StartCustomizeCamera(handle, isRumpusObject, playerId)
    if handle == nil then
        error("Crabe.Camera.StartCustomizeCamera: an actor handle is required", 2)
    end

    Crabe.native("Customize_StartCustomizeCamera", "Crabe.Camera.StartCustomizeCamera")(
        handle, isRumpusObject == true, Crabe.hostPlayer(playerId))
    return true
end

-- Stops customize camera and returns view to normal.
function Camera.StopCustomizeCamera(playerId)
    Crabe.native("Customize_StopCustomizeCamera", "Crabe.Camera.StopCustomizeCamera")(Crabe.hostPlayer(playerId))
    return true
end

-- Feeds customize camera two analog motion axes.
function Camera.Move(dx, dy, playerId)
    Crabe.native("Customize_MoveCamera", "Crabe.Camera.Move")(
        Crabe.hostPlayer(playerId), tonumber(dx) or 0, tonumber(dy) or 0, 0)
end

Camera.freeCam = { active = false, playerId = nil, generation = nil }

-- Helper returning current engine camera scene generation number.
local function sceneGeneration()
    if type(Crabe._engineSceneGeneration) ~= "function" then return nil end
    return Crabe._engineSceneGeneration()
end

-- Clears cached free camera state if scene generation changed.
local function forgetStaleFreeCam()
    local freeCam = Camera.freeCam
    if not freeCam.active or freeCam.generation == sceneGeneration() then return end
    freeCam.active = false
    freeCam.playerId = nil
    freeCam.generation = nil
end

-- Checks if engine free camera is currently active for player.
local function engineFreeCamOn(playerId)
    if type(Crabe._engineFreeCameraActive) ~= "function" then return nil end
    return Crabe._engineFreeCameraActive(playerId)
end

-- Steps the engine free camera state machine.
local function stepEngineFreeCam(playerId)
    if type(Crabe._engineFreeCamera) ~= "function" then
        error("Crabe.Camera: this loader has no engine free camera native", 3)
    end
    return Crabe._engineFreeCamera(playerId, true)
end

-- Drives engine free camera state machine to desired state.
local function setEngineFreeCam(wanted, playerId)
    local state = stepEngineFreeCam(playerId)
    if state == nil then return nil end
    if state ~= wanted then
        state = stepEngineFreeCam(playerId)
    end
    return state
end

-- Records active free camera player and generation.
local function adopt(playerId)
    Camera.freeCam.active = true
    Camera.freeCam.playerId = playerId
    Camera.freeCam.generation = sceneGeneration()
end

-- Switches player view to engine free camera, suppressing HUD.
function Camera.StartFreeCam(playerId)
    playerId = Crabe.hostPlayer(playerId)
    forgetStaleFreeCam()
    if Camera.freeCam.active then return true end
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

-- Returns view from free camera to avatar and restores HUD.
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

-- Returns true if free camera is currently active.
function Camera.IsFreeCamActive()
    forgetStaleFreeCam()
    local engine = engineFreeCamOn(Camera.freeCam.playerId or Crabe.hostPlayer())
    if engine ~= nil then return engine end
    return Camera.freeCam.active
end

-- Toggles free camera state on or off.
function Camera.ToggleFreeCam(playerId)
    if Camera.IsFreeCamActive() then
        Camera.StopFreeCam()
        return false
    end
    return Camera.StartFreeCam(playerId)
end

-- Returns x, y, z coordinates of player current camera.
function Camera.GetPosition(playerId)
    if type(Crabe._cameraEye) ~= "function" then
        error("Crabe.Camera.GetPosition: this loader has no camera position native", 2)
    end
    return Crabe._cameraEye(Crabe.hostPlayer(playerId))
end

-- Teleports player avatar to current camera position.
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
