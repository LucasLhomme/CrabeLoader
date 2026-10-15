-- CrabeLoader
-- File description:
-- The core Game surface: items, inventory, sparks, the host player and on-screen messages.
-- Every function here is a thin wrapper over an engine native; see docs/nativedb.md for the raws.
-- Spawns nothing into the world -- that is src/api/13_spawn.lua.
--
-- Authors: @LucasLhomme

Game = Game or {}
Game._itemRegistry = Game._itemRegistry or {}

-- Registers a custom item handler under the specified identifier.
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

-- Returns the current Sparks count for the active player.
function Game.GetSparks()
    return UI_GetSparks()
end

-- Returns the host player identifier.
function Game.GetHostPlayerID()
    return Players_GetHostPlayerID()
end

-- Checks whether the specified character is currently marked dead.
function Game.IsCharacterDead(playerId, characterIndex)
    return Players_IsCharacterDead(playerId, characterIndex or 0)
end

-- Displays an in-game text message bubble on screen.
function Game.ShowMessage(text, body)
    UI_DisplayTextBox(tostring(text), body or "", false, 0,
                    Players_GetHostPlayerID(), "HelpBubble", 3)
end

-- Enters object mode, calculates placement position and spawns an object.
local function spawnOnce(player, rrofile)
    Place_SetEditorState(player, "Editor::ObjectMode")
    Place_CalculateSpawnPosition(player)
    Place_PlaceObject(player, Place_CreateGhost(player, 0, rrofile))
end

-- Adds or spawns the specified item in the player inventory.
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

        pcall(Place_StopPlaceMode, player, 0, false)

        if not ok then error(err, 2) end
        return amount
    end

    error("Game.AddToInventory: item '" .. tostring(id) .. "' has an unknown kind", 2)
end

-- Deducts currency amount from the player inventory.
function Game.RemoveFromInventory(id, amount)
    local entry = Game._itemRegistry[id]
    if not entry then
        error("Game.RemoveFromInventory: unknown item id '" .. tostring(id) .. "'", 2)
    end
    if entry.kind ~= "currency" then
        error("Game.RemoveFromInventory: '" .. tostring(id) .. "' is not currency", 2)
    end

    amount = amount or 1
    entry.apply(-amount)
    return amount
end

-- Resolves an inventory item name to its placement rrofile path.
function Game.ResolveItemFile(invName, playerId)
    playerId = playerId or Players_GetHostPlayerID()

    if type(UI_GetItemDetailsForPlayer) == "function" then
        local rro = UI_GetItemDetailsForPlayer("Inventory", playerId, invName, "rrofile")
        if type(rro) == "string" and #rro > 0 then return rro end
    end
    if type(UI_GetItemDetails) == "function" then
        local rro = UI_GetItemDetails("Inventory", invName, "rrofile")
        if type(rro) == "string" and #rro > 0 then return rro end
    end
    return nil
end

-- Resolves and spawns an inventory item by its catalog name.
function Game.SpawnItem(invName, playerId)
    local rro = Game.ResolveItemFile(invName, playerId)
    if not rro then
        error("Game.SpawnItem: no rrofile for '" .. tostring(invName) .. "'", 2)
    end

    local id = "spawn." .. invName
    Game.registerItem(id, { kind = "spawn", rrofile = rro })
    return Game.AddToInventory(id, 1)
end

-- Enumerates inventory items with metadata using a placed toy handle.
function Game.ListInventory(playerId, toyHandle)
    playerId = playerId or Players_GetHostPlayerID()

    if type(toyHandle) ~= "number" or toyHandle == 0 then
        error("Game.ListInventory: a live toyHandle is required; passing 0 crashes the game", 2)
    end

    local raw = UI_ListInventoryToys(playerId, toyHandle)
    if type(raw) ~= "string" then
        error("Game.ListInventory: expected a string, got " .. type(raw), 2)
    end

    local rows = {}
    for name in string.gmatch(raw, "[^,]+") do
        local details = ""
        if type(UI_GetItemDetailsForPlayer) == "function" then
            local value = UI_GetItemDetailsForPlayer("Inventory", playerId, name, "type,LevelFilter")
            if type(value) == "string" then details = value end
        end
        rows[#rows + 1] = { name = name, details = details }
    end
    return rows
end

-- Unlocks a catalog recipe card for the specified player.
function Game.UnlockItem(recipeCardName, playerId)
    if type(recipeCardName) ~= "string" or recipeCardName == "" then
        error("Game.UnlockItem: recipeCardName must be a non-empty string", 2)
    end
    Catalog_UnlockCatalogItem(recipeCardName, playerId or Players_GetHostPlayerID())
    return recipeCardName
end

-- Queries inventory toys using an existing placed toy handle.
function Game.ListInventoryToys(playerId, toyHandle)
    playerId = playerId or Players_GetHostPlayerID()

    if type(toyHandle) ~= "number" or toyHandle == 0 then
        error("Game.ListInventoryToys: a live toyHandle is required; passing 0 crashes the game", 2)
    end
    return UI_ListInventoryToys(playerId, toyHandle)
end

-- Enumerates inventory item names belonging to the given category name.
function Game.ListInventoryByCategory(categoryName)
    if type(categoryName) ~= "string" or categoryName == "" then
        error("Game.ListInventoryByCategory: categoryName must be a non-empty string", 2)
    end
    if type(UI_GetList) ~= "function" then
        error("Game.ListInventoryByCategory: UI_GetList is not available in this Lua state", 2)
    end

    local raw = UI_GetList("Inventory", categoryName)
    local names = {}
    if type(raw) == "string" then
        for name in string.gmatch(raw, "[^,]+") do names[#names + 1] = name end
    end
    return names
end

-- Requests the game to exit to system.
function Game.QuitGame()
    return pcall(UI_QuitGame)
end

-- Transitions the player back to the main hub.
function Game.ReturnToHub()
    return pcall(UI_ReturnToHub)
end

-- Binds an engine native function to a Game namespace wrapper.
local function defineNative(name, nativeName, shape)
    Game[name] = function(a, b)
        local native = _G[nativeName]
        local ok, result

        if shape == "player" then
            ok, result = pcall(native, a or Players_GetHostPlayerID())
        elseif shape == "item" then
            ok, result = pcall(native, b or Players_GetHostPlayerID(), a)
        elseif shape == "arg" then
            ok, result = pcall(native, a)
        else
            ok, result = pcall(native)
        end

        if not ok then error("Game." .. name .. ": " .. tostring(result), 2) end
        return result
    end
end

for _, entry in ipairs({
    { "AwardItem",            "Catalog_AwardInventoryItem", "item"   },
    { "GetItemInfo",          "Catalog_GetItemInfo",        "arg"    },
    { "GetCatalogList",       "Catalog_GetList",            "none"   },
    { "BuyItem",              "Catalog_BuyItem",            "item"   },

    { "GetCurrentWorldName",  "UI_CurrentWorldName",        "none"   },
    { "GetGamerTag",          "UI_GetGamerTag",             "player" },
    { "GetPlayerZoneName",    "UI_GetPlayerZoneName",       "player" },
    { "GetInventoryToyCount", "UI_GetInventoryToyCount",    "player" },
    { "GetEarnedStarCount",   "UI_GetEarnedStarCount",      "player" },
    { "GetTotalStarCount",    "UI_GetTotalStarCount",       "player" },
    { "IsHost",               "UI_IsHost",                  "none"   },
    { "GetCurrentLanguage",   "UI_GetCurrentLanguage",      "none"   },
    { "PlayAudio",            "UI_PlayAudio",               "arg"    },
    { "KillAudio",            "UI_KillAudio",               "arg"    },

    { "GetRoundCoins",        "Players_GetRoundCoin",       "player" },
    { "SetRoundCoins",        "Players_SetRoundCoins",      "item"   },
    { "GetAvatarLevel",       "Players_GetAvatarLevel",     "player" },
    { "NumLocalPlayers",      "Players_NumLocalPlayers",    "none"   },
    { "NumPlayers",           "Players_NumPlayers",         "none"   },
    { "IsPlayerValid",        "Players_IsValid",            "player" },
}) do
    defineNative(entry[1], entry[2], entry[3])
end
