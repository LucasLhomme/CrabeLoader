-- CrabeLoader
-- File description:
-- Spawning: putting objects, vehicles and enemies into the world, by file or by inventory entry.
-- A spawn needs the item in the inventory catalogue first, so the listing calls live here too.
-- Unlocks nothing; making content available is src/api/17_unlock.lua.
--
-- Authors: @LucasLhomme

Game = Game or {}

local DEFAULT_META = "MCAT_Classic"

local CATALOG_PATHS = {
    "api/spawn_catalog.lua",
    "crabe/spawn_catalog.lua",
    "spawn_catalog.lua",
}

local function requireName(value, caller, what)
    if type(value) ~= "string" or value == "" then
        error(caller .. ": " .. what .. " must be a non-empty string", 3)
    end
    return value
end

-- Helper to place an object ghost in editor object mode.
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

-- Helper to run multiple placement attempts and exit place mode safely.
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

-- Places an already-resolved RRO file count times.
function Game.SpawnByFile(rrofile, count, playerId)
    requireName(rrofile, "Game.SpawnByFile", "rrofile")
    playerId = Crabe.hostPlayer(playerId)
    count = count or 1

    Crabe.native("Place_SetEditorState", "Game.SpawnByFile")
    Crabe.native("Place_CreateGhost", "Game.SpawnByFile")
    Crabe.native("Place_PlaceObject", "Game.SpawnByFile")
    Crabe.native("Place_StopPlaceMode", "Game.SpawnByFile")

    local placed, err = runPlacement(playerId, count, nil, nil, nil, rrofile)
    if err then error("Game.SpawnByFile: " .. tostring(err), 2) end
    return placed
end

-- Spawns an inventory item multiple times in one editor session.
function Game.SpawnItemMany(invName, count, playerId)
    requireName(invName, "Game.SpawnItemMany", "invName")
    playerId = Crabe.hostPlayer(playerId)

    local rro = Game.ResolveItemFile(invName, playerId)
    if not rro then
        error("Game.SpawnItemMany: no rrofile for '" .. invName .. "'", 2)
    end
    return Game.SpawnByFile(rro, count or 1, playerId)
end

-- Spawns an inventory item at an absolute world position.
function Game.SpawnItemAt(invName, x, y, z, playerId)
    requireName(invName, "Game.SpawnItemAt", "invName")
    if type(x) ~= "number" or type(y) ~= "number" or type(z) ~= "number" then
        error("Game.SpawnItemAt: x, y and z must be numbers", 2)
    end
    playerId = Crabe.hostPlayer(playerId)

    local rro = Game.ResolveItemFile(invName, playerId)
    if not rro then
        error("Game.SpawnItemAt: no rrofile for '" .. invName .. "'", 2)
    end
    Crabe.native("Place_CreateGhostAtLocation", "Game.SpawnItemAt")

    local placed, err = runPlacement(playerId, 1, x, y, z, rro)
    if err then error("Game.SpawnItemAt: " .. tostring(err), 2) end
    return placed
end

Game._invCache = Game._invCache or nil

-- Queries engine inventory items into categorized cache table.
local function enumerate(playerId, meta)
    local getInv = Crabe.native("Place_GetInvItems", "Game.RefreshInventory")
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

-- Refreshes cached inventory items for specified player and meta category.
function Game.RefreshInventory(playerId, meta)
    playerId = Crabe.hostPlayer(playerId)
    meta = meta or DEFAULT_META

    local cache = enumerate(playerId, meta)
    if cache.count == 0 then
        error("Game.RefreshInventory: Place_GetInvItems returned nothing for meta '"
              .. meta .. "' -- wrong super-category, or no inventory in this world", 2)
    end

    Game._invCache = cache
    return cache.count
end

-- Helper returning active inventory cache, refreshing if needed.
local function cacheFor(playerId, meta, caller)
    playerId = Crabe.hostPlayer(playerId)
    meta = meta or DEFAULT_META

    local cache = Game._invCache
    if not cache or cache.playerId ~= playerId or cache.meta ~= meta then
        local ok, err = pcall(Game.RefreshInventory, playerId, meta)
        if not ok then error(caller .. ": " .. tostring(err), 3) end
        cache = Game._invCache
    end
    return cache
end

-- Returns sorted list of inventory category names.
function Game.GetInventoryCategories(playerId, meta)
    return cacheFor(playerId, meta, "Game.GetInventoryCategories").categories
end

-- Returns inventory item rows for the specified category.
function Game.GetInventoryItems(category, playerId, meta)
    requireName(category, "Game.GetInventoryItems", "category")
    local cache = cacheFor(playerId, meta, "Game.GetInventoryItems")

    local rows = cache.byCategory[category]
    if not rows then
        error("Game.GetInventoryItems: unknown category '" .. category .. "'", 2)
    end
    return rows
end

-- Returns inventory entry row for an exact item name, or nil.
function Game.GetInventoryEntry(invName, playerId, meta)
    requireName(invName, "Game.GetInventoryEntry", "invName")
    return cacheFor(playerId, meta, "Game.GetInventoryEntry").byName[invName]
end

-- Performs case-insensitive substring search over inventory names.
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

-- Returns sorted list of summonable vehicle inventory rows.
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

-- Returns true if the vehicle item can currently be summoned.
function Game.CanSummonVehicle(invName, summonTag, playerId, meta)
    requireName(invName, "Game.CanSummonVehicle", "invName")

    if not summonTag then
        local row = Game.GetInventoryEntry(invName, playerId, meta)
        summonTag = row and row.summonTag
    end
    if not summonTag then
        error("Game.CanSummonVehicle: '" .. invName .. "' has no summonTag", 2)
    end
    return Crabe.native("Place_CanSummonVehicle", "Game.CanSummonVehicle")(invName, summonTag)
end

-- Summons a rideable vehicle using engine summon native.
function Game.SummonVehicle(invName, summonTag, playerId, meta)
    requireName(invName, "Game.SummonVehicle", "invName")
    playerId = Crabe.hostPlayer(playerId)

    if not summonTag then
        local row = Game.GetInventoryEntry(invName, playerId, meta)
        summonTag = row and row.summonTag
    end
    if not summonTag then
        error("Game.SummonVehicle: '" .. invName .. "' has no summonTag; "
              .. "it is a placeable prop, use Game.SpawnItemMany", 2)
    end
    return Crabe.native("Place_SummonVehicle", "Game.SummonVehicle")(playerId, invName, summonTag)
end

Game._spawnCatalog = Game._spawnCatalog or nil

-- Loads pre-generated spawn catalog tree from disk.
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

-- Clears editor working objects and optionally preview ghosts.
function Game.ClearEditorObjects(includeGhosts, playerId)
    playerId = Crabe.hostPlayer(playerId)
    Crabe.native("Place_ClearEditorObjects", "Game.ClearEditorObjects")(playerId, includeGhosts and true or false)
end

-- Removes editor objects for the player.
function Game.RemoveEditorObjects(playerId)
    playerId = Crabe.hostPlayer(playerId)
    Crabe.native("Place_RemoveEditorObjects", "Game.RemoveEditorObjects")(playerId)
end

-- Deletes a placed object or ghost by handle.
function Game.DeletePlacedObject(objectHandle, playerId)
    if objectHandle == nil then
        error("Game.DeletePlacedObject: objectHandle is required", 2)
    end
    playerId = Crabe.hostPlayer(playerId)
    Crabe.native("Place_DeletePickedObject", "Game.DeletePlacedObject")(playerId, objectHandle)
end

-- Returns true if the inventory item is marked as favorite.
function Game.IsFavorite(invName, playerId)
    requireName(invName, "Game.IsFavorite", "invName")
    return Crabe.native("Place_IsFavorite", "Game.IsFavorite")(invName, Crabe.hostPlayer(playerId))
end

-- Adds or removes an inventory item from favorites.
function Game.SetFavorite(invName, on, playerId)
    requireName(invName, "Game.SetFavorite", "invName")
    playerId = Crabe.hostPlayer(playerId)

    local fnName = on and "Place_AddFavorite" or "Place_RemoveFavorite"
    Crabe.native(fnName, "Game.SetFavorite")(invName, playerId)
    return on and true or false
end

-- Returns true if limit manager allows placing one more item.
function Game.CanPlaceItem(invName, playerId)
    requireName(invName, "Game.CanPlaceItem", "invName")
    playerId = Crabe.hostPlayer(playerId)

    local rro = Game.ResolveItemFile(invName, playerId)
    if not rro then
        error("Game.CanPlaceItem: no rrofile for '" .. invName .. "'", 2)
    end
    return Crabe.native("Place_LimitManagerCanPlace", "Game.CanPlaceItem")(playerId, rro)
end

-- Returns placement budget cost value for the item.
function Game.GetPlacementValue(invName, playerId)
    requireName(invName, "Game.GetPlacementValue", "invName")
    playerId = Crabe.hostPlayer(playerId)

    local rro = Game.ResolveItemFile(invName, playerId)
    if not rro then
        error("Game.GetPlacementValue: no rrofile for '" .. invName .. "'", 2)
    end
    return Crabe.native("Place_LimitManagerTotalValue", "Game.GetPlacementValue")(playerId, rro)
end

-- Returns the total count of placed objects in the world.
function Game.GetPlacedObjectCount()
    return Crabe.native("VirtualReader_GetPlacedObjectCount", "Game.GetPlacedObjectCount")()
end

-- Displays the engine enemy picker interface for the player.
function Game.OpenEnemyPicker(playerId)
    playerId = Crabe.hostPlayer(playerId)
    Crabe.native("UI_DisplayEnemyList", "Game.OpenEnemyPicker")(playerId)
end

-- Adds an enemy to the active wave selection list.
function Game.AddEnemyToWaveList(enemyName)
    requireName(enemyName, "Game.AddEnemyToWaveList", "enemyName")
    Crabe.native("UI_AddEnemyToList", "Game.AddEnemyToWaveList")(enemyName)
end

-- Removes an enemy from active loot or chest selection.
function Game.RemoveEnemyFromLoot(enemyName, listType)
    requireName(enemyName, "Game.RemoveEnemyFromLoot", "enemyName")
    Crabe.native("UI_RemoveEnemyFromLoot", "Game.RemoveEnemyFromLoot")(enemyName, listType or "Loot")
end

-- Spawns a sidekick using creator toy handle.
function Game.SpawnSideKick(toyHandle, id, playerId)
    if toyHandle == nil or id == nil then
        error("Game.SpawnSideKick: toyHandle and id are both required", 2)
    end
    playerId = Crabe.hostPlayer(playerId)
    Crabe.native("UI_SpawnSideKick", "Game.SpawnSideKick")(playerId, toyHandle, id)
end

-- Triggers wave enemy selection on a wave generator toy.
function Game.TriggerWaveEnemySelection(toyHandle, playerId)
    if toyHandle == nil then
        error("Game.TriggerWaveEnemySelection: toyHandle is required", 2)
    end
    playerId = Crabe.hostPlayer(playerId)
    Crabe.native("UI_TriggerWaveEnemySelection", "Game.TriggerWaveEnemySelection")(playerId, toyHandle)
end
