-- Spawning: putting objects, vehicles and enemies into the world.
--
-- Built on top of the placer recipe already proven in 10_game.lua
-- (Game.SpawnItem). This module generalises it: several copies at once, an
-- explicit world location, the live inventory enumeration the ribbon UI uses,
-- and the vehicle summon path -- which is a different native entirely.
--
-- Like 11_avatar.lua, everything raises on failure. The mod menu already runs
-- its handlers under pcall; a second layer here would only hide which native
-- was missing.

Game = Game or {}

local DEFAULT_META = "MCAT_Classic"

-- Where the generated Toy Box catalog may sit, relative to the game's working
-- directory. The loader's own api/ folder is tried first, then the crabe/
-- folder the previous menu shipped it in.
local CATALOG_PATHS = {
    "api/spawn_catalog.lua",
    "crabe/spawn_catalog.lua",
    "spawn_catalog.lua",
}

local function hostPlayer(playerId)
    if playerId then return playerId end
    return Players_GetHostPlayerID()
end

-- Level 3 so the error points at whoever called the Game.* function, not at
-- this helper and not at the Game.* function itself.
local function native(name, caller)
    local fn = _G[name]
    if type(fn) ~= "function" then
        error(caller .. ": " .. name .. " is not available in this Lua state", 3)
    end
    return fn
end

local function requireName(value, caller, what)
    if type(value) ~= "string" or value == "" then
        error(caller .. ": " .. what .. " must be a non-empty string", 3)
    end
    return value
end

-- ---------------------------------------------------------------------------
-- Placement
-- ---------------------------------------------------------------------------

-- The placer only reacts once the player is in editor object mode: without
-- SetEditorState, CalculateSpawnPosition yields nothing and the ghost is
-- created but never placed. Passing x makes it CreateGhostAtLocation, which
-- catalog.lua:691 uses for a world position the screen already knows.
local function placeGhost(playerId, rrofile, x, y, z)
    Place_SetEditorState(playerId, "Editor::ObjectMode")
    Place_CalculateSpawnPosition(playerId)

    local ghost
    if x then
        ghost = Place_CreateGhostAtLocation(playerId, 0, rrofile, x, y, z)
    else
        ghost = Place_CreateGhost(playerId, 0, rrofile)
    end

    Place_PlaceObject(playerId, ghost)
    return ghost
end

-- StopPlaceMode must run even after a failed placement, or the player stays
-- locked in editor mode with no way out -- that is the one pcall this file
-- allows. Returns how many went in plus the error that stopped it, if any.
local function runPlacement(playerId, count, x, y, z, rrofile)
    local placed, ok, err = 0, true, nil

    for _ = 1, count do
        ok, err = pcall(placeGhost, playerId, rrofile, x, y, z)
        if not ok then break end
        placed = placed + 1
    end

    pcall(Place_StopPlaceMode, playerId, 0, false)
    return placed, (not ok) and err or nil
end

-- Places an already-resolved rrofile, count times. The name-based entry
-- points below all funnel through here.
function Game.SpawnByFile(rrofile, count, playerId)
    requireName(rrofile, "Game.SpawnByFile", "rrofile")
    playerId = hostPlayer(playerId)
    count = count or 1

    native("Place_SetEditorState", "Game.SpawnByFile")
    native("Place_CreateGhost", "Game.SpawnByFile")
    native("Place_PlaceObject", "Game.SpawnByFile")
    native("Place_StopPlaceMode", "Game.SpawnByFile")

    local placed, err = runPlacement(playerId, count, nil, nil, nil, rrofile)
    if err then error("Game.SpawnByFile: " .. tostring(err), 2) end
    return placed
end

-- Spawns an inventory item several times in one editor session, which is
-- cheaper and safer than calling Game.SpawnItem in a loop: the editor is
-- entered and left once.
function Game.SpawnItemMany(invName, count, playerId)
    requireName(invName, "Game.SpawnItemMany", "invName")
    playerId = hostPlayer(playerId)

    local rro = Game.ResolveItemFile(invName, playerId)
    if not rro then
        error("Game.SpawnItemMany: no rrofile for '" .. invName .. "'", 2)
    end
    return Game.SpawnByFile(rro, count or 1, playerId)
end

-- Spawns at an absolute world position. Nothing in the Lua surface reports
-- where the avatar stands, so the caller has to already know the coordinates
-- (read from a save, or from a previous dump) -- otherwise use SpawnItemMany.
function Game.SpawnItemAt(invName, x, y, z, playerId)
    requireName(invName, "Game.SpawnItemAt", "invName")
    if type(x) ~= "number" or type(y) ~= "number" or type(z) ~= "number" then
        error("Game.SpawnItemAt: x, y and z must be numbers", 2)
    end
    playerId = hostPlayer(playerId)

    local rro = Game.ResolveItemFile(invName, playerId)
    if not rro then
        error("Game.SpawnItemAt: no rrofile for '" .. invName .. "'", 2)
    end
    native("Place_CreateGhostAtLocation", "Game.SpawnItemAt")

    local placed, err = runPlacement(playerId, 1, x, y, z, rro)
    if err then error("Game.SpawnItemAt: " .. tostring(err), 2) end
    return placed
end

-- ---------------------------------------------------------------------------
-- Live inventory enumeration
-- ---------------------------------------------------------------------------

Game._invCache = Game._invCache or nil

-- Place_GetInvItems is a C native that fills the two tables it is handed:
-- typeList[category] = { {invItemName=, rroObjectName=, summonTag=, ...}, ... }
-- and catNames as the ordered category list. The three nils are the exclude
-- and filter maps the shipped screens also leave empty (inventorysourcegrid).
local function enumerate(playerId, meta)
    local getInv = native("Place_GetInvItems", "Game.RefreshInventory")
    local typeList, catNames = {}, {}

    getInv(false, "", playerId, nil, nil, nil, typeList, catNames, {}, false, meta)

    local cache = { playerId = playerId, meta = meta,
                    categories = {}, byCategory = {}, byName = {}, count = 0 }

    for category, items in pairs(typeList) do
        if type(items) == "table" then
            local rows = {}
            for _, v in pairs(items) do
                if type(v) == "table" and type(v.invItemName) == "string" then
                    local tag = v.summonTag
                    if tag == "" or tag == "none" then tag = nil end
                    local row = { name = v.invItemName, category = category,
                                  rro = v.rroObjectName, summonTag = tag }
                    rows[#rows + 1] = row
                    cache.byName[row.name] = row
                    cache.count = cache.count + 1
                end
            end
            table.sort(rows, function(a, b) return a.name < b.name end)
            cache.byCategory[category] = rows
            cache.categories[#cache.categories + 1] = category
        end
    end

    table.sort(cache.categories)
    return cache
end

-- Rebuilds the inventory snapshot. Everything below reads the cache, so call
-- this after the player gains items; meta is the ribbon's super-category,
-- "MCAT_Classic" being the one that yields the whole classic Toy Box set.
function Game.RefreshInventory(playerId, meta)
    playerId = hostPlayer(playerId)
    meta = meta or DEFAULT_META

    local cache = enumerate(playerId, meta)
    if cache.count == 0 then
        error("Game.RefreshInventory: Place_GetInvItems returned nothing for meta '"
              .. meta .. "' -- wrong super-category, or no inventory in this world", 2)
    end

    Game._invCache = cache
    return cache.count
end

local function cacheFor(playerId, meta, caller)
    playerId = hostPlayer(playerId)
    meta = meta or DEFAULT_META

    local cache = Game._invCache
    if not cache or cache.playerId ~= playerId or cache.meta ~= meta then
        local ok, err = pcall(Game.RefreshInventory, playerId, meta)
        if not ok then error(caller .. ": " .. tostring(err), 3) end
        cache = Game._invCache
    end
    return cache
end

-- Category names the game itself groups the inventory by (CAT_* keys), sorted.
function Game.GetInventoryCategories(playerId, meta)
    return cacheFor(playerId, meta, "Game.GetInventoryCategories").categories
end

-- The { name, category, rro, summonTag } rows of one category, sorted by name.
function Game.GetInventoryItems(category, playerId, meta)
    requireName(category, "Game.GetInventoryItems", "category")
    local cache = cacheFor(playerId, meta, "Game.GetInventoryItems")

    local rows = cache.byCategory[category]
    if not rows then
        error("Game.GetInventoryItems: unknown category '" .. category .. "'", 2)
    end
    return rows
end

-- One row by exact inventory name, or nil. The row carries the summonTag,
-- which no UI_GetItemDetails field exposes -- only this enumeration does.
function Game.GetInventoryEntry(invName, playerId, meta)
    requireName(invName, "Game.GetInventoryEntry", "invName")
    return cacheFor(playerId, meta, "Game.GetInventoryEntry").byName[invName]
end

-- Case-insensitive substring search over every inventory name. limit caps the
-- result so a menu built from it stays navigable.
function Game.FindInventoryItems(pattern, limit, playerId, meta)
    requireName(pattern, "Game.FindInventoryItems", "pattern")
    local cache = cacheFor(playerId, meta, "Game.FindInventoryItems")
    local needle = string.lower(pattern)
    local out = {}

    for _, category in ipairs(cache.categories) do
        for _, row in ipairs(cache.byCategory[category]) do
            if string.find(string.lower(row.name), needle, 1, true) then
                out[#out + 1] = row
                if limit and #out >= limit then return out end
            end
        end
    end
    return out
end

-- ---------------------------------------------------------------------------
-- Vehicles
-- ---------------------------------------------------------------------------

-- Every inventory row that carries a summonTag, i.e. everything the game
-- considers summonable rather than merely placeable. Sorted by name.
function Game.ListVehicles(playerId, meta)
    local cache = cacheFor(playerId, meta, "Game.ListVehicles")
    local out = {}

    for _, category in ipairs(cache.categories) do
        for _, row in ipairs(cache.byCategory[category]) do
            if row.summonTag then out[#out + 1] = row end
        end
    end

    table.sort(out, function(a, b) return a.name < b.name end)
    return out
end

-- Note the argument order: the item name comes first, the player second is
-- NOT the case here -- Place_CanSummonVehicle takes no player at all
-- (ribbontoybox.lua:416). Defaults the tag to the one the inventory reports.
function Game.CanSummonVehicle(invName, summonTag, playerId, meta)
    requireName(invName, "Game.CanSummonVehicle", "invName")

    if not summonTag then
        local row = Game.GetInventoryEntry(invName, playerId, meta)
        summonTag = row and row.summonTag
    end
    if not summonTag then
        error("Game.CanSummonVehicle: '" .. invName .. "' has no summonTag", 2)
    end
    return native("Place_CanSummonVehicle", "Game.CanSummonVehicle")(invName, summonTag)
end

-- Summons a vehicle the player rides, which is a different engine path from
-- placing its prop: no editor mode, no ghost. The tag comes from the
-- inventory enumeration when the caller does not supply one.
function Game.SummonVehicle(invName, summonTag, playerId, meta)
    requireName(invName, "Game.SummonVehicle", "invName")
    playerId = hostPlayer(playerId)

    if not summonTag then
        local row = Game.GetInventoryEntry(invName, playerId, meta)
        summonTag = row and row.summonTag
    end
    if not summonTag then
        error("Game.SummonVehicle: '" .. invName .. "' has no summonTag; "
              .. "it is a placeable prop, use Game.SpawnItemMany", 2)
    end
    return native("Place_SummonVehicle", "Game.SummonVehicle")(playerId, invName, summonTag)
end

-- ---------------------------------------------------------------------------
-- Generated catalog file
-- ---------------------------------------------------------------------------

Game._spawnCatalog = Game._spawnCatalog or nil

-- Reads the pre-generated { {type, { {franchise, {names} } } } } tree from
-- disk instead of hardcoding 4337 names into a menu. io.open + loadstring
-- rather than require, so the loader's own chunk hooks stay out of the way.
function Game.LoadSpawnCatalog(path)
    if Game._spawnCatalog and not path then return Game._spawnCatalog end
    if not (io and io.open) then
        error("Game.LoadSpawnCatalog: the io library is not available", 2)
    end

    local tried = path and { path } or CATALOG_PATHS
    for _, candidate in ipairs(tried) do
        local f = io.open(candidate, "r")
        if f then
            local src = f:read("*a")
            f:close()

            local chunk = (loadstring or load)(src, "@spawn_catalog")
            if not chunk then
                error("Game.LoadSpawnCatalog: '" .. candidate .. "' does not compile", 2)
            end
            local ok, data = pcall(chunk)
            if not ok or type(data) ~= "table" then
                error("Game.LoadSpawnCatalog: '" .. candidate .. "' did not return a table", 2)
            end

            Game._spawnCatalog = data
            return data
        end
    end

    error("Game.LoadSpawnCatalog: none of " .. table.concat(tried, ", ")
          .. " exists next to the game executable", 2)
end

-- ---------------------------------------------------------------------------
-- Editor housekeeping
-- ---------------------------------------------------------------------------

-- Drops the editor's working objects. includeGhosts=true also clears the
-- preview ghost (ribbonbase.lua:92 passes true on exit, modeselect.lua:387
-- passes false mid-session).
function Game.ClearEditorObjects(includeGhosts, playerId)
    playerId = hostPlayer(playerId)
    native("Place_ClearEditorObjects", "Game.ClearEditorObjects")(playerId, includeGhosts and true or false)
end

-- Takes one argument only (ribbontoybox.lua:1185), unlike ClearEditorObjects.
function Game.RemoveEditorObjects(playerId)
    playerId = hostPlayer(playerId)
    native("Place_RemoveEditorObjects", "Game.RemoveEditorObjects")(playerId)
end

-- objectHandle is a placed/ghost handle as returned by Place_CreateGhost or
-- Place_SelectObject -- there is no native that finds one by name, so this is
-- only usable on a handle the caller already holds.
function Game.DeletePlacedObject(objectHandle, playerId)
    if objectHandle == nil then
        error("Game.DeletePlacedObject: objectHandle is required", 2)
    end
    playerId = hostPlayer(playerId)
    native("Place_DeletePickedObject", "Game.DeletePlacedObject")(playerId, objectHandle)
end

-- Favourites take the item name first and the player second, the reverse of
-- every other Place_ native (ribbonbase.lua:428).
function Game.IsFavorite(invName, playerId)
    requireName(invName, "Game.IsFavorite", "invName")
    return native("Place_IsFavorite", "Game.IsFavorite")(invName, hostPlayer(playerId))
end

function Game.SetFavorite(invName, on, playerId)
    requireName(invName, "Game.SetFavorite", "invName")
    playerId = hostPlayer(playerId)

    local fnName = on and "Place_AddFavorite" or "Place_RemoveFavorite"
    native(fnName, "Game.SetFavorite")(invName, playerId)
    return on and true or false
end

-- Asks the object-budget manager whether one more of this item fits. Takes
-- the rrofile, not the inventory name (ribbontoybox.lua:357).
function Game.CanPlaceItem(invName, playerId)
    requireName(invName, "Game.CanPlaceItem", "invName")
    playerId = hostPlayer(playerId)

    local rro = Game.ResolveItemFile(invName, playerId)
    if not rro then
        error("Game.CanPlaceItem: no rrofile for '" .. invName .. "'", 2)
    end
    return native("Place_LimitManagerCanPlace", "Game.CanPlaceItem")(playerId, rro)
end

-- The budget cost the same item counts for, same rrofile argument.
function Game.GetPlacementValue(invName, playerId)
    requireName(invName, "Game.GetPlacementValue", "invName")
    playerId = hostPlayer(playerId)

    local rro = Game.ResolveItemFile(invName, playerId)
    if not rro then
        error("Game.GetPlacementValue: no rrofile for '" .. invName .. "'", 2)
    end
    return native("Place_LimitManagerTotalValue", "Game.GetPlacementValue")(playerId, rro)
end

function Game.GetPlacedObjectCount()
    return native("VirtualReader_GetPlacedObjectCount", "Game.GetPlacedObjectCount")()
end

-- ---------------------------------------------------------------------------
-- Enemies and sidekicks
-- ---------------------------------------------------------------------------
--
-- These are the "glue" tier. The engine drives them from a screen that was
-- opened by a toy already standing in the world (a Wave Generator, a Sidekick
-- Creator), and the handle of that toy arrives in the screen's DataTable from
-- C++. No native hands a toy handle to Lua, so the two functions that need one
-- are pass-throughs: they are only callable when the caller has obtained a
-- handle some other way. Free enemy spawning goes through Game.SpawnItemMany
-- on the INV_NPC_* names instead -- those are ordinary placeable inventory
-- items.

-- Opens the game's own enemy chooser for the player (lootgridenemies.lua:44).
-- It only lands somewhere if a wave/loot toy selection is already in progress.
function Game.OpenEnemyPicker(playerId)
    playerId = hostPlayer(playerId)
    native("UI_DisplayEnemyList", "Game.OpenEnemyPicker")(playerId)
end

-- Adds an enemy to the wave list currently being edited. One argument, the
-- raw enemy name without the "@"/"_name" localisation wrapping
-- (lootlist.lua:55). It affects the pending selection, it does not spawn.
function Game.AddEnemyToWaveList(enemyName)
    requireName(enemyName, "Game.AddEnemyToWaveList", "enemyName")
    native("UI_AddEnemyToList", "Game.AddEnemyToWaveList")(enemyName)
end

-- listType is "Loot" in the only call site (lootgridenemies.lua:55); "Chest"
-- is the other value the sibling UI_AddToyToLoot uses.
function Game.RemoveEnemyFromLoot(enemyName, listType)
    requireName(enemyName, "Game.RemoveEnemyFromLoot", "enemyName")
    native("UI_RemoveEnemyFromLoot", "Game.RemoveEnemyFromLoot")(enemyName, listType or "Loot")
end

-- toyHandle must come from a Sidekick Creator screen's DataTable.ToyHandle
-- (sidekickselector.lua:152); nothing in the Lua surface produces one.
function Game.SpawnSideKick(toyHandle, id, playerId)
    if toyHandle == nil or id == nil then
        error("Game.SpawnSideKick: toyHandle and id are both required", 2)
    end
    playerId = hostPlayer(playerId)
    native("UI_SpawnSideKick", "Game.SpawnSideKick")(playerId, toyHandle, id)
end

-- Same toyHandle problem as SpawnSideKick (logiccategories.lua:316): opens the
-- wave-enemy picker bound to one placed Wave Generator toy.
function Game.TriggerWaveEnemySelection(toyHandle, playerId)
    if toyHandle == nil then
        error("Game.TriggerWaveEnemySelection: toyHandle is required", 2)
    end
    playerId = hostPlayer(playerId)
    native("UI_TriggerWaveEnemySelection", "Game.TriggerWaveEnemySelection")(playerId, toyHandle)
end
