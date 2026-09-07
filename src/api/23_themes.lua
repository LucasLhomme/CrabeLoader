-- Themes, Skydomes & Environment API for Disney Infinity 3.0
--
-- Exposes Toy Box skydome / skybox theme customizers and world customization.

Game = Game or {}

local function native(name, caller)
    local fn = _G[name]
    if type(fn) ~= "function" then
        error(caller .. ": " .. name .. " is not available in this Lua state", 3)
    end
    return fn
end

Game.THEME_CATALOG = {
    { id = "Coruscant", label = "Coruscant Metropolis Sky" },
    { id = "Tatooine", label = "Tatooine Twin Suns" },
    { id = "Hoth", label = "Hoth Snowstorm" },
    { id = "Space", label = "Deep Space & Stars" },
    { id = "Marvel_City", label = "Marvel Manhattan City" },
    { id = "Asgard", label = "Asgard Realm of Gods" },
    { id = "InsideOut_Mind", label = "Inside Out Imagination Land" },
    { id = "Night", label = "Midnight Clear Skies" },
    { id = "Sunset", label = "Golden Sunset" },
}

function Game.SetTheme(themeId)
    if type(themeId) ~= "string" or themeId == "" then
        error("Game.SetTheme: themeId must be a non-empty string", 2)
    end

    local fn = _G["Customize_SetTheme"] or _G["Customize_ThemeAll"]
    if type(fn) == "function" then
        fn(themeId)
        return true
    end
    return false
end

function Game.ListThemes()
    return Game.THEME_CATALOG
end
