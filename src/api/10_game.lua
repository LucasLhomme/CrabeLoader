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

-- Direct native check, not an event -- the engine never tells Lua an entity
-- died. Game.onDeath (20_hooks.lua) builds edge-triggered notification on
-- top of it. characterIndex is an unconfirmed guess, see docs/nativedb.md.
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

-- Unlocks a catalog entry (menus/Toy Box) rather than spawning it. itemId is
-- a numeric catalog id, unconfirmed -- see docs/nativedb.md.
function Game.UnlockItem(itemId, playerId)
    playerId = playerId or Players_GetHostPlayerID()
    local ok, err = pcall(Catalog_UnlockCatalogItem, playerId, itemId)
    if not ok then
        error("Game.UnlockItem: " .. tostring(err), 2)
    end
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

-- These two return the raw pcall pair rather than raising: a caller wants to
-- know the request was refused, not to be thrown out of its own code.
function Game.QuitGame()
    return pcall(UI_QuitGame)
end

function Game.ReturnToHub()
    return pcall(UI_ReturnToHub)
end

-- Everything below is a thin wrapper over one native, and they were all the
-- same four lines: default the player id, pcall, re-raise with the Game.*
-- name in front, return the result. The pcall is not defensive habit -- most
-- of these natives are guesses at an argument shape that has never been
-- confirmed (see docs/nativedb.md "To verify"), so a wrong guess must surface
-- as a named Lua error rather than take the game down.
--
-- Shapes, by how the native wants the player id:
--   "none"    Game.X()           -> native()
--   "arg"     Game.X(a)          -> native(a)
--   "player"  Game.X(playerId)   -> native(playerId)
--   "item"    Game.X(a, playerId) -> native(playerId, a)
--
-- The native is looked up by name at call time, not captured here, so this
-- module stays loadable in a state where the game's natives are not up yet.
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
    -- Grants possession directly, where UnlockItem only makes the entry
    -- selectable in menus. Same numeric-id guess as UnlockItem.
    { "AwardItem",            "Catalog_AwardInventoryItem", "item"   },
    -- Confirmed callable with a string name and no error; returned "" for a
    -- name that is not a real item, so it may take an id like the two above.
    { "GetItemInfo",          "Catalog_GetItemInfo",        "arg"    },
    -- Confirmed callable, returned "" in this save -- likely nothing pending,
    -- or the list depends on a screen a console call does not have open.
    { "GetCatalogList",       "Catalog_GetList",            "none"   },
    -- Same numeric-id guess as UnlockItem/AwardItem.
    { "BuyItem",              "Catalog_BuyItem",            "item"   },

    { "GetCurrentWorldName",  "UI_CurrentWorldName",        "none"   },
    -- Confirmed callable; returned "" here, plausibly because the session has
    -- no platform identity signed in.
    { "GetGamerTag",          "UI_GetGamerTag",             "player" },
    { "GetPlayerZoneName",    "UI_GetPlayerZoneName",       "player" },
    { "GetInventoryToyCount", "UI_GetInventoryToyCount",    "player" },
    { "GetEarnedStarCount",   "UI_GetEarnedStarCount",      "player" },
    { "GetTotalStarCount",    "UI_GetTotalStarCount",       "player" },
    { "IsHost",               "UI_IsHost",                  "none"   },
    { "GetCurrentLanguage",   "UI_GetCurrentLanguage",      "none"   },
    -- Sound feedback for a mod reacting to something. `handle` is a guess at
    -- the argument shape -- it could be a sound bank name instead.
    { "PlayAudio",            "UI_PlayAudio",               "arg"    },
    { "KillAudio",            "UI_KillAudio",               "arg"    },

    -- Round Coins are the physical Power Disc currency, distinct from Sparks
    -- (Game.GetSparks / "Items.money") -- see the false-friend warning in
    -- docs/nativedb.md. Untested.
    { "GetRoundCoins",        "Players_GetRoundCoin",       "player" },
    { "SetRoundCoins",        "Players_SetRoundCoins",      "item"   },
    { "GetAvatarLevel",       "Players_GetAvatarLevel",     "player" },
    { "LevelUpAvatar",        "Players_AvatarLevelUp",      "player" },
    { "NumLocalPlayers",      "Players_NumLocalPlayers",    "none"   },
    { "NumPlayers",           "Players_NumPlayers",         "none"   },
    { "IsPlayerValid",        "Players_IsValid",            "player" },
}) do
    defineNative(entry[1], entry[2], entry[3])
end
