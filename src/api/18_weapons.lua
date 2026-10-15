-- CrabeLoader
-- File description:
-- Weapons, lightsabers and the toolset: equipping a tool and listing what exists by category.
-- A tool must be owned before it can be set active, which the engine reports rather than refuses.
-- Grants nothing -- awarding a weapon goes through src/api/17_unlock.lua.
--
-- Authors: @LucasLhomme

Game = Game or {}

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

-- Equips the specified active tool or weapon on the player.
function Game.SetActiveTool(toolName, playerId)
    if type(toolName) ~= "string" then
        error("Game.SetActiveTool: toolName must be a string", 2)
    end
    playerId = Crabe.hostPlayer(playerId)

    local fn = Crabe.native("SetActiveTool", "Game.SetActiveTool")
    fn(toolName)
    return true
end

-- Unequips the currently active tool on the player.
function Game.UnequipTool(playerId)
    return Game.SetActiveTool("", playerId)
end

-- Returns sorted list of weapon catalog category names.
function Game.ListWeaponCategories()
    local cats = {}
    for k, _ in pairs(Game.WEAPON_CATALOG) do
        cats[#cats + 1] = k
    end
    table.sort(cats)
    return cats
end

-- Returns weapon entries for the specified catalog category.
function Game.GetWeaponsInCategory(category)
    return Game.WEAPON_CATALOG[category] or {}
end
