-- CrabeLoader
-- File description:
-- Worlds and zones: reading where the player is, listing levels, and moving them elsewhere.
-- A level load tears down the Lua-visible world, so anything held across one must be re-resolved.
-- Spawns nothing on arrival -- that is src/api/13_spawn.lua.
--
-- Authors: @LucasLhomme

-- Worlds: reading where the player is, and moving them somewhere else.
--
-- Level names and world files nearly line up -- "RumpusRoom_TBI_Home" is
-- assets/worlds/rumpusroom_tbi_home.zip -- but not always: the daily challenge
-- level "TBX_Skirmish_DailyChallenge" has no file of that name. So the folder
-- is a discovery aid, never the authority. UI_GetListPlayerIndx("Levels", ...)
-- is what the game itself asks, and it is what LoadLevel validates against.
--
-- No Lua native reads or writes a world position. Placing an actor goes through
-- Crabe._actorPlace, which calls the engine setter the script VM's own
-- KinematicStatePlaceWithPosition uses -- a raw memory write never sticks.

Game = Game or {}

-- Most list natives answer with one comma-joined string plus a count. Splitting
-- here keeps every caller from re-implementing it, and drops the empty trailing
-- field a trailing comma would otherwise produce.
function Crabe.splitList(csv)
    local out = {}
    if type(csv) ~= "string" then return out end

    for field in string.gmatch(csv, "([^,]+)") do
        local trimmed = string.gsub(field, "^%s*(.-)%s*$", "%1")
        if trimmed ~= "" then out[#out + 1] = trimmed end
    end
    return out
end

-- ---------------------------------------------------------------------------
-- Placing an actor
-- ---------------------------------------------------------------------------

-- Moves the actor behind `handle` (Game.GetAvatarHandle for a player) to
-- x, y, z, y being height. Keeps its facing. True when the engine placed it;
-- false when the handle names nothing that can be placed.
function Game.PlaceActor(handle, x, y, z)
    if type(Crabe._actorPlace) ~= "function" then
        error("Game.PlaceActor: this loader has no actor placement native", 2)
    end
    if type(handle) ~= "number" or type(x) ~= "number" or type(y) ~= "number" or type(z) ~= "number" then
        error("Game.PlaceActor: expected handle, x, y, z as numbers", 2)
    end
    return Crabe._actorPlace(handle, x, y, z) == true
end

-- x, y, z of the actor behind `handle`, y being height, or nil when it names
-- no kinematic actor. Read the way the script VM's KinematicStateGetActualPosition does.
function Game.ActorPosition(handle)
    if type(Crabe._actorPosition) ~= "function" then
        error("Game.ActorPosition: this loader has no actor position native", 2)
    end
    if type(handle) ~= "number" then
        error("Game.ActorPosition: expected an actor handle (number)", 2)
    end
    return Crabe._actorPosition(handle)
end

-- ---------------------------------------------------------------------------
-- Creating an actor
-- ---------------------------------------------------------------------------

-- Creates an actor from an engine parameter string at x, y, z facing `heading`
-- radians, and returns its handle (nil when the engine built nothing). The
-- string is an actor list entry's Parms, e.g. "DNAFile=characters/X.dnax".
--
-- This is CActorCreator::CreateActor, what the script VM's CreateFromInitString
-- calls. Unlike Game.SpawnItemMany it does not go through the Toy Box editor,
-- so it also works in the playsets -- where the Rumpus placer refuses to build
-- anything. The actor's assets load on the spot, which can hitch the game.
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

-- Sets (on, the default) or clears a named ActorState bit on an actor. Combat
-- teams are such bits -- "StartTeamNeutral", "CombatTeam1".."CombatTeam4", as
-- the Toy Box's Team property lists them -- and an actor only fights actors of
-- another team. True when the engine applied it.
function Game.SetActorState(handle, state, on)
    if type(Crabe._actorSetState) ~= "function" then
        error("Game.SetActorState: this loader has no actor state native", 2)
    end
    if type(handle) ~= "number" or type(state) ~= "string" then
        error("Game.SetActorState: expected an actor handle and a state name", 2)
    end
    return Crabe._actorSetState(handle, state, on ~= false) == true
end

-- Whether an actor carries a named ActorState bit; nil when it cannot be read.
function Game.ActorHasState(handle, state)
    if type(Crabe._actorTestState) ~= "function" then
        error("Game.ActorHasState: this loader has no actor state native", 2)
    end
    if type(handle) ~= "number" or type(state) ~= "string" then
        error("Game.ActorHasState: expected an actor handle and a state name", 2)
    end
    return Crabe._actorTestState(handle, state)
end

-- Creates `count` actors (1 by default) `distance` units (4 by default) in front
-- of a player, spread side by side and facing them. "In front" is away from
-- that player's camera, which is the direction they are looking. Returns how
-- many the engine built; raises when the player or the camera cannot be read.
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

    -- Horizontal camera-to-player direction; straight down the camera falls back to +z.
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

-- ---------------------------------------------------------------------------
-- Where am I
-- ---------------------------------------------------------------------------

function Game.CurrentWorld()
    return Crabe.native("UI_CurrentWorldName", "Game.CurrentWorld")()
end

-- Replaces the sky and its lighting with a realm from realms/ (realmlist.lua),
-- e.g. "tbx_ala_skydome". The engine does this through a Script VM native no
-- shipped script calls, so the loader calls it (src/infrastructure/engine_sky.cpp).
-- Call it once the world is up, not during the load. Returns true when loaded.
function Game.LoadSkyDome(realmName)
    if type(realmName) ~= "string" or realmName == "" then
        error("Game.LoadSkyDome: realmName must be a non-empty string", 2)
    end
    if type(Crabe._loadSkyDome) ~= "function" then
        error("Game.LoadSkyDome: this loader has no sky native", 2)
    end
    return Crabe._loadSkyDome(realmName) == true
end

function Game.CurrentZone(playerId)
    return Crabe.native("UI_GetPlayerZoneName", "Game.CurrentZone")(Crabe.hostPlayer(playerId))
end

-- Zone metadata. The zone argument accepts the literal "<current>", which is
-- how catalog.lua:1226 reads properties of wherever the player happens to be.
function Game.ZoneString(key, zone)
    if type(key) ~= "string" or key == "" then
        error("Game.ZoneString: key must be a non-empty string", 2)
    end
    return Crabe.native("UI_GetZoneMgrString", "Game.ZoneString")(zone or "<current>", key)
end

function Game.ZoneBool(key, zone)
    if type(key) ~= "string" or key == "" then
        error("Game.ZoneBool: key must be a non-empty string", 2)
    end
    return Crabe.native("UI_GetZoneMgrBool", "Game.ZoneBool")(zone or "<current>", key)
end

function Game.ZoneInt(key, zone)
    if type(key) ~= "string" or key == "" then
        error("Game.ZoneInt: key must be a non-empty string", 2)
    end
    return Crabe.native("UI_GetZoneMgrInt", "Game.ZoneInt")(zone or "<current>", key)
end

-- Rolls the world-shape predicates into one table. Each is optional: a build
-- missing one simply leaves that key absent rather than failing the lot.
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

-- ---------------------------------------------------------------------------
-- Destinations
-- ---------------------------------------------------------------------------

-- The live list the level-select screen is built from. levelselectmenu.lua
-- calls it with "sortedList" (:193) and "filters" (:194) as well as a world
-- filter (:173), so the filter is a mode selector, not just a search string.
function Game.ListLevels(filter, playerId)
    local csv, count = Crabe.native("UI_GetListPlayerIndx", "Game.ListLevels")(
        "Levels", filter or "", Crabe.hostPlayer(playerId))
    return Crabe.splitList(csv), count
end

function Game.CanLoadLevel(levelName)
    if type(levelName) ~= "string" or levelName == "" then
        error("Game.CanLoadLevel: levelName must be a non-empty string", 2)
    end
    return Crabe.native("UI_CanTransitionToLevel", "Game.CanLoadLevel")(levelName) == true
end

-- The engine free camera takes the player's controls and outlives the world
-- it flies in, so the next world's menus would get no input. Every way out of
-- a world below hands it back first. Crabe.Camera loads after this module.
local function releaseFreeCam()
    local Camera = Crabe.Camera
    if Camera and type(Camera.IsFreeCamActive) == "function" and Camera.IsFreeCamActive() then
        Camera.StopFreeCam()
    end
end

-- The five-argument form is what levelselectmenu.lua:70 and :111 use. The
-- trailing booleans are unexplained by any call site -- they are passed
-- identically everywhere except the daily-challenge call (:519), which passes
-- the name alone. Defaults here mirror the common case.
--
-- Refuses to load what CanLoadLevel rejects: a bad name can strand the session
-- with no way back to a menu.
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

-- Adds the main menu screens to the front end. Only meaningful once the front
-- end is up: from inside a world it changes nothing. To leave a world, use
-- Game.QuitToMainMenu.
function Game.LoadMainMenu()
    Crabe.native("UI_LaunchMainMenu", "Game.LoadMainMenu")()
end

-- The pause menu's Quit without its popup (pausemenu.lua PauseExit): the game
-- autosaves, then returns to the main menu by itself.
function Game.QuitToMainMenu()
    releaseFreeCam()
    Crabe.native("Pause_ExitGame", "Game.QuitToMainMenu")()
end

function Game.LoadDefaultLevel()
    releaseFreeCam()
    Crabe.native("UI_LaunchDefaultLevel", "Game.LoadDefaultLevel")()
end

-- ---------------------------------------------------------------------------
-- Returning and resetting
-- ---------------------------------------------------------------------------

-- UI_ReturnToHub takes the player number, as pausemenu.lua passes it.
function Game.ReturnToHub(playerId)
    releaseFreeCam()
    Crabe.native("UI_ReturnToHub", "Game.ReturnToHub")(Crabe.hostPlayer(playerId))
end

-- Destructive. Kept separate from ReturnToHub so no menu can wire them to
-- neighbouring entries by accident.
function Game.ResetToyBox()
    Crabe.native("UI_ResetToyBox", "Game.ResetToyBox")()
end

function Game.ResetPlayset()
    Crabe.native("UI_ResetPlayset", "Game.ResetPlayset")()
end

-- Storage.ForceStartOver is the hardest reset reachable from Lua -- it is what
-- levelselectmenu.lua:413 calls behind the "start over" confirmation, and the
-- exact extent of what it erases has not been verified. Treat as save-wiping.
function Game.ForceStartOver()
    if type(Storage) ~= "table" or type(Storage.ForceStartOver) ~= "function" then
        error("Game.ForceStartOver: Storage.ForceStartOver is not available in this Lua state", 2)
    end
    Storage.ForceStartOver()
end
