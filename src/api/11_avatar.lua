-- CrabeLoader
-- File description:
-- Avatar identity and progression: handle, sku, level, entitlements, abilities and character swap.
-- A player id and a sku both arrive in several shapes from the engine, hence the coercion helpers.
-- Reports no live state such as alive or dead; that is src/api/11b_avatar_state.lua.
--
-- Authors: @LucasLhomme

Game = Game or {}

-- Resolves player ID, defaulting to host player if not provided.
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

-- Coerces SKU to match current character's SKU type (string or number).
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

-- Returns the actor handle of the player avatar.
function Game.GetAvatarHandle(playerId)
    playerId = hostPlayer(playerId)

    if type(Players) ~= "table" or type(Players.GetAvatarFromPlayerId) ~= "function" then
        error("Game.GetAvatarHandle: Players.GetAvatarFromPlayerId is not available in this Lua state", 2)
    end
    return Players.GetAvatarFromPlayerId(playerId)
end

-- Returns the SKU id of the played avatar as a number, or nil.
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

-- Returns the level of the avatar with the specified SKU.
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

-- Levels up the avatar via "players" or "skilltree" route.
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

-- Sets the current avatar to an absolute level from 0 to 20, down as well as up.
-- Experience is untouched, so the next experience gained can level the avatar back up.
function Game.SetAvatarLevel(level, playerId)
    if type(Crabe._avatarSetLevel) ~= "function" then
        error("Game.SetAvatarLevel: this loader has no avatar level native", 2)
    end
    level = tonumber(level)
    if not level or level < 0 or level > 20 then
        error("Game.SetAvatarLevel: level must be a number from 0 to 20", 2)
    end

    playerId = hostPlayer(playerId)
    local sku = Game.GetAvatarSku(playerId)
    if not sku then
        error("Game.SetAvatarLevel: could not resolve the current avatar's sku_id", 2)
    end
    return Crabe._avatarSetLevel(playerId, sku, math.floor(level))
end

-- Returns the unspent skill points of the avatar, or nil when the skill tree data is unavailable.
function Game.GetSkillPoints(playerId)
    if type(UI_GetItemArgumentsForPlayer) ~= "function" then
        error("Game.GetSkillPoints: UI_GetItemArgumentsForPlayer is not available in this Lua state", 2)
    end
    return tonumber(UI_GetItemArgumentsForPlayer("SkillTree_IN3", hostPlayer(playerId), "skillpoints"))
end

-- Refunds every skill the avatar bought, without the Sparks cost the skill tree screen charges.
-- The game allows two refunds per figure, so call it sparingly.
function Game.RespecAvatar(playerId)
    if type(UI_GetItemArgumentsForPlayer) ~= "function" then
        error("Game.RespecAvatar: UI_GetItemArgumentsForPlayer is not available in this Lua state", 2)
    end
    UI_GetItemArgumentsForPlayer("SkillTree_IN3", hostPlayer(playerId), "PerformRespec", 0)
    return true
end

-- Sets current avatar progression level directly.
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

-- Swaps played character using "loadout" or "legacy" method.
function Game.SetCharacter(sku, method, playerId)
    if sku == nil then
        error("Game.SetCharacter: a sku_id is required", 2)
    end

    if type(sku) == "string" and not tonumber(sku) then
        local foundSku = nil
        if Crabe and Crabe.VirtualReader and Crabe.VirtualReader.skuForName then
            foundSku = Crabe.VirtualReader.skuForName(sku)
        end
        if not foundSku then
            local lowerName = string.lower(sku)
            for _, list in pairs(Game.CHARACTER_ROSTER) do
                for _, c in ipairs(list) do
                    if string.lower(c.name) == lowerName then
                        foundSku = c.sku
                        break
                    end
                end
                if foundSku then break end
            end
        end
        if foundSku then
            sku = foundSku
        end
    end

    playerId = hostPlayer(playerId)
    method = method or "loadout"

    if method == "loadout" then
        if type(VirtualReaderPC_SetCurrentCharacter) ~= "function"
            or type(VirtualReaderPC_ActivateChanges) ~= "function" then
            error("Game.SetCharacter: the loadout natives are not available in this Lua state", 2)
        end

        VirtualReaderPC_SetCurrentCharacter(coerceSku(sku, playerId))
        VirtualReaderPC_ActivateChanges(playerId, true)
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

-- Returns avatar name, sparks, and playset name table.
function Game.GetPlayerAvatarData(playerId)
    playerId = hostPlayer(playerId)

    if type(GetPlayerAvatarData) ~= "function" then
        error("Game.GetPlayerAvatarData: GetPlayerAvatarData is not available in this Lua state", 2)
    end
    local name, sparks, playsetName = GetPlayerAvatarData(playerId)
    return { name = name, sparks = sparks, playsetName = playsetName }
end

-- Refreshes and returns the avatar entitlement list.
function Game.GetAvatarEntitlements(playerId)
    playerId = hostPlayer(playerId)

    if type(VirtualReaderPC_GetAvatarEntitlements) ~= "function" then
        error("Game.GetAvatarEntitlements: VirtualReaderPC_GetAvatarEntitlements is not available in this Lua state", 2)
    end
    return VirtualReaderPC_GetAvatarEntitlements(playerId)
end

-- Returns core abilities of the specified progression tree name.
function Game.GetAvatarAbilities(progressionTree)
    if type(progressionTree) ~= "string" or progressionTree == "" then
        error("Game.GetAvatarAbilities: expected a ProgressionTree name (string)", 2)
    end
    if type(VirtualReaderPC_GetAvatarAbilitiesByName) ~= "function" then
        error("Game.GetAvatarAbilities: VirtualReaderPC_GetAvatarAbilitiesByName is not available in this Lua state", 2)
    end
    return VirtualReaderPC_GetAvatarAbilitiesByName(progressionTree)
end

-- Returns progression tree name of the current avatar, or nil.
function Game.GetProgressionTree(playerId)
    playerId = hostPlayer(playerId)

    if type(VirtualReaderPC_GetCurrentCharacter) ~= "function" then
        error("Game.GetProgressionTree: VirtualReaderPC_GetCurrentCharacter is not available in this Lua state", 2)
    end
    local current = VirtualReaderPC_GetCurrentCharacter(playerId)
    if type(current) ~= "table" then return nil end
    return current.ProgressionTree
end

-- Returns key-value information rows about the player avatar.
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

Game.CHARACTER_ROSTER = {
    starwars = {
        { name = "Anakin Skywalker",  sku = 1000200 },
        { name = "Obi-Wan Kenobi",    sku = 1000201 },
        { name = "Yoda",              sku = 1000202 },
        { name = "Ahsoka Tano",       sku = 1000203 },
        { name = "Darth Maul",        sku = 1000204 },
        { name = "Luke Skywalker",    sku = 1000206 },
        { name = "Han Solo",          sku = 1000207 },
        { name = "Princess Leia",     sku = 1000208 },
        { name = "Chewbacca",         sku = 1000209 },
        { name = "Darth Vader",       sku = 1000210 },
        { name = "Boba Fett",         sku = 1000211 },
        { name = "Ezra Bridger",      sku = 1000212 },
        { name = "Kanan Jarrus",      sku = 1000213 },
        { name = "Sabine Wren",       sku = 1000214 },
        { name = "Zeb Orrelios",      sku = 1000215 },
    },
    marvel = {
        { name = "Captain America",         sku = 1000100 },
        { name = "Hulk",                    sku = 1000101 },
        { name = "Iron Man",                sku = 1000102 },
        { name = "Thor",                    sku = 1000103 },
        { name = "Groot",                   sku = 1000104 },
        { name = "Rocket Raccoon",          sku = 1000105 },
        { name = "Star-Lord",               sku = 1000106 },
        { name = "Spider-Man",              sku = 1000107 },
        { name = "Nick Fury",               sku = 1000108 },
        { name = "Black Widow",             sku = 1000109 },
        { name = "Hawkeye",                 sku = 1000110 },
        { name = "Drax",                    sku = 1000111 },
        { name = "Gamora",                  sku = 1000112 },
        { name = "Iron Fist",               sku = 1000113 },
        { name = "Nova",                    sku = 1000114 },
        { name = "Venom",                   sku = 1000115 },
        { name = "Ronan the Accuser",       sku = 1000125 },
        { name = "Green Goblin",            sku = 1000126 },
        { name = "Loki",                    sku = 1000124 },
        { name = "Falcon",                  sku = 1000127 },
        { name = "Yondu",                   sku = 1000128 },
        { name = "Jasmine",                 sku = 1000129 },
        { name = "Black Suit Spider-Man",   sku = 1000134 },
        { name = "Vision",                  sku = 1000225 },
        { name = "Ultron",                  sku = 1000226 },
        { name = "Ant-Man",                 sku = 1000227 },
        { name = "Hulkbuster",              sku = 1000238 },
    },
    disney = {
        { name = "Joy",               sku = 1000216 },
        { name = "Anger",             sku = 1000217 },
        { name = "Fear",              sku = 1000218 },
        { name = "Sadness",           sku = 1000219 },
        { name = "Disgust",           sku = 1000220 },
        { name = "Mickey Mouse",      sku = 1000221 },
        { name = "Minnie Mouse",      sku = 1000222 },
        { name = "Mulan",             sku = 1000223 },
        { name = "Olaf",              sku = 1000224 },
        { name = "Sam Flynn",         sku = 1000150 },
        { name = "Quorra",            sku = 1000151 },
        { name = "Aladdin",           sku = 1000117 },
        { name = "Stitch",            sku = 1000118 },
        { name = "Merida",            sku = 1000119 },
        { name = "Tinker Bell",       sku = 1000120 },
        { name = "Maleficent",        sku = 1000121 },
        { name = "Hiro Hamada",       sku = 1000122 },
        { name = "Baymax",            sku = 1000123 },
        { name = "Anna",              sku = 1000024 },
        { name = "Elsa",              sku = 1000025 },
        { name = "Rapunzel",          sku = 1000023 },
        { name = "Mickey (TB 1.0)",   sku = 1000021 },
        { name = "Jack Skellington",  sku = 1000022 },
        { name = "Spot",              sku = 1000235 },
        { name = "Nick Wilde",        sku = 1000236 },
        { name = "Judy Hopps",        sku = 1000237 },
    },
}

-- Returns characters list filtered by franchise or modded roster.
function Game.ListCharacters(franchise)
    if franchise then
        local f = string.lower(franchise)
        if f == "custom" or f == "mods" then
            if Crabe and Crabe.VirtualReader and Crabe.VirtualReader.getModdedCharacters then
                local res = {}
                for _, row in ipairs(Crabe.VirtualReader.getModdedCharacters()) do
                    table.insert(res, { name = row.Name, sku = tonumber(row.sku_id) or row.sku_id, icon = row.Icon })
                end
                return res
            end
            return {}
        end
        return Game.CHARACTER_ROSTER[f] or {}
    end
    local all = {}
    for _, list in pairs(Game.CHARACTER_ROSTER) do
        for _, c in ipairs(list) do
            all[#all + 1] = c
        end
    end
    if Crabe and Crabe.VirtualReader and Crabe.VirtualReader.getModdedCharacters then
        for _, row in ipairs(Crabe.VirtualReader.getModdedCharacters()) do
            all[#all + 1] = { name = row.Name, sku = tonumber(row.sku_id) or row.sku_id, icon = row.Icon }
        end
    end
    return all
end
