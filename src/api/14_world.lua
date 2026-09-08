-- Worlds: reading where the player is, and moving them somewhere else.
--
-- Level names and world files nearly line up -- "RumpusRoom_TBI_Home" is
-- assets/worlds/rumpusroom_tbi_home.zip -- but not always: the daily challenge
-- level "TBX_Skirmish_DailyChallenge" has no file of that name. So the folder
-- is a discovery aid, never the authority. UI_GetListPlayerIndx("Levels", ...)
-- is what the game itself asks, and it is what LoadLevel validates against.
--
-- There is no native anywhere that reads or writes a world position, so there
-- is no teleport here. Free-coordinate movement lives in 25_cheats.lua.

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
-- Where am I
-- ---------------------------------------------------------------------------

function Game.CurrentWorld()
    return Crabe.native("UI_CurrentWorldName", "Game.CurrentWorld")()
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

    Crabe.native("UI_LaunchLevel", "Game.LoadLevel")(levelName, "world", true, true, false)
    return levelName
end

function Game.LoadMainMenu()
    Crabe.native("UI_LaunchMainMenu", "Game.LoadMainMenu")()
end

function Game.LoadDefaultLevel()
    Crabe.native("UI_LaunchDefaultLevel", "Game.LoadDefaultLevel")()
end

-- ---------------------------------------------------------------------------
-- Returning and resetting
-- ---------------------------------------------------------------------------

function Game.ReturnToHub()
    Crabe.native("UI_ReturnToHub", "Game.ReturnToHub")()
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
