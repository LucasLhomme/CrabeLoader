-- Splitscreen & Multi-Controller API for Disney Infinity 3.0
--
-- Exposes viewports, screen splitting (horizontal, vertical, combined),
-- drop-in management, and controller locking bindings.

Game = Game or {}

-- ---------------------------------------------------------------------------
-- Viewport & Screen Layout
-- ---------------------------------------------------------------------------

function Game.GetViewportCount()
    local fn = _G["GetViewportCount"]
    if type(fn) == "function" then return fn() end
    return 1
end

function Game.GetViewportID(playerId)
    playerId = playerId or 0
    local fn = _G["Display_GetViewportIDFromPlayerID"]
    if type(fn) == "function" then return fn(playerId) end
    return 0
end

function Game.IsHorizontalSplit()
    local fn = _G["UI_IsGamePlayHorizontalSplit"]
    if type(fn) == "function" then return fn() end
    return false
end

function Game.IsVerticalSplit()
    local fn = _G["UI_IsGamePlayVerticalSplit"]
    if type(fn) == "function" then return fn() end
    return false
end

function Game.IsCombinedScreen()
    local fn = _G["UI_IsGamePlayCombinedScreen"]
    if type(fn) == "function" then return fn() end
    return true
end

function Game.SetScreenSplitDirection(direction)
    -- direction: 0 = None / Combined, 1 = Horizontal, 2 = Vertical
    local fn = _G["UI_ScreenSplitDirection"]
    if type(fn) == "function" then
        fn(direction)
        return true
    end
    return false
end

-- ---------------------------------------------------------------------------
-- Drop-In & Controller Management
-- ---------------------------------------------------------------------------

function Game.SetDropInBlocked(blocked)
    local fn = _G["DropInBlocked"]
    if type(fn) == "function" then
        fn(blocked and true or false)
        return true
    end
    return false
end

function Game.GetLocalPlayers()
    local fn = _G["Players_GetLocalPlayers"]
    if type(fn) == "function" then
        local players = fn()
        if type(players) == "table" then return players end
    end
    return { 0 }
end

function Game.NumLocalPlayers()
    local fn = _G["Players_NumLocalPlayers"]
    if type(fn) == "function" then return fn() end
    return 1
end

function Game.LockPlayerController(playerId, controllerIndex)
    local fn = _G["LockPlayerToController"]
    if type(fn) == "function" then
        fn(playerId or 0, controllerIndex or 0)
        return true
    end
    return false
end

function Game.UnlockAllControllers()
    local fn = _G["UnlockAllControllers"]
    if type(fn) == "function" then
        fn()
        return true
    end
    return false
end

function Game.RemoveLocalPlayer(playerId)
    local fn = _G["RemovePlayer"]
    if type(fn) == "function" then
        fn(playerId)
        return true
    end
    return false
end
