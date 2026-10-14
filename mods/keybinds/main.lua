for name in pairs(package.loaded) do
    if name:find("^mods%.keybinds%.") then package.loaded[name] = nil end
end

local Bindings = require("mods.keybinds.bindings")

if not Bindings.supported() then
    Crabe.write("[Key Bindings] this CrabeLoader cannot remap inputs; key bindings are off")
    return
end

local Input = require("mods.keybinds.input")
local MenuFocus = require("mods.keybinds.menu_focus")
local ControlsPage = require("mods.keybinds.controls_page")
local OptionsRows = require("mods.keybinds.options_rows")

local refused = Bindings.load()
Crabe.write(string.format("[Key Bindings] %d remap(s) loaded from %s%s", #Bindings.list(), Bindings.FILE,
    refused > 0 and string.format(", %d refused by the loader", refused) or ""))
OptionsRows.init()

Crabe.Mod.register({
    name = "Key Bindings",

    onUpdate = function(dt)
        Input.pollWheel()
        Bindings.setMenuOpen(MenuFocus.poll())
        Bindings.update()
        ControlsPage.update(dt)
    end,

    onShutdown = function()
        ControlsPage.shutdown()
        Crabe.Input.clearRemap()
    end,
})
