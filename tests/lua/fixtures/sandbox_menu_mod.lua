-- Fixture for test_sandbox.lua: a mod shaped like CrabeMenu/main.lua.
--
-- It registers menu entries, then draws the menu from inside the sandbox by
-- reading Crabe.Menu.stack with `#` and walking the items with ipairs -- the
-- exact reads a deep sandbox proxy turned into "No mod has registered a menu
-- entry". Loaded through mod_manager.cpp's real kLoadModChunk, never dofile.

Crabe.Menu.registerInCategory("Cheats", { label = "God Mode", toggle = true, state = false })
Crabe.Menu.registerInCategory("Cheats", { label = "Infinite Sparks", action = function() end })
Crabe.Menu.register({ label = "About", action = function() end })

local function currentMenu()
    local stack = Crabe.Menu.stack
    local frame = stack and stack[#stack]
    return frame and frame.menu or nil
end

Crabe.Mod.register({
    name = "SandboxMenuFixture",
    onDraw = function()
        local menu = currentMenu()
        if not menu or not menu.items or #menu.items == 0 then
            ImGui.TextDisabled("No mod has registered a menu entry.")
            return
        end
        for i, item in ipairs(menu.items) do
            ImGui.Selectable(item.label .. "##item_" .. i, i == 1)
        end
    end,
})
