local ControlsPage = require("mods.keybinds.controls_page")

local OptionsRows = {}

OptionsRows.SCREEN = "SettingsMenu"

OptionsRows.ROWS = {
    { id = "Crabe_KeyboardBindings", label = "Keyboard and mouse bindings", page = "keyboard" },
    { id = "Crabe_PadBindings", label = "Controller bindings", page = "pad" },
}

-- Checks whether the current context is the frontend main menu.
local function isFrontend(menu)
    if UI_CurrentWorldName and UI_CurrentWorldName() ~= "frontend" then
        return false
    end
    if menu and menu.ClassName and menu.ClassName == "InGameSettingsMenu" then
        return false
    end
    return true
end

-- Builds one Options row; plain text, since the game looks "@" labels up in string tables these lack.
local function newRow(spec)
    return {
        id = spec.id,
        text = spec.label,
        effectName = spec.id,
        widgetType = "Button",
        buttonState = "",
        icon = "MenuIcon_Controls",
    }
end

-- Returns the spec of one of this mod's rows by id, or nil.
local function specFor(id)
    for _, spec in ipairs(OptionsRows.ROWS) do
        if spec.id == id then return spec end
    end
end

-- Makes one Options screen instance open the pages for this mod's rows and hand every other row back.
-- The game's own SelectItem ignores ids it does not know.
local function answerRows(menu)
    if rawget(menu, "__crabeBindingsRows") then return end
    rawset(menu, "__crabeBindingsRows", true)
    local original = menu.SelectItem
    menu.SelectItem = function(self, ...)
        local data = self.listData and self.listData[self.selectedIndex]
        local spec = data and specFor(data.id)
        if spec then
            ControlsPage.open(self, spec.page)
            return
        end
        if original then return original(self, ...) end
    end
end

--- Runs after the game built its Options list: puts the two entries right under Controls, or at the end without it.
function OptionsRows.addRows(menu)
    if not isFrontend(menu) then return end
    local list = menu.listData
    if type(list) ~= "table" then return end
    for _, entry in ipairs(list) do
        if specFor(entry.id) then return end
    end
    local position = #list + 1
    for index, entry in ipairs(list) do
        if entry.id == "Scn_Controls_Options" then position = index + 1 end
    end
    for offset, spec in ipairs(OptionsRows.ROWS) do
        table.insert(list, position + offset - 1, newRow(spec))
    end
    answerRows(menu)
end

--- Registers the build hook on the game's Options list through Crabe.Settings, which waits for the class to exist.
function OptionsRows.init()
    Crabe.Settings.onBuild(OptionsRows.SCREEN, OptionsRows.addRows)
end

return OptionsRows
