-- Game API. See docs/nativedb.md for the natives it is built on.

Game = Game or {}
Game._itemRegistry = Game._itemRegistry or {}

-- entry = { kind = "currency", apply = function(amount) end }
--       | { kind = "spawn", rrofile = "<INV_ name without the INV_ prefix>" }
function Game.registerItem(id, entry)
    if type(id) ~= "string" or type(entry) ~= "table" then
        error("Game.registerItem: expected (string, table)", 2)
    end
    Game._itemRegistry[id] = entry
end

Game.registerItem("Items.money", {
    kind = "currency",
    apply = function(amount) UI_IncrementSparks(amount) end,
})

function Game.GetSparks()
    return UI_GetSparks()
end

function Game.GetHostPlayerID()
    return Players_GetHostPlayerID()
end

-- Closes the "no death event" gap documented in docs/nativedb.md: the engine
-- never tells Lua an entity died, so this is a direct native check, not an
-- event. Game.onDeath (20_hooks.lua) builds the actual edge-triggered
-- notification on top of it.
--
-- Takes 2 arguments (confirmed in-game: "bad argument #2 ... number expected,
-- got no value" with only one). The decompiled call site (revive.lua) has
-- "playerNum" next to it but no second name; characterIndex is a guess and
-- still needs in-game confirmation -- see docs/nativedb.md.
function Game.IsCharacterDead(playerId, characterIndex)
    return Players_IsCharacterDead(playerId, characterIndex or 0)
end

function Game.ShowMessage(text, body)
    UI_DisplayTextBox(tostring(text), body or "", false, 0,
                    Players_GetHostPlayerID(), "HelpBubble", 3)
end

-- Without SetEditorState, CalculateSpawnPosition yields nothing and the ghost
-- is created but never placed.
local function spawnOnce(player, rrofile)
    Place_SetEditorState(player, "Editor::ObjectMode")
    Place_CalculateSpawnPosition(player)
    Place_PlaceObject(player, Place_CreateGhost(player, 0, rrofile))
end

function Game.AddToInventory(id, amount)
    local entry = Game._itemRegistry[id]
    if not entry then
        error("Game.AddToInventory: unknown item id '" .. tostring(id) .. "'", 2)
    end
    amount = amount or 1

    if entry.kind == "currency" then
        entry.apply(amount)
        return amount
    end

    if entry.kind == "spawn" then
        local player = Players_GetHostPlayerID()
        local ok, err = true, nil

        for _ = 1, amount do
            ok, err = pcall(spawnOnce, player, entry.rrofile)
            if not ok then break end
        end

        -- Must run even after a failure, or the editor keeps the player's
        -- input locked.
        pcall(Place_StopPlaceMode, player, 0, false)

        if not ok then error(err, 2) end
        return amount
    end

    error("Game.AddToInventory: item '" .. tostring(id) .. "' has an unknown kind", 2)
end

-- Unlocks a catalog entry (menus/Toy Box list) rather than spawning it, unlike
-- Game.AddToInventory's "spawn" kind. Confirmed in-game that arg #2 is a
-- NUMBER, not the item's string name as first guessed ("bad argument #2 ...
-- number expected, got string") -- catalog entries are referenced by a
-- numeric id, still unknown. Still needs a real id to confirm the call itself
-- works; the pcall stays until then. See docs/nativedb.md.
function Game.UnlockItem(itemId, playerId)
    playerId = playerId or Players_GetHostPlayerID()
    local ok, err = pcall(Catalog_UnlockCatalogItem, playerId, itemId)
    if not ok then
        error("Game.UnlockItem: " .. tostring(err), 2)
    end
end

-- Everything below is unconfirmed: guessed from the native's name and the
-- (playerID, ...) convention every confirmed native so far follows. Each call
-- goes through pcall for that reason -- see docs/nativedb.md "To verify".

-- Grants possession directly, as opposed to Game.UnlockItem which only makes
-- the entry selectable in menus. Same numeric-id correction as UnlockItem.
function Game.AwardItem(itemId, playerId)
    playerId = playerId or Players_GetHostPlayerID()
    local ok, result = pcall(Catalog_AwardInventoryItem, playerId, itemId)
    if not ok then error("Game.AwardItem: " .. tostring(result), 2) end
    return result
end

-- Confirmed callable with a string name and no error; returned an empty
-- string for "Test" (not a real item), so this may well take an id like the
-- two above once one is known -- not yet confirmed either way.
function Game.GetItemInfo(itemName)
    local ok, result = pcall(Catalog_GetItemInfo, itemName)
    if not ok then error("Game.GetItemInfo: " .. tostring(result), 2) end
    return result
end

-- Confirmed callable with zero arguments and no error; returned an empty
-- string in this save (likely nothing purchasable pending, or the list is
-- context-dependent on a screen this console call doesn't have open).
function Game.GetCatalogList()
    local ok, result = pcall(Catalog_GetList)
    if not ok then error("Game.GetCatalogList: " .. tostring(result), 2) end
    return result
end

function Game.QuitGame()
    return pcall(UI_QuitGame)
end

function Game.ReturnToHub()
    return pcall(UI_ReturnToHub)
end

function Game.GetCurrentWorldName()
    local ok, result = pcall(UI_CurrentWorldName)
    if not ok then error("Game.GetCurrentWorldName: " .. tostring(result), 2) end
    return result
end

-- Confirmed callable and no error; returned an empty string in this session,
-- plausibly because it is not signed into a platform identity offline/in test.
function Game.GetGamerTag(playerId)
    playerId = playerId or Players_GetHostPlayerID()
    local ok, result = pcall(UI_GetGamerTag, playerId)
    if not ok then error("Game.GetGamerTag: " .. tostring(result), 2) end
    return result
end

function Game.GetPlayerZoneName(playerId)
    playerId = playerId or Players_GetHostPlayerID()
    local ok, result = pcall(UI_GetPlayerZoneName, playerId)
    if not ok then error("Game.GetPlayerZoneName: " .. tostring(result), 2) end
    return result
end

-- Confirmed in-game that this takes 2 arguments, not 1 ("bad argument #2 ...
-- number expected, got no value"). What the second number selects (category?
-- page?) is unknown -- 0 is an untested guess.
function Game.ListInventoryToys(playerId, category)
    playerId = playerId or Players_GetHostPlayerID()
    local ok, result = pcall(UI_ListInventoryToys, playerId, category or 0)
    if not ok then error("Game.ListInventoryToys: " .. tostring(result), 2) end
    return result
end

function Game.GetInventoryToyCount(playerId)
    playerId = playerId or Players_GetHostPlayerID()
    local ok, result = pcall(UI_GetInventoryToyCount, playerId)
    if not ok then error("Game.GetInventoryToyCount: " .. tostring(result), 2) end
    return result
end

-- Round Coins are the physical Power Disc currency, distinct from Sparks
-- (Game.GetSparks/AddToInventory's "Items.money") -- see the false-friend
-- warning in docs/nativedb.md. Untested.
function Game.GetRoundCoins(playerId)
    playerId = playerId or Players_GetHostPlayerID()
    local ok, result = pcall(Players_GetRoundCoin, playerId)
    if not ok then error("Game.GetRoundCoins: " .. tostring(result), 2) end
    return result
end

function Game.SetRoundCoins(amount, playerId)
    playerId = playerId or Players_GetHostPlayerID()
    local ok, result = pcall(Players_SetRoundCoins, playerId, amount)
    if not ok then error("Game.SetRoundCoins: " .. tostring(result), 2) end
    return result
end

function Game.GetAvatarLevel(playerId)
    playerId = playerId or Players_GetHostPlayerID()
    local ok, result = pcall(Players_GetAvatarLevel, playerId)
    if not ok then error("Game.GetAvatarLevel: " .. tostring(result), 2) end
    return result
end

function Game.LevelUpAvatar(playerId)
    playerId = playerId or Players_GetHostPlayerID()
    local ok, result = pcall(Players_AvatarLevelUp, playerId)
    if not ok then error("Game.LevelUpAvatar: " .. tostring(result), 2) end
    return result
end

function Game.NumLocalPlayers()
    local ok, result = pcall(Players_NumLocalPlayers)
    if not ok then error("Game.NumLocalPlayers: " .. tostring(result), 2) end
    return result
end

function Game.NumPlayers()
    local ok, result = pcall(Players_NumPlayers)
    if not ok then error("Game.NumPlayers: " .. tostring(result), 2) end
    return result
end

function Game.IsPlayerValid(playerId)
    playerId = playerId or Players_GetHostPlayerID()
    local ok, result = pcall(Players_IsValid, playerId)
    if not ok then error("Game.IsPlayerValid: " .. tostring(result), 2) end
    return result
end

-- Same numeric-id guess as Game.UnlockItem/AwardItem -- Catalog_ entries seem
-- to be referenced by number, not by their string name.
function Game.BuyItem(itemId, playerId)
    playerId = playerId or Players_GetHostPlayerID()
    local ok, result = pcall(Catalog_BuyItem, playerId, itemId)
    if not ok then error("Game.BuyItem: " .. tostring(result), 2) end
    return result
end

function Game.GetEarnedStarCount(playerId)
    playerId = playerId or Players_GetHostPlayerID()
    local ok, result = pcall(UI_GetEarnedStarCount, playerId)
    if not ok then error("Game.GetEarnedStarCount: " .. tostring(result), 2) end
    return result
end

function Game.GetTotalStarCount(playerId)
    playerId = playerId or Players_GetHostPlayerID()
    local ok, result = pcall(UI_GetTotalStarCount, playerId)
    if not ok then error("Game.GetTotalStarCount: " .. tostring(result), 2) end
    return result
end

function Game.IsHost()
    local ok, result = pcall(UI_IsHost)
    if not ok then error("Game.IsHost: " .. tostring(result), 2) end
    return result
end

function Game.GetCurrentLanguage()
    local ok, result = pcall(UI_GetCurrentLanguage)
    if not ok then error("Game.GetCurrentLanguage: " .. tostring(result), 2) end
    return result
end

-- Sound feedback for a mod reacting to something (an item picked up, a spawn
-- succeeding, ...). `handle` is a guess at the argument shape -- could well be
-- a sound bank name string instead of whatever a "handle" is here.
function Game.PlayAudio(handle)
    local ok, result = pcall(UI_PlayAudio, handle)
    if not ok then error("Game.PlayAudio: " .. tostring(result), 2) end
    return result
end

function Game.KillAudio(handle)
    local ok, result = pcall(UI_KillAudio, handle)
    if not ok then error("Game.KillAudio: " .. tostring(result), 2) end
    return result
end
