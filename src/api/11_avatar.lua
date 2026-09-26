-- CrabeLoader
-- File description:
-- Avatar identity and progression: handle, sku, level, entitlements, abilities and character swap.
-- A player id and a sku both arrive in several shapes from the engine, hence the coercion helpers.
-- Reports no live state such as alive or dead; that is src/api/11b_avatar_state.lua.
--
-- Authors: @LucasLhomme

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

    -- Support passing character name directly (e.g. "SOR_Sora", "Luke Skywalker")
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

        local item = nil
        if type(VirtualReaderPC_GetItemByName) == "function" then
            local charId = nil
            local n = tonumber(sku)
            if Game.CHARACTER_ROSTER then
                for _, list in pairs(Game.CHARACTER_ROSTER) do
                    for _, c in ipairs(list) do
                        if c.sku == n then
                            charId = c.id
                            break
                        end
                    end
                    if charId then break end
                end
            end
            if charId then
                item = VirtualReaderPC_GetItemByName(charId)
            end
        end

        if item then
            VirtualReaderPC_SetCurrentCharacter(item)
        else
            VirtualReaderPC_SetCurrentCharacter(coerceSku(sku, playerId))
        end
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

-- ---------------------------------------------------------------------------
-- Master Character Roster (104 Characters)
-- ---------------------------------------------------------------------------

Game.CHARACTER_ROSTER = {
    starwars = {
        { name = "Anakin Skywalker", id = "TCW_Anakin", sku = 1000200 },
        { name = "Obi-Wan Kenobi", id = "TCW_ObiWan", sku = 1000201 },
        { name = "Yoda", id = "TCW_Yoda", sku = 1000202 },
        { name = "Ahsoka Tano", id = "TCW_Ahsoka", sku = 1000203 },
        { name = "Darth Maul", id = "TCW_DarthMaul", sku = 1000204 },
        { name = "Luke Skywalker", id = "EMP_Luke", sku = 1000206 },
        { name = "Han Solo", id = "EMP_HanSolo", sku = 1000207 },
        { name = "Princess Leia", id = "EMP_Leia", sku = 1000208 },
        { name = "Chewbacca", id = "EMP_Chewbacca", sku = 1000209 },
        { name = "Darth Vader", id = "EMP_DarthVader", sku = 1000210 },
        { name = "Boba Fett", id = "EMP_BobaFett", sku = 1000211 },
        { name = "Ezra Bridger", id = "REB_Ezra", sku = 1000212 },
        { name = "Kanan Jarrus", id = "REB_Kanan", sku = 1000213 },
        { name = "Sabine Wren", id = "REB_Sabine", sku = 1000214 },
        { name = "Zeb Orrelios", id = "REB_Zeb", sku = 1000215 },
        { name = "Finn (The Force Awakens)", id = "PSX_Emmitt", sku = 1000230 },
        { name = "Rey (The Force Awakens)", id = "PSX_Grimm", sku = 1000231 },
        { name = "Poe Dameron (The Force Awakens)", id = "PSX_James", sku = 1000232 },
        { name = "Kylo Ren (The Force Awakens)", id = "PSX_Lola", sku = 1000233 },
        { name = "Mace Windu", id = "TCW_MaceWindu", sku = 1000444 },
    },
    marvel = {
        { name = "Captain America", id = "AVG_CaptainAmerica", sku = 1000100 },
        { name = "Hulk", id = "AVG_Hulk", sku = 1000101 },
        { name = "Iron Man", id = "AVG_IronMan", sku = 1000102 },
        { name = "Thor", id = "AVG_Thor", sku = 1000103 },
        { name = "Groot", id = "GOG_Groot", sku = 1000104 },
        { name = "Rocket Raccoon", id = "GOG_RocketRaccoon", sku = 1000105 },
        { name = "Star-Lord", id = "GOG_StarLord", sku = 1000106 },
        { name = "Spider-Man", id = "SPD_Spiderman", sku = 1000107 },
        { name = "Nick Fury", id = "SPD_NickFury", sku = 1000108 },
        { name = "Black Widow", id = "AVG_BlackWidow", sku = 1000109 },
        { name = "Hawkeye", id = "AVG_Hawkeye", sku = 1000110 },
        { name = "Drax the Destroyer", id = "GOG_Drax", sku = 1000111 },
        { name = "Gamora", id = "GOG_Gamora", sku = 1000112 },
        { name = "Iron Fist", id = "SPD_IronFist", sku = 1000113 },
        { name = "Nova", id = "SPD_Nova", sku = 1000114 },
        { name = "Venom", id = "SPD_Venom", sku = 1000115 },
        { name = "Loki", id = "AVG_Loki", sku = 1000124 },
        { name = "Ronan the Accuser", id = "GOG_Ronan", sku = 1000125 },
        { name = "Green Goblin", id = "SPD_GreenGoblin", sku = 1000126 },
        { name = "Falcon", id = "AVG_Falcon", sku = 1000127 },
        { name = "Yondu", id = "GOG_Yondu", sku = 1000128 },
        { name = "Black Suit Spider-Man", id = "SPD_Spiderman_Black", sku = 1000134 },
        { name = "Ultron", id = "AVG_Ultron", sku = 1000226 },
        { name = "Vision", id = "AVG_Vision", sku = 1000225 },
        { name = "Ant-Man", id = "ANT_AntMan", sku = 1000227 },
        { name = "Black Panther", id = "AVG_BlackPanther", sku = 1000246 },
        { name = "Cap First Avenger", id = "AVG_CaptainAmerica_CW", sku = 1000229 },
        { name = "Hulkbuster", id = "AVG_HulkBuster", sku = 1000238 },
    },
    disney = {
        { name = "Mr. Incredible", id = "AV_MrIncredible", sku = 1000001 },
        { name = "Sulley", id = "MU_Sully", sku = 1000002 },
        { name = "Captain Jack Sparrow", id = "PIR_JackSparrow", sku = 1000003 },
        { name = "Lone Ranger", id = "LR_LoneRanger", sku = 1000004 },
        { name = "Tonto", id = "LR_Tonto", sku = 1000005 },
        { name = "Lightning McQueen", id = "AV_McQueen", sku = 1000006 },
        { name = "Holley Shiftwell", id = "AV_Holly", sku = 1000007 },
        { name = "Buzz Lightyear", id = "AV_Buzz", sku = 1000008 },
        { name = "Jessie", id = "AV_Jessie", sku = 1000009 },
        { name = "Mike Wazowski", id = "MU_Mike", sku = 1000010 },
        { name = "Elastigirl", id = "AV_ElastiGirl", sku = 1000011 },
        { name = "Barbossa", id = "PIR_Barbossa", sku = 1000012 },
        { name = "Davy Jones", id = "PIR_DavyJones", sku = 1000013 },
        { name = "Randall Boggs", id = "MU_Randall", sku = 1000014 },
        { name = "Syndrome", id = "AV_Syndrome", sku = 1000015 },
        { name = "Woody", id = "AV_Woody", sku = 1000016 },
        { name = "Tow Mater", id = "AV_Mater", sku = 1000017 },
        { name = "Dash", id = "AV_Dash", sku = 1000018 },
        { name = "Violet", id = "AV_Violet", sku = 1000019 },
        { name = "Francesco Bernoulli", id = "AV_Cars_Francesco", sku = 1000020 },
        { name = "Sorcerer Mickey", id = "TB_MickeyMouse", sku = 1000021 },
        { name = "Jack Skellington", id = "NBC_JackSkellington", sku = 1000022 },
        { name = "Rapunzel", id = "TAN_Rapunzel", sku = 1000023 },
        { name = "Anna", id = "FRO_Anna", sku = 1000024 },
        { name = "Elsa", id = "FRO_Elsa", sku = 1000025 },
        { name = "Phineas", id = "PNF_Phineas", sku = 1000026 },
        { name = "Agent P", id = "PNF_Perry", sku = 1000027 },
        { name = "Wreck-It Ralph", id = "WR_Ralph", sku = 1000028 },
        { name = "Vanellope", id = "WR_Vanellope", sku = 1000029 },
        { name = "Donald Duck", id = "TB_DonaldDuck", sku = 1000116 },
        { name = "Aladdin", id = "AL_Aladdin", sku = 1000117 },
        { name = "Stitch", id = "LAS_Stitch", sku = 1000118 },
        { name = "Merida", id = "BRV_Merida", sku = 1000119 },
        { name = "Tinker Bell", id = "TB_Tinkerbell", sku = 1000120 },
        { name = "Maleficent", id = "MAL_Maleficent", sku = 1000121 },
        { name = "Hiro Hamada", id = "BHS_Hiro", sku = 1000122 },
        { name = "Baymax", id = "BHS_Baymax", sku = 1000123 },
        { name = "Jasmine", id = "AL_Jasmine", sku = 1000129 },
        { name = "Sam Flynn", id = "TRN_Sam", sku = 1000150 },
        { name = "Quorra", id = "TRN_Quorra", sku = 1000151 },
        { name = "Joy", id = "OUT_Joy", sku = 1000216 },
        { name = "Anger", id = "OUT_Anger", sku = 1000217 },
        { name = "Fear", id = "OUT_Fear", sku = 1000218 },
        { name = "Sadness", id = "OUT_Sadness", sku = 1000219 },
        { name = "Disgust", id = "OUT_Disgust", sku = 1000220 },
        { name = "Mickey Mouse (Classic)", id = "TBX_ClassicMickey", sku = 1000221 },
        { name = "Minnie Mouse", id = "TBX_Minnie", sku = 1000222 },
        { name = "Mulan", id = "TBX_Mulan", sku = 1000223 },
        { name = "Olaf", id = "FRO_Olaf", sku = 1000224 },
        { name = "Baloo", id = "TBX_Baloo", sku = 1000228 },
        { name = "Spot (The Good Dinosaur)", id = "DNO_Spot", sku = 1000235 },
        { name = "Nick Wilde", id = "TBX_NickWilde", sku = 1000236 },
        { name = "Judy Hopps", id = "TBX_JudyHopps", sku = 1000237 },
        { name = "Alice", id = "ALI_Alice", sku = 1000239 },
        { name = "Mad Hatter", id = "ALI_MadHatter", sku = 1000240 },
        { name = "Time", id = "ALI_Time", sku = 1000241 },
        { name = "Dory", id = "DOR_Dory", sku = 1000242 },
        { name = "Nemo", id = "DOR_Nemo", sku = 1000243 },
    }
}

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

