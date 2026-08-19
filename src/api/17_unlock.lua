-- Content unlocking and the Power Disc currencies.
--
-- Not to be confused with the online session lock (UI_LockGame /
-- UI_UnlockGame), which lives in 22_system.lua. Nothing here touches it.
--
-- Two unrelated lock systems exist and both are exposed:
--   * the catalog, keyed by recipe-card name (Catalog_UnlockCatalogItem)
--   * a generic named-lock table (Lock_IsLocked / Lock_SetAvailable), whose
--     keys look like "IGP_PLAYSET_<world>" in levelselectmenu.lua:236
--
-- Everything raises on failure; the mod menu already pcalls its handlers.

Game = Game or {}

local function hostPlayer(playerId)
    if playerId then return playerId end
    return Players_GetHostPlayerID()
end

-- Level 3 so the error points at the caller of the Game.* function rather
-- than at this helper.
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
-- Catalog
-- ---------------------------------------------------------------------------

-- Unlocks one Toy Box catalog entry. The name is a recipe-card name, not an
-- inventory name: catalog.lua:649 passes ribbonItem:RecipeCardName().
function Game.UnlockCatalogItem(recipeCardName, playerId)
    requireName(recipeCardName, "Game.UnlockCatalogItem", "recipeCardName")
    native("Catalog_UnlockCatalogItem", "Game.UnlockCatalogItem")(recipeCardName, hostPlayer(playerId))
    return recipeCardName
end

-- Fires the reward flow for a locked item -- the path the game uses when the
-- player earns something, as opposed to unlocking it outright.
function Game.RewardLock(recipeCardName, playerId)
    requireName(recipeCardName, "Game.RewardLock", "recipeCardName")
    native("Catalog_RewardLockByName", "Game.RewardLock")(recipeCardName, hostPlayer(playerId))
    return recipeCardName
end

-- Grants inventory items outright. catalog.lua:657 passes a tonumber()'d
-- amount, so the native wants a number and not a numeric string.
function Game.AwardItem(name, amount, playerId)
    requireName(name, "Game.AwardItem", "name")
    amount = tonumber(amount) or 1
    native("Catalog_AwardInventoryItem", "Game.AwardItem")(name, amount, hostPlayer(playerId))
    return amount
end

-- ---------------------------------------------------------------------------
-- Named locks
-- ---------------------------------------------------------------------------

-- The generic lock table. Known key shapes: "IGP_PLAYSET_<worldFilter>" for
-- Play Sets, and per-zone audio lock names. Others are almost certainly out
-- there; this is a probe as much as a setter.
function Game.IsLocked(lockName, playerId)
    requireName(lockName, "Game.IsLocked", "lockName")
    return native("Lock_IsLocked", "Game.IsLocked")(lockName, hostPlayer(playerId)) == true
end

function Game.SetLockAvailable(lockName, playerId)
    requireName(lockName, "Game.SetLockAvailable", "lockName")
    native("Lock_SetAvailable", "Game.SetLockAvailable")(lockName, hostPlayer(playerId))
    return lockName
end

-- Play Set locks follow one naming rule, confirmed at levelselectmenu.lua:236.
-- Wrapping it keeps the prefix in one place instead of at every call site.
function Game.UnlockPlayset(worldFilter, playerId)
    requireName(worldFilter, "Game.UnlockPlayset", "worldFilter")
    return Game.SetLockAvailable("IGP_PLAYSET_" .. worldFilter, playerId)
end

-- Reloads progression data with everything marked unlocked. The boolean is
-- passed both ways in toystore.lua (true at :112, false at :167), so this is
-- a mode switch rather than a one-shot.
function Game.ForceUnlockData(enabled)
    if type(enabled) ~= "boolean" then
        error("Game.ForceUnlockData: expected a boolean", 2)
    end
    native("UI_ForceRumpuseUnlockLoadData", "Game.ForceUnlockData")(enabled)
    return enabled
end

-- ---------------------------------------------------------------------------
-- Lock state of an inventory item
-- ---------------------------------------------------------------------------

-- Names for Inventory.LockState_*, so a menu can say why something is locked
-- instead of printing a bare number.
local LOCK_STATE_NAMES = {
    [0] = "aucun", [1] = "verrouille", [2] = "achetable (nouveau)",
    [3] = "achetable", [4] = "achete", [5] = "livre",
    [6] = "disponible", [7] = "actif", [8] = "sync requise",
}

-- Returns the numeric state and its label. Inventory.GetItemState is a nested
-- native, so it is reached through the table rather than _G.
function Game.GetItemLockState(itemName, playerId)
    requireName(itemName, "Game.GetItemLockState", "itemName")

    if type(Inventory) ~= "table" or type(Inventory.GetItemState) ~= "function" then
        error("Game.GetItemLockState: Inventory.GetItemState is not available in this Lua state", 2)
    end

    local state = Inventory.GetItemState(itemName, hostPlayer(playerId))
    return state, LOCK_STATE_NAMES[state] or ("inconnu (" .. tostring(state) .. ")")
end

-- ---------------------------------------------------------------------------
-- Bulk unlock
-- ---------------------------------------------------------------------------

-- Unlocking the whole catalog is thousands of native calls; doing them in one
-- tick freezes the game long enough to look like a crash. This spreads the
-- work across ticks and reports progress, so the menu can show a live count.
--
-- names: array of recipe-card names. perTick: how many to unlock per frame.
function Game.UnlockAllAsync(names, perTick, playerId)
    if type(names) ~= "table" then
        error("Game.UnlockAllAsync: expected an array of names", 2)
    end
    if type(Game.onTick) ~= "function" then
        error("Game.UnlockAllAsync: Game.onTick is unavailable (20_hooks.lua did not load)", 2)
    end

    perTick = tonumber(perTick) or 25
    local player = hostPlayer(playerId)
    local unlock = native("Catalog_UnlockCatalogItem", "Game.UnlockAllAsync")

    local progress = { done = 0, total = #names, failed = 0, finished = false }
    local index = 1

    Game.onTick(function()
        if progress.finished then return end

        local budget = perTick
        while budget > 0 and index <= #names do
            -- One bad name must not abort the whole run, and this is a
            -- background job with no caller left to catch a raise.
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

-- ---------------------------------------------------------------------------
-- Power Disc currencies
-- ---------------------------------------------------------------------------
--
-- Round Coins and Hex Coins are not Sparks. They are the physical Power Disc
-- currencies, and their setters take a count followed by that many slot ids
-- (virtualreader.lua:791 and :805) -- the ids identify which discs occupy the
-- slots, so passing arbitrary numbers is not the same as "give me N coins".
-- The getters are safe; the setters are exposed but deliberately thin.

function Game.GetRoundCoins(playerId)
    return native("Players_GetRoundCoin", "Game.GetRoundCoins")(hostPlayer(playerId))
end

function Game.GetHexCoins(playerId)
    return native("Players_GetHexCoin", "Game.GetHexCoins")(hostPlayer(playerId))
end

-- ids is an array of up to 4 slot ids. Mirrors virtualreader.lua:805, which
-- passes #selected followed by the four ids positionally.
function Game.SetRoundCoins(ids, playerId)
    if type(ids) ~= "table" then
        error("Game.SetRoundCoins: expected an array of slot ids", 2)
    end
    native("Players_SetRoundCoins", "Game.SetRoundCoins")(
        hostPlayer(playerId), #ids, ids[1], ids[2], ids[3], ids[4])
    return #ids
end

-- Same shape as SetRoundCoins but three ids, per virtualreader.lua:791.
function Game.SetHexCoins(ids, playerId)
    if type(ids) ~= "table" then
        error("Game.SetHexCoins: expected an array of slot ids", 2)
    end
    native("Players_SetHexCoins", "Game.SetHexCoins")(
        hostPlayer(playerId), #ids, ids[1], ids[2], ids[3])
    return #ids
end

function Game.GetMaxCoins()
    return native("VirtualReaderPC_GetMaxNumCoins", "Game.GetMaxCoins")()
end
