-- CrabeLoader
-- File description:
-- Splitscreen and controllers: viewport count, split direction, drop-in and player locking.
-- Drop-in blocking is an engine flag, so it takes effect only on the next join attempt.
-- Detects no controller; which pads the game polls is observed in src/presentation/input_hook.cpp.
--
-- Authors: @LucasLhomme

Game = Game or {}

-- Returns active viewport count.
function Game.GetViewportCount()
    local fn = _G["GetViewportCount"]
    if type(fn) == "function" then return fn() end
    return 1
end

-- Returns viewport ID associated with the player ID.
function Game.GetViewportID(playerId)
    playerId = playerId or 0
    local fn = _G["Display_GetViewportIDFromPlayerID"]
    if type(fn) == "function" then return fn(playerId) end
    return 0
end

-- Returns true if gameplay screen is split horizontally.
function Game.IsHorizontalSplit()
    local fn = _G["UI_IsGamePlayHorizontalSplit"]
    if type(fn) == "function" then return fn() end
    return false
end

-- Returns true if gameplay screen is split vertically.
function Game.IsVerticalSplit()
    local fn = _G["UI_IsGamePlayVerticalSplit"]
    if type(fn) == "function" then return fn() end
    return false
end

-- Returns true if gameplay screen is in combined single-screen mode.
function Game.IsCombinedScreen()
    local fn = _G["UI_IsGamePlayCombinedScreen"]
    if type(fn) == "function" then return fn() end
    return true
end

-- Sets screen split direction (0 = combined, 1 = horizontal, 2 = vertical).
function Game.SetScreenSplitDirection(direction)
    local fn = _G["UI_ScreenSplitDirection"]
    if type(fn) == "function" then
        fn(direction)
        return true
    end
    return false
end

-- Sets whether multiplayer drop-in join is blocked.
function Game.SetDropInBlocked(blocked)
    local fn = _G["DropInBlocked"]
    if type(fn) == "function" then
        fn(blocked and true or false)
        return true
    end
    return false
end

-- Returns table of active local player IDs.
function Game.GetLocalPlayers()
    local fn = _G["Players_GetLocalPlayers"]
    if type(fn) == "function" then
        local players = fn()
        if type(players) == "table" then return players end
    end
    return { 0 }
end

-- Returns number of active local players.
function Game.NumLocalPlayers()
    local fn = _G["Players_NumLocalPlayers"]
    if type(fn) == "function" then return fn() end
    return 1
end

-- Binds and locks a player ID to a controller index.
function Game.LockPlayerController(playerId, controllerIndex)
    local fn = _G["LockPlayerToController"]
    if type(fn) == "function" then
        fn(playerId or 0, controllerIndex or 0)
        return true
    end
    return false
end

-- Unlocks all player controller bindings.
function Game.UnlockAllControllers()
    local fn = _G["UnlockAllControllers"]
    if type(fn) == "function" then
        fn()
        return true
    end
    return false
end

-- Removes the specified local player from session.
function Game.RemoveLocalPlayer(playerId)
    local fn = _G["RemovePlayer"]
    if type(fn) == "function" then
        fn(playerId)
        return true
    end
    return false
end
