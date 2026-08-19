-- Live state of the played avatar: alive/dead, respawn, and input locking.
--
-- Split out of 11_avatar.lua only to stay under the 500-line cap; it loads
-- right after it (std::sort on the filename puts "11_" before "11b_" before
-- "12_") and reuses Game.ResolvePlayerId / Game.CoerceSku from there.
--
-- The death natives all take (playerNum, skuId) -- a SKU, never a character
-- index. 10_game.lua's generated Game.IsCharacterDead passes 0 for the second
-- argument, which is a different thing entirely; use Game.IsAvatarDead here.

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

-- True while the player has a living avatar on the reader. Guarded everywhere
-- in the shipped UI by UI_IsAvatarOnReader first (pausemenu.lua:663): a player
-- with no figure at all is neither alive nor dead, it simply has no avatar.
function Game.IsAvatarAlive(playerId)
    playerId = player(playerId)
    return requireNative("Game.IsAvatarAlive", "UI_IsAvatarAlive")(playerId)
end

function Game.IsAvatarOnReader(playerId)
    playerId = player(playerId)
    return requireNative("Game.IsAvatarOnReader", "UI_IsAvatarOnReader")(playerId)
end

function Game.IsReaderLocked(playerId)
    playerId = player(playerId)
    return requireNative("Game.IsReaderLocked", "UI_IsReaderLockedForPlayerID")(playerId)
end

-- Per-character death flag, which outlives a respawn: DI 3.0 keeps a figure
-- "dead" until it is revived with sparks. revive.lua:295 calls
-- Players_IsCharacterDead(playerNum, avatar.sku_id). Defaults to the sku being
-- played, so it answers about the current character.
function Game.IsAvatarDead(playerId, sku)
    playerId = player(playerId)
    sku = sku or Game.GetAvatarSku(playerId)
    if not sku then
        error("Game.IsAvatarDead: could not resolve a sku_id for player " .. tostring(playerId), 2)
    end
    return requireNative("Game.IsAvatarDead", "Players_IsCharacterDead")(playerId, sku)
end

-- Clears the stored death of one character. This is the whole of what the
-- revive screen buys with sparks (revive.lua:137) -- it does not put the avatar
-- back in the world, Game.ReviveAvatar does that part.
function Game.ClearDeathInfo(playerId, sku)
    playerId = player(playerId)
    sku = sku or Game.GetAvatarSku(playerId)
    if not sku then
        error("Game.ClearDeathInfo: could not resolve a sku_id for player " .. tostring(playerId), 2)
    end
    requireNative("Game.ClearDeathInfo", "Players_RemovePlayerDeathInfo")(playerId, sku)
    return sku
end

-- Puts the avatar back at the last checkpoint. The sku goes through CoerceSku
-- because the loadout screen passes its own currentCharacter.SkuId
-- (virtualreaderpc_loadout.lua:612), whose type differs between builds.
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

-- The full revive the game performs when you pay at the loadout screen
-- (virtualreaderpc_loadout.lua:575): drop the death record, then re-activate
-- the character with forceAvatarChange so the model actually comes back. Doing
-- only the first half leaves the player watching a dead body.
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

-- Feeds the engine's death-screen state machine. Returns false when the input
-- was refused, which is how the shipped screens decide not to close.
-- Confirmed modes: 0 = the loadout screen's restart branch
-- (virtualreaderpc_loadout.lua:443), 2 = leaving the revive screen without
-- reviving (revive.lua:166). What mode 1 does is unknown.
function Game.HandleDeathInput(playerId, mode)
    playerId = player(playerId)
    mode = tonumber(mode)
    if not mode then
        error("Game.HandleDeathInput: mode must be a number (0 = restart, 2 = exit)", 2)
    end
    return requireNative("Game.HandleDeathInput", "Players_HandleDeathInput")(playerId, mode)
end

-- Number of figures the reader currently holds. settingsgame.lua:149 uses it to
-- refuse a figure reset unless exactly one is present.
function Game.GetReaderAvatarCount()
    return requireNative("Game.GetReaderAvatarCount", "UI_GetReaderAvatarCount")()
end

-- DESTRUCTIVE. This is Settings > Reset Figure (settingsgame.lua:156), not a
-- respawn: it wipes the figure's saved progress. The engine only allows it with
-- exactly one figure on the reader, so the count is checked first rather than
-- letting the native fail with no explanation. Returns the native's success
-- boolean.
function Game.ResetFigure()
    local count = Game.GetReaderAvatarCount()
    if count ~= 1 then
        error("Game.ResetFigure: needs exactly 1 figure on the reader, found " .. tostring(count), 2)
    end
    return requireNative("Game.ResetFigure", "UI_PerformResetAvatar")()
end

-- Health of the play set's core, in percent. Read-only, and no shipped script
-- calls it, so the zero-argument shape is a guess -- see the report.
function Game.GetCoreHealth()
    return requireNative("Game.GetCoreHealth", "UI_GetHealthPercentageOfCore")()
end

-- Freezes every player EXCEPT this one. The only control lock with a real call
-- site (disney_id_splash.lua:23): LockOtherPlayerControls(playerNum, on). The
-- shipped screens always pair a `true` with a `false` on exit -- leave it on and
-- the other players stay frozen for good.
function Game.LockOtherPlayerControls(playerId, on)
    playerId = player(playerId)
    requireNative("Game.LockOtherPlayerControls", "LockOtherPlayerControls")(playerId, on and true or false)
    return on and true or false
end

-- Players.LockControls / UnlockControls / LockAllControls / UnlockAllControls
-- exist in the Toy Box reflection but are called nowhere in the 501 decompiled
-- screens, so these four arities are guesses from the names: the singular pair
-- takes a player, the All pair takes nothing. Wrong guesses surface as a Lua
-- error from the native, not as a silent no-op.
local function playersTableCall(fnName, key, ...)
    if type(Players) ~= "table" or type(Players[key]) ~= "function" then
        error(fnName .. ": Players." .. key .. " is not available in this Lua state", 3)
    end
    return Players[key](...)
end

function Game.LockControls(playerId)
    playerId = player(playerId)
    return playersTableCall("Game.LockControls", "LockControls", playerId)
end

function Game.UnlockControls(playerId)
    playerId = player(playerId)
    return playersTableCall("Game.UnlockControls", "UnlockControls", playerId)
end

function Game.LockAllControls()
    return playersTableCall("Game.LockAllControls", "LockAllControls")
end

function Game.UnlockAllControls()
    return playersTableCall("Game.UnlockAllControls", "UnlockAllControls")
end

-- Releases every controller-to-player binding. pressstart.lua:54 calls the
-- System_ prefixed form with no arguments; the bare UnlockAllControllers also
-- exists at runtime but has no call site, so the confirmed one is used.
function Game.UnlockAllControllers()
    return requireNative("Game.UnlockAllControllers", "System_UnlockAllControllers")()
end

-- Controller-binding probes. No shipped script calls either, so the
-- single-argument shape is a guess -- controllerIndex for the first, player
-- index for the second.
function Game.IsControllerLocked(controllerIndex)
    return requireNative("Game.IsControllerLocked", "IsControllerLocked")(controllerIndex or 0)
end

function Game.GetLockedControllerIndex(playerId)
    return requireNative("Game.GetLockedControllerIndex", "GetLockedControllerIndex")(player(playerId))
end

-- One-line "sku 1000200, niveau 3, vivant, core 100%" for a status bar. Every
-- field is optional on purpose: this is the first thing a mod menu calls, and a
-- single missing native must not cost the whole line.
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
    -- Core health is deliberately absent: UI_GetHealthPercentageOfCore has no
    -- call site, so a wrong arity there would take this whole line down.
    return table.concat(parts, ", ")
end

-- Valid player slots, as { id, isLocal } rows. The ids are 0-based and there
-- are gaps: challengelobby.lua:154 walks `for i = 0, Players_MaxPlayers() - 1`
-- and keeps only the slots Players_IsValid accepts.
function Game.ListPlayers()
    local maxPlayers = requireNative("Game.ListPlayers", "Players_MaxPlayers")()
    local isValid = requireNative("Game.ListPlayers", "Players_IsValid")

    local locals = {}
    if type(Players_GetLocalPlayers) == "function" then
        -- Comma-separated ids, e.g. "0,1" (pausemenu.lua:49).
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
