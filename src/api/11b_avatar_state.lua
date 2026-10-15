-- CrabeLoader
-- File description:
-- Live state of the played avatar: alive or dead, respawn, revive, and controller locking.
-- Split from 11_avatar.lua because this half is polled every frame while identity rarely changes.
-- Watches no death itself; the transition watcher is Game.onDeath in src/api/20_hooks.lua.
--
-- Authors: @LucasLhomme

Game = Game or {}

local function player(playerId)
    return Game.ResolvePlayerId(playerId)
end

local function requireNative(fnName, nativeName)
    if type(_G[nativeName]) ~= "function" then
        error(fnName .. ": " .. nativeName .. " is not available in this Lua state", 3)
    end
    return _G[nativeName]
end

-- Returns true if the specified player avatar is currently alive.
function Game.IsAvatarAlive(playerId)
    playerId = player(playerId)
    return requireNative("Game.IsAvatarAlive", "UI_IsAvatarAlive")(playerId)
end

-- Returns true if the player's avatar figure is on the reader.
function Game.IsAvatarOnReader(playerId)
    playerId = player(playerId)
    return requireNative("Game.IsAvatarOnReader", "UI_IsAvatarOnReader")(playerId)
end

-- Returns true if the virtual reader is locked for the player.
function Game.IsReaderLocked(playerId)
    playerId = player(playerId)
    return requireNative("Game.IsReaderLocked", "UI_IsReaderLockedForPlayerID")(playerId)
end

-- Returns true if the avatar figure is marked dead in game state.
function Game.IsAvatarDead(playerId, sku)
    playerId = player(playerId)
    sku = sku or Game.GetAvatarSku(playerId)
    if not sku then
        error("Game.IsAvatarDead: could not resolve a sku_id for player " .. tostring(playerId), 2)
    end
    return requireNative("Game.IsAvatarDead", "Players_IsCharacterDead")(playerId, sku)
end

-- Clears stored death record for the specified character figure.
function Game.ClearDeathInfo(playerId, sku)
    playerId = player(playerId)
    sku = sku or Game.GetAvatarSku(playerId)
    if not sku then
        error("Game.ClearDeathInfo: could not resolve a sku_id for player " .. tostring(playerId), 2)
    end
    requireNative("Game.ClearDeathInfo", "Players_RemovePlayerDeathInfo")(playerId, sku)
    return sku
end

-- Respawns the player avatar at the last checkpoint.
function Game.CheckpointRespawn(playerId, sku)
    playerId = player(playerId)
    sku = sku or Game.GetAvatarSku(playerId)
    if not sku then
        error("Game.CheckpointRespawn: could not resolve a sku_id for player " .. tostring(playerId), 2)
    end
    requireNative("Game.CheckpointRespawn", "VirtualReaderPC_AvatarCheckpointRespawn")(
        playerId, Game.CoerceSku(sku, playerId))
    return sku
end

-- Clears death record and reloads the character model.
function Game.ReviveAvatar(playerId, sku)
    playerId = player(playerId)
    sku = sku or Game.GetAvatarSku(playerId)
    if not sku then
        error("Game.ReviveAvatar: could not resolve a sku_id for player " .. tostring(playerId), 2)
    end

    Game.ClearDeathInfo(playerId, sku)
    Game.SetCharacter(sku, "loadout", playerId)
    return sku
end

-- Feeds death screen input state (0 = restart, 2 = exit).
function Game.HandleDeathInput(playerId, mode)
    playerId = player(playerId)
    mode = tonumber(mode)
    if not mode then
        error("Game.HandleDeathInput: mode must be a number (0 = restart, 2 = exit)", 2)
    end
    return requireNative("Game.HandleDeathInput", "Players_HandleDeathInput")(playerId, mode)
end

-- Returns the count of avatar figures currently on the reader.
function Game.GetReaderAvatarCount()
    return requireNative("Game.GetReaderAvatarCount", "UI_GetReaderAvatarCount")()
end

-- Resets figure progress when exactly one figure is present.
function Game.ResetFigure()
    local count = Game.GetReaderAvatarCount()
    if count ~= 1 then
        error("Game.ResetFigure: needs exactly 1 figure on the reader, found " .. tostring(count), 2)
    end
    return requireNative("Game.ResetFigure", "UI_PerformResetAvatar")()
end

-- Returns core health percentage.
function Game.GetCoreHealth()
    return requireNative("Game.GetCoreHealth", "UI_GetHealthPercentageOfCore")()
end

-- Toggles input lock for all players other than the given player.
function Game.LockOtherPlayerControls(playerId, on)
    playerId = player(playerId)
    requireNative("Game.LockOtherPlayerControls", "LockOtherPlayerControls")(playerId, on and true or false)
    return on and true or false
end

local function playersTableCall(fnName, key, ...)
    if type(Players) ~= "table" or type(Players[key]) ~= "function" then
        error(fnName .. ": Players." .. key .. " is not available in this Lua state", 3)
    end
    return Players[key](...)
end

-- Locks input controls for the specified player.
function Game.LockControls(playerId)
    playerId = player(playerId)
    return playersTableCall("Game.LockControls", "LockControls", playerId)
end

-- Unlocks input controls for the specified player.
function Game.UnlockControls(playerId)
    playerId = player(playerId)
    return playersTableCall("Game.UnlockControls", "UnlockControls", playerId)
end

-- Locks input controls for all players.
function Game.LockAllControls()
    return playersTableCall("Game.LockAllControls", "LockAllControls")
end

-- Unlocks input controls for all players.
function Game.UnlockAllControls()
    return playersTableCall("Game.UnlockAllControls", "UnlockAllControls")
end

-- Releases all controller-to-player bindings.
function Game.UnlockAllControllers()
    return requireNative("Game.UnlockAllControllers", "System_UnlockAllControllers")()
end

-- Returns true if the given controller index is locked.
function Game.IsControllerLocked(controllerIndex)
    return requireNative("Game.IsControllerLocked", "IsControllerLocked")(controllerIndex or 0)
end

-- Returns locked controller index for the given player.
function Game.GetLockedControllerIndex(playerId)
    return requireNative("Game.GetLockedControllerIndex", "GetLockedControllerIndex")(player(playerId))
end

-- Returns a formatted status summary string for the player avatar.
function Game.GetAvatarSummary(playerId)
    playerId = player(playerId)

    local parts = { "j" .. tostring(playerId) }
    local sku = Game.GetAvatarSku(playerId)
    parts[#parts + 1] = "sku " .. tostring(sku or "?")

    if sku and type(Players_GetAvatarLevel) == "function" then
        parts[#parts + 1] = "niveau " .. tostring(Players_GetAvatarLevel(sku))
    end
    if type(UI_IsAvatarAlive) == "function" then
        parts[#parts + 1] = UI_IsAvatarAlive(playerId) and "vivant" or "mort"
    end
    if sku and type(Players_IsCharacterDead) == "function" and Players_IsCharacterDead(playerId, sku) then
        parts[#parts + 1] = "figurine morte"
    end
    if type(UI_IsAvatarOnReader) == "function" and not UI_IsAvatarOnReader(playerId) then
        parts[#parts + 1] = "pas de figurine"
    end
    return table.concat(parts, ", ")
end

-- Returns list of valid player slots with local indicator.
function Game.ListPlayers()
    local maxPlayers = requireNative("Game.ListPlayers", "Players_MaxPlayers")()
    local isValid = requireNative("Game.ListPlayers", "Players_IsValid")

    local locals = {}
    if type(Players_GetLocalPlayers) == "function" then
        for id in string.gmatch(tostring(Players_GetLocalPlayers()), "[^,]+") do
            locals[tonumber(id)] = true
        end
    end

    local rows = {}
    for i = 0, maxPlayers - 1 do
        if isValid(i) then
            rows[#rows + 1] = { id = i, isLocal = locals[i] == true }
        end
    end
    return rows
end
