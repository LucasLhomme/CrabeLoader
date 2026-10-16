-- CrabeLoader
-- File description:
-- Worlds and zones: reading where the player is, listing levels, and moving them elsewhere.
-- A level load tears down the Lua-visible world, so anything held across one must be re-resolved.
-- Spawns nothing on arrival -- that is src/api/13_spawn.lua.
--
-- Authors: @LucasLhomme

Game = Game or {}

-- Splits a comma-separated string into a list of trimmed strings.
function Crabe.splitList(csv)
    local out = {}
    if type(csv) ~= "string" then return out end

    for field in string.gmatch(csv, "([^,]+)") do
        local trimmed = string.gsub(field, "^%s*(.-)%s*$", "%1")
        if trimmed ~= "" then out[#out + 1] = trimmed end
    end
    return out
end

-- Moves the actor to x, y, z preserving facing, returns true on success.
function Game.PlaceActor(handle, x, y, z)
    if type(Crabe._actorPlace) ~= "function" then
        error("Game.PlaceActor: this loader has no actor placement native", 2)
    end
    if type(handle) ~= "number" or type(x) ~= "number" or type(y) ~= "number" or type(z) ~= "number" then
        error("Game.PlaceActor: expected handle, x, y, z as numbers", 2)
    end
    return Crabe._actorPlace(handle, x, y, z) == true
end

-- Returns x, y, z coordinates of the kinematic actor, or nil.
function Game.ActorPosition(handle)
    if type(Crabe._actorPosition) ~= "function" then
        error("Game.ActorPosition: this loader has no actor position native", 2)
    end
    if type(handle) ~= "number" then
        error("Game.ActorPosition: expected an actor handle (number)", 2)
    end
    return Crabe._actorPosition(handle)
end

-- Creates an actor from engine parameter string at x, y, z facing heading.
function Game.CreateActor(parameters, x, y, z, heading)
    if type(Crabe._actorCreate) ~= "function" then
        error("Game.CreateActor: this loader has no actor creation native", 2)
    end
    if type(parameters) ~= "string" or parameters == "" then
        error("Game.CreateActor: parameters must be a non-empty string", 2)
    end
    if type(x) ~= "number" or type(y) ~= "number" or type(z) ~= "number" then
        error("Game.CreateActor: expected x, y, z as numbers", 2)
    end
    return Crabe._actorCreate(parameters, x, y, z, heading or 0)
end

-- Sets or clears a named ActorState bit on an actor.
function Game.SetActorState(handle, state, on)
    if type(Crabe._actorSetState) ~= "function" then
        error("Game.SetActorState: this loader has no actor state native", 2)
    end
    if type(handle) ~= "number" or type(state) ~= "string" then
        error("Game.SetActorState: expected an actor handle and a state name", 2)
    end
    return Crabe._actorSetState(handle, state, on ~= false) == true
end

-- Returns whether an actor carries a named ActorState bit.
function Game.ActorHasState(handle, state)
    if type(Crabe._actorTestState) ~= "function" then
        error("Game.ActorHasState: this loader has no actor state native", 2)
    end
    if type(handle) ~= "number" or type(state) ~= "string" then
        error("Game.ActorHasState: expected an actor handle and a state name", 2)
    end
    return Crabe._actorTestState(handle, state)
end

-- Sets an actor's movement speed multiplier; 1.0 is normal speed and 20.0 the ceiling.
-- Unlike the game clock it scales only this actor, so it can speed up the player alone.
function Game.SetActorSpeedScale(handle, factor)
    if type(Crabe._actorSetSpeedScale) ~= "function" then
        error("Game.SetActorSpeedScale: this loader has no actor speed native", 2)
    end
    if type(handle) ~= "number" or type(factor) ~= "number" then
        error("Game.SetActorSpeedScale: expected an actor handle and a factor as numbers", 2)
    end
    return Crabe._actorSetSpeedScale(handle, factor) == true
end

-- Returns an actor's movement speed multiplier, or nil when it cannot be read.
function Game.ActorSpeedScale(handle)
    if type(Crabe._actorSpeedScale) ~= "function" then
        error("Game.ActorSpeedScale: this loader has no actor speed native", 2)
    end
    if type(handle) ~= "number" then
        error("Game.ActorSpeedScale: expected an actor handle (number)", 2)
    end
    return Crabe._actorSpeedScale(handle)
end

-- Creates actors in front of the player camera, spread side by side.
function Game.SpawnActorNearPlayer(parameters, count, distance, playerId)
    count = count or 1
    distance = distance or 4
    local handle = Game.GetAvatarHandle(playerId)
    local px, py, pz = Game.ActorPosition(handle)
    if not px then
        error("Game.SpawnActorNearPlayer: the player's position cannot be read", 2)
    end
    local cx, _, cz = Crabe._cameraEye(Crabe.hostPlayer(playerId))
    if not cx then
        error("Game.SpawnActorNearPlayer: the player's camera cannot be read", 2)
    end

    local dx, dz = px - cx, pz - cz
    local length = math.sqrt(dx * dx + dz * dz)
    if length < 0.001 then dx, dz, length = 0, 1, 1 end
    dx, dz = dx / length, dz / length
    local heading = math.atan2(-dx, -dz)

    local built = 0
    for index = 1, count do
        local side = (index - (count + 1) / 2) * 1.5
        local x = px + dx * distance - dz * side
        local z = pz + dz * distance + dx * side
        if Game.CreateActor(parameters, x, py, z, heading) then built = built + 1 end
    end
    return built
end

-- Returns the current world name.
function Game.CurrentWorld()
    return Crabe.native("UI_CurrentWorldName", "Game.CurrentWorld")()
end

-- Replaces current sky and lighting with the specified realm name.
function Game.LoadSkyDome(realmName)
    if type(realmName) ~= "string" or realmName == "" then
        error("Game.LoadSkyDome: realmName must be a non-empty string", 2)
    end
    if type(Crabe._loadSkyDome) ~= "function" then
        error("Game.LoadSkyDome: this loader has no sky native", 2)
    end
    return Crabe._loadSkyDome(realmName) == true
end

-- Returns the zone name the player is currently in.
function Game.CurrentZone(playerId)
    return Crabe.native("UI_GetPlayerZoneName", "Game.CurrentZone")(Crabe.hostPlayer(playerId))
end

-- Reads string metadata for the given zone, defaulting to current zone.
function Game.ZoneString(key, zone)
    if type(key) ~= "string" or key == "" then
        error("Game.ZoneString: key must be a non-empty string", 2)
    end
    return Crabe.native("UI_GetZoneMgrString", "Game.ZoneString")(zone or "<current>", key)
end

-- Reads boolean metadata for the given zone, defaulting to current zone.
function Game.ZoneBool(key, zone)
    if type(key) ~= "string" or key == "" then
        error("Game.ZoneBool: key must be a non-empty string", 2)
    end
    return Crabe.native("UI_GetZoneMgrBool", "Game.ZoneBool")(zone or "<current>", key)
end

-- Reads integer metadata for the given zone, defaulting to current zone.
function Game.ZoneInt(key, zone)
    if type(key) ~= "string" or key == "" then
        error("Game.ZoneInt: key must be a non-empty string", 2)
    end
    return Crabe.native("UI_GetZoneMgrInt", "Game.ZoneInt")(zone or "<current>", key)
end

-- Returns table of boolean flags describing the current world shape.
function Game.WorldKind()
    local kind = {}
    local checks = {
        playset = "UI_IsWorldAPlayset",
        userToyBox = "UI_IsWorldAUserToyBox",
        dungeon = "UI_IsCurrentWorldADungeon",
        transition = "UI_IsCurrentWorldATransition",
        tutorial = "UI_IsCurrentWorldATutorial",
        intro = "UI_IsCurrentWorldAnIntro",
        defense = "UI_IsCurrentWorldDefense",
        inSpace = "UI_IsGameInSpace",
    }

    for label, fn in pairs(checks) do
        if type(_G[fn]) == "function" then kind[label] = _G[fn]() end
    end
    return kind
end

-- Returns list of level names and count from engine level select.
function Game.ListLevels(filter, playerId)
    local csv, count = Crabe.native("UI_GetListPlayerIndx", "Game.ListLevels")(
        "Levels", filter or "", Crabe.hostPlayer(playerId))
    return Crabe.splitList(csv), count
end

-- Returns true if the engine permits transition to the level.
function Game.CanLoadLevel(levelName)
    if type(levelName) ~= "string" or levelName == "" then
        error("Game.CanLoadLevel: levelName must be a non-empty string", 2)
    end
    return Crabe.native("UI_CanTransitionToLevel", "Game.CanLoadLevel")(levelName) == true
end

-- Helper to deactivate free camera before level transitions.
local function releaseFreeCam()
    local Camera = Crabe.Camera
    if Camera and type(Camera.IsFreeCamActive) == "function" and Camera.IsFreeCamActive() then
        Camera.StopFreeCam()
    end
end

-- Launches transition to the specified level name.
function Game.LoadLevel(levelName, force)
    if type(levelName) ~= "string" or levelName == "" then
        error("Game.LoadLevel: levelName must be a non-empty string", 2)
    end

    if not force and not Game.CanLoadLevel(levelName) then
        error("Game.LoadLevel: the game refuses to transition to '" .. levelName .. "'", 2)
    end

    releaseFreeCam()
    Crabe.native("UI_LaunchLevel", "Game.LoadLevel")(levelName, "world", true, true, false)
    return levelName
end

-- Returns true when the Play Set has a saved game the engine can resume.
function Game.CanContinuePlayset(playset)
    if type(playset) ~= "string" or playset == "" then
        error("Game.CanContinuePlayset: playset must be a non-empty string", 2)
    end
    return Crabe.native("UI_CanContinuePlayset", "Game.CanContinuePlayset")(playset) == true
end

-- Resumes the Play Set's saved game at the place it was saved, skipping the character screen.
-- The engine picks that zone itself, whatever zone is given; unlike Game.LoadLevel it keeps the progress.
function Game.ContinuePlayset(playset, zone)
    if type(playset) ~= "string" or playset == "" then
        error("Game.ContinuePlayset: playset must be a non-empty string", 2)
    end

    releaseFreeCam()
    Crabe.native("UI_ContinuePlayset", "Game.ContinuePlayset")(playset, zone or "", true)
    return playset
end

-- Launches front-end main menu screens.
function Game.LoadMainMenu()
    Crabe.native("UI_LaunchMainMenu", "Game.LoadMainMenu")()
end

-- Autosaves and exits current game back to main menu.
function Game.QuitToMainMenu()
    releaseFreeCam()
    Crabe.native("Pause_ExitGame", "Game.QuitToMainMenu")()
end

-- Launches default level.
function Game.LoadDefaultLevel()
    releaseFreeCam()
    Crabe.native("UI_LaunchDefaultLevel", "Game.LoadDefaultLevel")()
end

-- Transitions player back to the hub world.
function Game.ReturnToHub(playerId)
    releaseFreeCam()
    Crabe.native("UI_ReturnToHub", "Game.ReturnToHub")(Crabe.hostPlayer(playerId))
end

-- Resets the current Toy Box.
function Game.ResetToyBox()
    Crabe.native("UI_ResetToyBox", "Game.ResetToyBox")()
end

-- Resets the current Play Set.
function Game.ResetPlayset()
    Crabe.native("UI_ResetPlayset", "Game.ResetPlayset")()
end

-- Forces game restart and resets saved progression.
function Game.ForceStartOver()
    if type(Storage) ~= "table" or type(Storage.ForceStartOver) ~= "function" then
        error("Game.ForceStartOver: Storage.ForceStartOver is not available in this Lua state", 2)
    end
    Storage.ForceStartOver()
end
