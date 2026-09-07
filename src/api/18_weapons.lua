-- Weapons, Lightsabers & Toolset API for Disney Infinity 3.0
--
-- Exposes direct tool and weapon equips via engine native _G.SetActiveTool.
-- Extends the previous minimal tool list to cover all Jedi/Sith master lightsabers,
-- blasters, jetpacks, and franchise-specific gadgets.

Game = Game or {}

local function hostPlayer(playerId)
    if playerId then return playerId end
    return (type(Players_GetHostPlayerID) == "function") and Players_GetHostPlayerID() or 0
end

local function native(name, caller)
    local fn = _G[name]
    if type(fn) ~= "function" then
        error(caller .. ": " .. name .. " is not available in this Lua state", 3)
    end
    return fn
end

-- ---------------------------------------------------------------------------
-- Master Weapons & Tools Catalog
-- ---------------------------------------------------------------------------

Game.WEAPON_CATALOG = {
    lightsabers = {
        { id = "Tools.GreenLightsaber", label = "Green Lightsaber (Standard)" },
        { id = "Tools.Lightsaber_Ahsoka_Shoto", label = "Ahsoka Shoto Saber (Dual)" },
        { id = "Tools.Lightsaber_Mace_Prop", label = "Mace Windu Saber (Purple)" },
        { id = "Tools.Lightsaber_Yoda_Prop", label = "Yoda Saber (Green)" },
        { id = "Tools.Lightsaber_Anakin_Prop", label = "Anakin Skywalker Saber (Blue)" },
        { id = "Tools.Lightsaber_ObiWan_Prop", label = "Obi-Wan Kenobi Saber (Blue)" },
        { id = "Tools.Lightsaber_Luke_Prop", label = "Luke Skywalker Saber (Green)" },
        { id = "Tools.Lightsaber_DarthVader_Prop", label = "Darth Vader Saber (Red)" },
        { id = "Tools.Lightsaber_DarthMaul_Prop", label = "Darth Maul Saber (Double-Bladed)" },
        { id = "Tools.Lightsaber_KyloRen_Prop", label = "Kylo Ren Saber (Crossguard)" },
        { id = "Tools.Lightsaber_Ezra_Prop", label = "Ezra Bridger Blaster/Saber" },
        { id = "Tools.Lightsaber_Kanan_Prop", label = "Kanan Jarrus Saber" },
    },
    blasters = {
        { id = "Tools.Blaster", label = "Standard Blaster Rifle" },
        { id = "Tools.HanSolo_Blaster", label = "Han Solo DL-44 Heavy Blaster" },
        { id = "Tools.Chewbacca_Bowcaster", label = "Chewbacca Bowcaster" },
        { id = "Tools.BobaFett_Blaster", label = "Boba Fett EE-3 Carbine" },
        { id = "Tools.Stormtrooper_Blaster", label = "E-11 Imperial Blaster" },
        { id = "Tools.StarLord_Blaster", label = "Star-Lord Quad Blasters" },
    },
    gadgets = {
        { id = "Packs.BobaFett_Jetpack", label = "Boba Fett Mandalorian Jetpack" },
        { id = "Packs.HoverBoard", label = "Sci-Fi Hoverboard" },
        { id = "Tools.TronDisc", label = "Identity Disc (Tron)" },
        { id = "Tools.MagicWand", label = "Cinderella Magic Wand" },
        { id = "Tools.FryingPan", label = "Rapunzel Frying Pan" },
        { id = "Tools.WebShooter", label = "Spider-Man Web Shooters" },
        { id = "Tools.CaptainAmerica_Shield", label = "Captain America Vibranium Shield" },
    }
}

-- ---------------------------------------------------------------------------
-- Tool Operations
-- ---------------------------------------------------------------------------

function Game.SetActiveTool(toolName, playerId)
    if type(toolName) ~= "string" then
        error("Game.SetActiveTool: toolName must be a string", 2)
    end
    playerId = hostPlayer(playerId)

    local fn = native("SetActiveTool", "Game.SetActiveTool")
    fn(toolName)
    return true
end

function Game.UnequipTool(playerId)
    return Game.SetActiveTool("", playerId)
end

function Game.ListWeaponCategories()
    local cats = {}
    for k, _ in pairs(Game.WEAPON_CATALOG) do
        cats[#cats + 1] = k
    end
    table.sort(cats)
    return cats
end

function Game.GetWeaponsInCategory(category)
    return Game.WEAPON_CATALOG[category] or {}
end
