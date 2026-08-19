-- Avatar identity and progression for the character currently being played --
-- its handle, sku, level and the routes that swap it.
--
-- Distinct from Crabe.VirtualReader (12_virtualreader.lua), which edits the
-- character-select catalog before the game freezes it. This one acts on the
-- live player, from the game's own Lua state. Life, death, respawn and control
-- locking live next door in 11b_avatar_state.lua.
--
-- Everything here raises on failure rather than returning a status. Callers
-- that need to survive an error -- the mod menu, for one -- already wrap
-- handlers; a second layer of pcall in every caller only hides which native
-- was missing.

Game = Game or {}

-- Shared with 11b_avatar_state.lua, which has no local copy of its own.
function Game.ResolvePlayerId(playerId)
    if playerId ~= nil then return playerId end
    if type(Players_GetHostPlayerID) ~= "function" then
        error("Game.ResolvePlayerId: Players_GetHostPlayerID is not available in this Lua state", 2)
    end
    return Players_GetHostPlayerID()
end

local function hostPlayer(playerId)
    return Game.ResolvePlayerId(playerId)
end

-- The engine reports SkuId as a string in some builds and a number in others.
-- Match whatever the current character uses rather than guess: the loadout and
-- respawn natives compare by value and silently no-op on a type mismatch.
function Game.CoerceSku(sku, playerId)
    if type(VirtualReaderPC_GetCurrentCharacter) ~= "function" then
        return sku
    end

    local current = VirtualReaderPC_GetCurrentCharacter(playerId)
    if type(current) == "table" and current.SkuId ~= nil then
        if type(current.SkuId) == "number" then return tonumber(sku) or sku end
        return tostring(sku)
    end
    return sku
end

local function coerceSku(sku, playerId)
    return Game.CoerceSku(sku, playerId)
end

-- The actor handle of a player's avatar -- what the tool, feat and customize
-- screens pass on to the Actor_*/Agent_* natives. Confirmed by pausemenu.lua:43
-- and a dozen siblings. Place_GetAvatarHandle exists at runtime too but has no
-- call site anywhere, so its argument shape is a guess and it is not used here.
function Game.GetAvatarHandle(playerId)
    playerId = hostPlayer(playerId)

    if type(Players) ~= "table" or type(Players.GetAvatarFromPlayerId) ~= "function" then
        error("Game.GetAvatarHandle: Players.GetAvatarFromPlayerId is not available in this Lua state", 2)
    end
    return Players.GetAvatarFromPlayerId(playerId)
end

-- sku_id of the character currently played, as a number, or nil.
--
-- Three routes, best-confirmed first. VirtualReaderPC_GetCurrentCharacter is
-- per-player; VirtualReader_GetCurrentAvatarSku takes no argument at all
-- (revive.lua:39) so it only ever answers for the local avatar; UI_GetAvatarSKU
-- has no call site in the shipped scripts and is the last resort.
function Game.GetAvatarSku(playerId)
    playerId = hostPlayer(playerId)

    if type(VirtualReaderPC_GetCurrentCharacter) == "function" then
        local current = VirtualReaderPC_GetCurrentCharacter(playerId)
        if type(current) == "table" then
            local sku = tonumber(current.SkuId)
            if sku then return sku end
        end
    end

    if type(VirtualReader_GetCurrentAvatarSku) == "function" then
        local sku = tonumber(VirtualReader_GetCurrentAvatarSku())
        if sku then return sku end
    end

    if type(UI_GetAvatarSKU) == "function" then
        local sku = tonumber(UI_GetAvatarSKU(playerId))
        if sku then return sku end
    end
    return nil
end

-- Level of the character with this sku. The native takes a SKU and not a
-- player number, as the call site at virtualreaderdata.lua line 3241 shows.
-- A wrongly shaped version is generated in 10_game.lua; this file loads
-- afterwards and replaces it. Defaults to the character being played.
function Game.GetAvatarLevel(sku, playerId)
    sku = tonumber(sku) or Game.GetAvatarSku(playerId)
    if not sku then
        error("Game.GetAvatarLevel: could not resolve a sku_id to read the level of", 2)
    end
    if type(Players_GetAvatarLevel) ~= "function" then
        error("Game.GetAvatarLevel: Players_GetAvatarLevel is not available in this Lua state", 2)
    end
    return Players_GetAvatarLevel(sku)
end

-- Two level-up natives, both (playerNum, skuId), both confirmed:
--   "players"   Players_AvatarLevelUp         upsell.lua:151
--   "skilltree" VirtualReaderPC_AvatarLevelUp skilltreegrid_in3.lua:596 -- the
--               one behind the skill tree's own paid Level Up button.
-- Neither takes a count, so repeat the call to gain several levels.
function Game.LevelUpAvatar(playerId, sku, route)
    playerId = hostPlayer(playerId)
    sku = tonumber(sku) or Game.GetAvatarSku(playerId)
    route = route or "players"

    if not sku then
        error("Game.LevelUpAvatar: could not resolve the current avatar's sku_id", 2)
    end

    local nativeName = "Players_AvatarLevelUp"
    if route == "skilltree" then
        nativeName = "VirtualReaderPC_AvatarLevelUp"
    elseif route ~= "players" then
        error("Game.LevelUpAvatar: unknown route '" .. tostring(route) .. "' (players or skilltree)", 2)
    end

    if type(_G[nativeName]) ~= "function" then
        error("Game.LevelUpAvatar: " .. nativeName .. " is not available in this Lua state", 2)
    end
    _G[nativeName](playerId, sku)
    return sku
end

-- Sets progression outright instead of stepping it. One argument only --
-- virtualreader_in3.lua:1042 calls Players_SetAvatarProgression(level) with no
-- player and no sku, so it lands on whatever avatar the engine holds current
-- and cannot be aimed at a second player.
function Game.SetAvatarProgression(level)
    level = tonumber(level)
    if not level then
        error("Game.SetAvatarProgression: level must be a number", 2)
    end
    if type(Players_SetAvatarProgression) ~= "function" then
        error("Game.SetAvatarProgression: Players_SetAvatarProgression is not available in this Lua state", 2)
    end
    Players_SetAvatarProgression(level)
    return level
end

-- Swaps the played character. Two routes, because neither works everywhere:
--
--   "loadout"  VirtualReaderPC_SetCurrentCharacter + ActivateChanges -- the
--              path the game's own character grid takes.
--   "legacy"   Players_ForceAvatar + Players_ChangeAvatar -- fine in Toy Box,
--              but does not reload the model inside a Play Set.
--
-- Defaults to "loadout"; the caller picks when it has reason to.
function Game.SetCharacter(sku, method, playerId)
    if sku == nil then
        error("Game.SetCharacter: a sku_id is required", 2)
    end
    playerId = hostPlayer(playerId)
    method = method or "loadout"

    if method == "loadout" then
        if type(VirtualReaderPC_SetCurrentCharacter) ~= "function"
            or type(VirtualReaderPC_ActivateChanges) ~= "function" then
            error("Game.SetCharacter: the loadout natives are not available in this Lua state", 2)
        end

        VirtualReaderPC_SetCurrentCharacter(coerceSku(sku, playerId))
        VirtualReaderPC_ActivateChanges(playerId, true)   -- true = forceAvatarChange
        return method
    end

    if method == "legacy" then
        if type(Players_ChangeAvatar) ~= "function" then
            error("Game.SetCharacter: Players_ChangeAvatar is not available in this Lua state", 2)
        end

        if type(Players_ForceAvatar) == "function" then
            Players_ForceAvatar(playerId, sku)
        end
        Players_ChangeAvatar(playerId, sku)
        return method
    end

    error("Game.SetCharacter: unknown method '" .. tostring(method) .. "' (loadout or legacy)", 2)
end

-- Name, sparks and play set of a player in one call: placetoyonslot.lua:50
-- destructures exactly these three, in this order.
function Game.GetPlayerAvatarData(playerId)
    playerId = hostPlayer(playerId)

    if type(GetPlayerAvatarData) ~= "function" then
        error("Game.GetPlayerAvatarData: GetPlayerAvatarData is not available in this Lua state", 2)
    end
    local name, sparks, playsetName = GetPlayerAvatarData(playerId)
    return { name = name, sparks = sparks, playsetName = playsetName }
end

-- Refreshes the entitlement list the character grid filters on, and returns the
-- native's own success boolean (virtualreaderpc.lua:333). The flat
-- Players_GetAvatarEntitlements exists at runtime with no call site anywhere,
-- so its arity is unknown and it is deliberately not wrapped.
function Game.GetAvatarEntitlements(playerId)
    playerId = hostPlayer(playerId)

    if type(VirtualReaderPC_GetAvatarEntitlements) ~= "function" then
        error("Game.GetAvatarEntitlements: VirtualReaderPC_GetAvatarEntitlements is not available in this Lua state", 2)
    end
    return VirtualReaderPC_GetAvatarEntitlements(playerId)
end

-- Core abilities of a progression tree. The argument is a ProgressionTree NAME,
-- not a sku and not a player: virtualreaderpc_coreabilities.lua:32 passes
-- self.characterInfo.ProgressionTree.
function Game.GetAvatarAbilities(progressionTree)
    if type(progressionTree) ~= "string" or progressionTree == "" then
        error("Game.GetAvatarAbilities: expected a ProgressionTree name (string)", 2)
    end
    if type(VirtualReaderPC_GetAvatarAbilitiesByName) ~= "function" then
        error("Game.GetAvatarAbilities: VirtualReaderPC_GetAvatarAbilitiesByName is not available in this Lua state", 2)
    end
    return VirtualReaderPC_GetAvatarAbilitiesByName(progressionTree)
end

-- ProgressionTree name of the played character -- the argument
-- Game.GetAvatarAbilities wants. It only exists on the loadout's character
-- table (virtualreaderpc_coreabilities.lua reads characterInfo.ProgressionTree),
-- so there is nothing to fall back on when that screen never ran. nil then.
function Game.GetProgressionTree(playerId)
    playerId = hostPlayer(playerId)

    if type(VirtualReaderPC_GetCurrentCharacter) ~= "function" then
        error("Game.GetProgressionTree: VirtualReaderPC_GetCurrentCharacter is not available in this Lua state", 2)
    end
    local current = VirtualReaderPC_GetCurrentCharacter(playerId)
    if type(current) ~= "table" then return nil end
    return current.ProgressionTree
end

-- Everything the getters can say about one player, as { label, value } rows --
-- what the menu info screen renders. Each probe is behind a type check rather
-- than a pcall, so a native missing from this build drops its own line instead
-- of aborting the whole dump.
function Game.GetAvatarInfo(playerId)
    playerId = hostPlayer(playerId)

    local rows = {}
    local function row(label, value)
        rows[#rows + 1] = { label = label, value = tostring(value) }
    end

    row("playerId", playerId)

    local sku = Game.GetAvatarSku(playerId)
    row("sku", sku or "n/a")
    if sku and type(Players_GetAvatarLevel) == "function" then
        row("niveau", Players_GetAvatarLevel(sku))
    end
    if sku and type(UI_GetAvatarVersion) == "function" then
        -- Takes the sku, not the player (virtualreader_in3.lua:596).
        row("version", UI_GetAvatarVersion(sku))
    end
    if sku and type(Player_IsCharacterValid) == "function" then
        row("perso valide", Player_IsCharacterValid(sku))
    end

    if type(GetPlayerAvatarData) == "function" then
        local name, sparks, playsetName = GetPlayerAvatarData(playerId)
        row("nom", name)
        row("sparks", sparks)
        row("playset", playsetName)
    end
    if type(UI_GetAvatarNameForIGPPopup) == "function" then
        row("nom IGP", UI_GetAvatarNameForIGPPopup(playerId))
    end
    if type(UI_GetPlayerIcon) == "function" then
        row("icone", UI_GetPlayerIcon(playerId))
    end
    if type(Players) == "table" and type(Players.GetAvatarFromPlayerId) == "function" then
        row("handle acteur", Players.GetAvatarFromPlayerId(playerId))
    end
    if type(IsPlayerSpectator) == "function" then
        row("spectateur", IsPlayerSpectator(playerId))
    end
    return rows
end
