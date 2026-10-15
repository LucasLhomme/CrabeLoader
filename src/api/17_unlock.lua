-- CrabeLoader
-- File description:
-- Content unlocking and the Power Disc currencies: locks, awards, playsets and coin balances.
-- Unlocking is asynchronous, so the bulk call reports progress rather than completing at once.
-- Spawns nothing once unlocked; that is src/api/13_spawn.lua.
--
-- Authors: @LucasLhomme

Game = Game or {}

local function requireName(value, caller, what)
    if type(value) ~= "string" or value == "" then
        error(caller .. ": " .. what .. " must be a non-empty string", 3)
    end
    return value
end

-- Unlocks a Toy Box catalog entry by its recipe-card name.
function Game.UnlockCatalogItem(recipeCardName, playerId)
    requireName(recipeCardName, "Game.UnlockCatalogItem", "recipeCardName")
    Crabe.native("Catalog_UnlockCatalogItem", "Game.UnlockCatalogItem")(recipeCardName, Crabe.hostPlayer(playerId))
    return recipeCardName
end

-- Triggers reward flow for an item lock by recipe-card name.
function Game.RewardLock(recipeCardName, playerId)
    requireName(recipeCardName, "Game.RewardLock", "recipeCardName")
    Crabe.native("Catalog_RewardLockByName", "Game.RewardLock")(recipeCardName, Crabe.hostPlayer(playerId))
    return recipeCardName
end

-- Grants specified amount of an inventory item to the player.
function Game.AwardItem(name, amount, playerId)
    requireName(name, "Game.AwardItem", "name")
    amount = tonumber(amount) or 1
    Crabe.native("Catalog_AwardInventoryItem", "Game.AwardItem")(name, amount, Crabe.hostPlayer(playerId))
    return amount
end

-- Checks if a generic named game lock is currently locked.
function Game.IsLocked(lockName, playerId)
    requireName(lockName, "Game.IsLocked", "lockName")
    return Crabe.native("Lock_IsLocked", "Game.IsLocked")(lockName, Crabe.hostPlayer(playerId)) == true
end

-- Marks a generic named game lock as available.
function Game.SetLockAvailable(lockName, playerId)
    requireName(lockName, "Game.SetLockAvailable", "lockName")
    Crabe.native("Lock_SetAvailable", "Game.SetLockAvailable")(lockName, Crabe.hostPlayer(playerId))
    return lockName
end

-- Unlocks Play Set world filter lock for the specified player.
function Game.UnlockPlayset(worldFilter, playerId)
    requireName(worldFilter, "Game.UnlockPlayset", "worldFilter")
    return Game.SetLockAvailable("IGP_PLAYSET_" .. worldFilter, playerId)
end

-- Toggles force unlock progression data mode.
function Game.ForceUnlockData(enabled)
    if type(enabled) ~= "boolean" then
        error("Game.ForceUnlockData: expected a boolean", 2)
    end
    Crabe.native("UI_ForceRumpuseUnlockLoadData", "Game.ForceUnlockData")(enabled)
    return enabled
end

local LOCK_STATE_NAMES = {
    [0] = "aucun", [1] = "verrouille", [2] = "achetable (nouveau)",
    [3] = "achetable", [4] = "achete", [5] = "livre",
    [6] = "disponible", [7] = "actif", [8] = "sync requise",
}

-- Returns numeric lock state and label string for an inventory item.
function Game.GetItemLockState(itemName, playerId)
    requireName(itemName, "Game.GetItemLockState", "itemName")

    if type(Inventory) ~= "table" or type(Inventory.GetItemState) ~= "function" then
        error("Game.GetItemLockState: Inventory.GetItemState is not available in this Lua state", 2)
    end

    local state = Inventory.GetItemState(itemName, Crabe.hostPlayer(playerId))
    return state, LOCK_STATE_NAMES[state] or ("inconnu (" .. tostring(state) .. ")")
end

-- Asynchronously unlocks catalog items in batches across game ticks.
function Game.UnlockAllAsync(names, perTick, playerId)
    if type(names) ~= "table" then
        error("Game.UnlockAllAsync: expected an array of names", 2)
    end
    if type(Game.onTick) ~= "function" then
        error("Game.UnlockAllAsync: Game.onTick is unavailable (20_hooks.lua did not load)", 2)
    end

    perTick = tonumber(perTick) or 25
    local player = Crabe.hostPlayer(playerId)
    local unlock = Crabe.native("Catalog_UnlockCatalogItem", "Game.UnlockAllAsync")

    local progress = { done = 0, total = #names, failed = 0, finished = false }
    local index = 1

    Game.onTick(function()
        if progress.finished then return end

        local budget = perTick
        while budget > 0 and index <= #names do
            if not pcall(unlock, names[index], player) then
                progress.failed = progress.failed + 1
            end
            index = index + 1
            progress.done = progress.done + 1
            budget = budget - 1
        end

        if index > #names then progress.finished = true end
    end)

    return progress
end

-- Returns current count of Power Disc round coins.
function Game.GetRoundCoins(playerId)
    return Crabe.native("Players_GetRoundCoin", "Game.GetRoundCoins")(Crabe.hostPlayer(playerId))
end

-- Returns current count of Power Disc hex coins.
function Game.GetHexCoins(playerId)
    return Crabe.native("Players_GetHexCoin", "Game.GetHexCoins")(Crabe.hostPlayer(playerId))
end

-- Sets Power Disc round coin slot ids for the player.
function Game.SetRoundCoins(ids, playerId)
    if type(ids) ~= "table" then
        error("Game.SetRoundCoins: expected an array of slot ids", 2)
    end
    Crabe.native("Players_SetRoundCoins", "Game.SetRoundCoins")(
        Crabe.hostPlayer(playerId), #ids, ids[1], ids[2], ids[3], ids[4])
    return #ids
end

-- Sets Power Disc hex coin slot ids for the player.
function Game.SetHexCoins(ids, playerId)
    if type(ids) ~= "table" then
        error("Game.SetHexCoins: expected an array of slot ids", 2)
    end
    Crabe.native("Players_SetHexCoins", "Game.SetHexCoins")(
        Crabe.hostPlayer(playerId), #ids, ids[1], ids[2], ids[3])
    return #ids
end

-- Returns maximum allowed Power Disc coins count.
function Game.GetMaxCoins()
    return Crabe.native("VirtualReaderPC_GetMaxNumCoins", "Game.GetMaxCoins")()
end
