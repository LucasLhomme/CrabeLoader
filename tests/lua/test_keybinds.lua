local harness = ...

local assertEquals = harness.assertEquals
local assertTrue = harness.assertTrue
local assertFalse = harness.assertFalse
local assertNil = harness.assertNil
local assertNotNil = harness.assertNotNil

local suite = { name = "mods/keybinds (Key Bindings)", cases = {} }

local DIK_NAMES = {
    [0x11] = "Z", [0x1E] = "Q", [0x1F] = "S", [0x20] = "D", [0x39] = "Space", [0x12] = "E", [0x21] = "F",
    [0x2A] = "Left Shift", [0x1D] = "Left Ctrl", [0x10] = "A", [0x0F] = "Tab", [0x02] = "1", [0x03] = "2", [0x01] = "Esc",
}
local VK_TO_DIK = { [0x45] = 0x12, [0x46] = 0x21, [0x20] = 0x39, [0x5A] = 0x11, [0x1B] = 0x01 }

-- Builds a test state whose Crabe.Input, Storage and Settings are recording fakes, with the mod's files loadable.
-- `world.held` is the keys held by virtual key, `world.pad` the pad buttons and `world.remap` the loader's table.
-- Returns the state and that world.
local function newWorld(state)
    local world = { held = {}, pad = 0, wheel = 0, remap = { keyboard = {}, mouse = {}, pad = {} }, files = {}, builds = {} }
    local Crabe = state.Crabe
    Crabe.write = function() end
    local Input = Crabe.Input
    Input.isKeyDown = function(vk) return world.held[vk] == true end
    Input.padState = function() return { buttons = world.pad, leftX = 0, leftY = 0, leftTrigger = 0, rightTrigger = 0 } end
    Input.captureKeys = function(keys) world.captured = keys end
    Input.capturePad = function(on) world.padCaptured = on end
    Input.remapKey = function(from, to) world.remap.keyboard[from] = to; return from > 0 and from < 256 and to >= 0 and to < 256 end
    Input.remapMouseButton = function(from, to) world.remap.mouse[from] = to; return from >= 0 and from < 8 and to < 8 end
    Input.remapPadButton = function(from, to) world.remap.pad[from] = to; return true end
    Input.clearRemap = function() world.remap = { keyboard = {}, mouse = {}, pad = {} } end
    Input.scanCode = function(vk) return VK_TO_DIK[vk] or 0 end
    Input.keyName = function(code) return DIK_NAMES[code] or "" end
    Input.wheel = function() local n = world.wheel; world.wheel = 0; return n end
    Crabe.Storage.save = function(name, data) world.files[name] = Crabe.Storage.decode(Crabe.Storage.encode(data)); return true end
    Crabe.Storage.load = function(name) return world.files[name] end
    Crabe.Settings.onBuild = function(screen, fn) world.builds[screen] = fn; return true end
    Crabe.Mod.register = function(def) world.mod = def; return def end

    state._G = state
    state.require = function(name)
        local loaded = state.package.loaded[name]
        if loaded ~= nil then return loaded end
        local file = name:match("^mods%.keybinds%.(.+)$")
        if file then
            local chunk = assert(loadfile(harness.path("mods", "keybinds", file .. ".lua")))
            setfenv(chunk, state)
            local module = chunk()
            state.package.loaded[name] = module
            return module
        end
        if world.onRequire then return world.onRequire(name) end
    end
    return state, world
end

-- Loads one of the mod's modules into a world's state.
local function load(state, name)
    return state.require("mods.keybinds." .. name)
end

-- A stand-in for the game's SettingsExtras class: a list screen whose rows come from BuildList on each visit.
local function newScreenClass(calls)
    local class = {}
    class.__index = class
    function class:onEnter() calls.enter = calls.enter + 1; self:BuildList() end
    function class:BuildList() calls.build = calls.build + 1; self.listData = {} end
    function class:SelectItem(option) calls.selectItem = calls.selectItem + 1; self.gameRow = option end
    function class:onExit() calls.exit = calls.exit + 1 end
    function class:MovieLoaded() calls.loaded = calls.loaded + 1 end
    function class:Rebuild() calls.rebuild = calls.rebuild + 1 end
    function class:SetButtonLegend(legend) self.legend = legend end
    function class:CallFlashFunction(name)
        calls.flash[#calls.flash + 1] = name
        if self.flashMovesSelection ~= false then
            local step = (name == "_root.NextElement") and 1 or (name == "_root.PreviousElement") and -1 or 0
            self.selectedIndex = math.max(1, math.min(#self.listData, self.selectedIndex + step))
        end
    end
    return class
end

-- Returns a list row by id, and its index.
local function rowById(list, id)
    for index, row in ipairs(list) do
        if row.id == id then return row, index end
    end
end

-- Sets up a world with the controls page loaded and SettingsExtras created by its require.
local function newPageWorld(state)
    local _, world = newWorld(state)
    world.calls = { build = 0, selectItem = 0, exit = 0, rebuild = 0, loaded = 0, enter = 0, flash = {} }
    world.onRequire = function(name)
        if name == "SettingsExtras" and not state.SettingsExtras then state.SettingsExtras = newScreenClass(world.calls) end
    end
    world.opened = {}
    world.parent = {
        InstanceData = { playerNum = 0 },
        AddScreen = function(_, def) world.opened[#world.opened + 1] = def end,
        SetFocus = function(_, focus) world.opened.focus = focus end,
    }
    world.Page = load(state, "controls_page")
    world.Bindings = load(state, "bindings")
    world.Input = load(state, "input")
    world.Glyphs = load(state, "glyphs")
    world.glyph = function(name) return world.Glyphs.markup(name, 20, 19) end
    world.tick = function()
        world.Input.pollWheel()
        world.Bindings.update()
        world.Page.update(0.016)
    end
    world.open = function(page)
        world.Page.open(world.parent, page)
        local screen = setmetatable({ selectedIndex = 1 }, state.SettingsExtras)
        state.SettingsExtras.onEnter(screen)
        return screen
    end
    return world
end

-- Counts the actions a page lists.
local function count(Page, page)
    local total = 0
    for _, action in ipairs(Page.ACTIONS) do
        if Page.PAGES[page].devices[action.device] then total = total + 1 end
    end
    return total
end

suite.cases[#suite.cases + 1] = {
    "the keyboard page opens on the game's SettingsExtras screen with one row per action",
    function(state)
        local world = newPageWorld(state)
        local screen = world.open("keyboard")
        assertEquals(#world.opened, 1)
        assertEquals(world.opened[1].screenName, "SettingsExtras")
        assertEquals(world.opened[1].playerNum, 0)
        assertFalse(world.opened.focus, "the parent gives up focus")
        assertEquals(#screen.listData, count(world.Page, "keyboard") + 1, "actions plus a reset row")
        assertNil(rowById(screen.listData, "crabe_bind_padJump"), "no controller row")
        assertEquals(state.SettingsExtras.GetHeaderLabel(screen), "Keyboard and mouse bindings")
        assertEquals(rowById(screen.listData, "crabe_bind_jump").text, "Jump: " .. world.glyph("cbi_keyspacebar"))
        assertEquals(rowById(screen.listData, "crabe_bind_forward").text, "Move forward: " .. world.glyph("cbi_keyz"),
            "a letter follows the layout's name for the key")
        assertEquals(rowById(screen.listData, "crabe_bind_attack").text, "Attack: " .. world.glyph("cbi_mouselmb"))
        assertEquals(rowById(screen.listData, "crabe_bind_reset").text, "Reset keyboard and mouse")
        state.SettingsExtras.MovieLoaded(screen)
        assertEquals(screen.legend, "Btn_Legend_Select,Btn_Legend_Back")
    end,
}

suite.cases[#suite.cases + 1] = {
    "the controller page lists only the pad actions, with the game's button icons",
    function(state)
        local world = newPageWorld(state)
        local screen = world.open("pad")
        assertEquals(#screen.listData, count(world.Page, "pad") + 1)
        assertNil(rowById(screen.listData, "crabe_bind_jump"))
        assertEquals(rowById(screen.listData, "crabe_bind_padJump").text, "Jump: " .. world.glyph("cbi_buttona"))
        assertEquals(rowById(screen.listData, "crabe_bind_padToolLeft").text, "Previous tool: " .. world.glyph("cbi_buttondp_l"))
        assertEquals(state.SettingsExtras.GetHeaderLabel(screen), "Controller bindings")
    end,
}

suite.cases[#suite.cases + 1] = {
    "reopening the page wraps the class once and builds fresh rows",
    function(state)
        local world = newPageWorld(state)
        local first = world.open("keyboard")
        local builds = world.calls.build
        local second = world.open("keyboard")
        assertEquals(world.calls.build, builds + 1)
        assertEquals(#second.listData, count(world.Page, "keyboard") + 1)
        assertTrue(rowById(second.listData, "crabe_bind_jump") ~= rowById(first.listData, "crabe_bind_jump"))
    end,
}

suite.cases[#suite.cases + 1] = {
    "choosing a row captures the next key, swaps the bindings and saves them",
    function(state)
        local world = newPageWorld(state)
        local screen = world.open("keyboard")
        local jump, index = rowById(screen.listData, "crabe_bind_jump")
        screen.selectedIndex = index
        world.held[0x20] = true
        jump.onSelected(jump)
        assertEquals(jump.text, "Jump: [ press a key ]")
        assertEquals(screen.startId, index, "the cursor stays on the row")
        assertEquals(world.remap.keyboard[0x11], 0, "the game sees no key while it waits")
        assertEquals(world.remap.mouse[0], -1)
        assertEquals(world.remap.pad[0x1000], 0)
        assertTrue(world.padCaptured)
        assertNotNil(world.captured)

        world.tick()
        assertEquals(jump.text, "Jump: [ press a key ]", "a key already down is ignored")
        world.held[0x20] = nil
        world.tick()
        world.held[0x45] = true
        world.tick()
        assertEquals(jump.text, "Jump: " .. world.glyph("cbi_keye"))
        assertEquals(world.remap.keyboard[0x39], 0x12, "Space now does what E did")
        assertNil(world.remap.keyboard[0x11], "the keyboard is back")
        assertFalse(world.padCaptured)
        assertNil(world.captured)
        assertEquals(rowById(screen.listData, "crabe_bind_interact").text, "Interact / pick up: " .. world.glyph("cbi_keyspacebar"))
        assertEquals(world.remap.keyboard[0x12], 0, "E stays hidden while held")
        world.held[0x45] = nil
        world.tick()
        assertEquals(world.remap.keyboard[0x12], 0x39, "released, E jumps")

        local saved = world.files["Crabe/keybinds.json"]
        assertNotNil(saved, "the remaps are saved")
        assertEquals(saved.version, 1)
        assertEquals(#saved.remaps, 2)
    end,
}

suite.cases[#suite.cases + 1] = {
    "a capture ignores another device and ends on Escape without changing anything",
    function(state)
        local world = newPageWorld(state)
        local screen = world.open("keyboard")
        local forward = rowById(screen.listData, "crabe_bind_forward")
        forward.onSelected(forward)
        world.tick()
        world.pad = 0x1000
        world.tick()
        assertEquals(forward.text, "Move forward: [ press a key ]")
        world.pad = 0
        world.tick()
        world.held[0x1B] = true
        world.tick()
        assertEquals(forward.text, "Move forward: " .. world.glyph("cbi_keyz"))
        assertNil(world.remap.keyboard[0x11])
        assertEquals(world.remap.keyboard[0x01], 0, "the Escape that cancelled stays hidden while held")
        world.held[0x1B] = nil
        world.tick()
        assertNil(world.remap.keyboard[0x01])
        assertEquals(#world.Bindings.list(), 0)
    end,
}

suite.cases[#suite.cases + 1] = {
    "a capture times out after eight seconds",
    function(state)
        local world = newPageWorld(state)
        local screen = world.open("keyboard")
        local jump = rowById(screen.listData, "crabe_bind_jump")
        jump.onSelected(jump)
        for _ = 1, 600 do world.tick() end
        assertEquals(jump.text, "Jump: " .. world.glyph("cbi_keyspacebar"))
        assertFalse(world.padCaptured)
    end,
}

suite.cases[#suite.cases + 1] = {
    "a mouse row takes a mouse button, and reset puts the page back",
    function(state)
        local world = newPageWorld(state)
        local screen = world.open("keyboard")
        local attack = rowById(screen.listData, "crabe_bind_attack")
        attack.onSelected(attack)
        world.tick()
        world.held[0x02] = true
        world.tick()
        assertEquals(attack.text, "Attack: " .. world.glyph("cbi_mousermb"))
        world.held[0x02] = nil
        world.tick()
        assertEquals(world.remap.mouse[1], 0)
        local reset = rowById(screen.listData, "crabe_bind_reset")
        reset.onSelected(reset)
        assertEquals(#world.Bindings.list(), 0)
        assertEquals(attack.text, "Attack: " .. world.glyph("cbi_mouselmb"))
        assertEquals(#world.files["Crabe/keybinds.json"].remaps, 0)
    end,
}

suite.cases[#suite.cases + 1] = {
    "the wheel scrolls the page only when the game did not move the selection",
    function(state)
        local world = newPageWorld(state)
        local screen = world.open("keyboard")
        screen.selectedIndex = 3
        world.tick()
        world.wheel = -1
        world.tick()
        world.tick()
        assertEquals(screen.selectedIndex, 4, "one notch down scrolls one row")
        assertEquals(world.calls.flash[#world.calls.flash], "_root.NextElement")
        local before = #world.calls.flash
        world.tick()
        world.wheel = 1
        screen.selectedIndex = 3
        world.tick()
        world.tick()
        assertEquals(#world.calls.flash, before, "a notch the game already handled is not scrolled again")
    end,
}

suite.cases[#suite.cases + 1] = {
    "the Options rows go right under Controls and open their page",
    function(state)
        local world = newPageWorld(state)
        local Rows = load(state, "options_rows")
        Rows.init()
        assertNotNil(world.builds.SettingsMenu, "the build hook is registered on SettingsMenu")
        local selected = nil
        local menu = {
            selectedIndex = 1,
            listData = { { id = "Scn_Audio_Options" }, { id = "Scn_Controls_Options" }, { id = "Scn_Video_Options" } },
            SelectItem = function(self) selected = self.listData[self.selectedIndex].id end,
            InstanceData = { playerNum = 0 },
            AddScreen = function(_, def) world.opened[#world.opened + 1] = def end,
            SetFocus = function() end,
        }
        world.builds.SettingsMenu(menu)
        world.builds.SettingsMenu(menu)
        assertEquals(#menu.listData, 5, "added once")
        assertEquals(menu.listData[3].id, "Crabe_KeyboardBindings")
        assertEquals(menu.listData[4].id, "Crabe_PadBindings")
        menu.selectedIndex = 5
        menu:SelectItem()
        assertEquals(selected, "Scn_Video_Options", "the game's own rows still work")
        menu.selectedIndex = 4
        menu:SelectItem()
        assertEquals(#world.opened, 1)
        assertEquals(world.opened[1].screenName, "SettingsExtras")

        local inGameMenu = {
            ClassName = "InGameSettingsMenu",
            listData = { { id = "Scn_Audio_Options" } },
        }
        world.builds.SettingsMenu(inGameMenu)
        assertEquals(#inGameMenu.listData, 1, "in-game pause menu is not touched")
    end,
}

suite.cases[#suite.cases + 1] = {
    "remaps step aside while a game menu has the input",
    function(state)
        local _, world = newWorld(state)
        local Bindings = load(state, "bindings")
        assertTrue(Bindings.bind("keyboard", 0x11, 0x12))
        assertEquals(world.remap.keyboard[0x12], 0x11)
        Bindings.setMenuOpen(true)
        assertNil(world.remap.keyboard[0x12], "the menu keeps its usual keys")
        Bindings.setMenuOpen(false)
        assertEquals(world.remap.keyboard[0x12], 0x11)
    end,
}

suite.cases[#suite.cases + 1] = {
    "saved remaps are restored at load and malformed ones skipped",
    function(state)
        local _, world = newWorld(state)
        world.files["Crabe/keybinds.json"] = { version = 1, remaps = {
            { d = "keyboard", f = 0x12, t = 0x11 },
            { d = "keyboard", f = 0x11, t = 0x12 },
            { d = "joystick", f = 1, t = 2 },
            { d = "mouse", f = 0, t = 0 },
            { d = "pad", f = "x", t = 1 },
        } }
        local Bindings = load(state, "bindings")
        assertEquals(Bindings.load(), 0)
        assertEquals(#Bindings.list(), 2)
        assertEquals(world.remap.keyboard[0x12], 0x11)
        assertEquals(Bindings.producing("keyboard", 0x11), 0x12)
    end,
}

suite.cases[#suite.cases + 1] = {
    "a missing settings file means no remap",
    function(state)
        local _, world = newWorld(state)
        local Bindings = load(state, "bindings")
        assertEquals(Bindings.load(), 0)
        assertEquals(#Bindings.list(), 0)
        assertNil(next(world.remap.keyboard))
    end,
}

suite.cases[#suite.cases + 1] = {
    "main.lua loads the saved remaps, adds the Options rows and registers the mod",
    function(state)
        local _, world = newWorld(state)
        world.files["Crabe/keybinds.json"] = { version = 1, remaps = { { d = "keyboard", f = 0x12, t = 0x11 } } }
        local chunk = assert(loadfile(harness.path("mods", "keybinds", "main.lua")))
        setfenv(chunk, state)
        chunk()
        assertEquals(world.remap.keyboard[0x12], 0x11)
        assertNotNil(world.builds.SettingsMenu)
        assertEquals(world.mod.name, "Key Bindings")
        world.mod.onShutdown()
        assertNil(world.remap.keyboard[0x12], "shutdown hands the keys back")
    end,
}

suite.cases[#suite.cases + 1] = {
    "main.lua does nothing on a loader without input remapping",
    function(state)
        local _, world = newWorld(state)
        state.Crabe.Input.remapKey = nil
        local chunk = assert(loadfile(harness.path("mods", "keybinds", "main.lua")))
        setfenv(chunk, state)
        chunk()
        assertNil(world.mod)
        assertNil(world.builds.SettingsMenu)
    end,
}

suite.cases[#suite.cases + 1] = {
    "Crabe.Input.isKeyDown reads the key native",
    function(state)
        state.Crabe._keyDown = function(vk) return vk == 0x41 and 1 or 0 end
        assertTrue(state.Crabe.Input.isKeyDown(0x41))
        assertFalse(state.Crabe.Input.isKeyDown(0x42))
        assertFalse(state.Crabe.Input.isKeyDown("A"))
        state.Crabe._keyDown = nil
        assertFalse(state.Crabe.Input.isKeyDown(0x41))
    end,
}

return suite
