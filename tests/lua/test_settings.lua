local harness = ...

local assertEquals = harness.assertEquals
local assertTrue = harness.assertTrue
local assertFalse = harness.assertFalse
local assertNotNil = harness.assertNotNil

local suite = { name = "Crabe.Settings", cases = {} }

local function newScreen(state, name, opts)
    opts = opts or {}
    local screen = { buildCalls = 0 }
    screen.BuildList = function(self, ...)
        self.buildCalls = self.buildCalls + 1
        if not opts.noList then
            self.listData = { { id = "gamma" }, { id = "resolution" } }
        end
        return "original-result"
    end
    state[name] = screen
    return screen
end

local function option(id, text)
    return {
        id = id,
        text = text or id,
        widgetType = "Toggle",
        get = function() return true end,
        set = function() end,
    }
end

local function idsIn(list)
    local ids = {}
    for _, entry in ipairs(list or {}) do
        ids[#ids + 1] = entry.id
    end
    return table.concat(ids, ",")
end

suite.cases[#suite.cases + 1] = {
    "an option added before the screen exists is installed when it appears",
    function(state)
        assertEquals(state.SettingsVideo, nil, "the screen must not exist yet")
        assertTrue(state.Crabe.Settings.addOption("SettingsVideo", option("crabeWindowMode")))

        local screen = newScreen(state, "SettingsVideo")
        state.Crabe.Settings._install("SettingsVideo")
        screen:BuildList()

        assertEquals(idsIn(screen.listData), "gamma,resolution,crabeWindowMode")
    end,
}

suite.cases[#suite.cases + 1] = {
    "an option added after the screen exists installs immediately",
    function(state)
        local screen = newScreen(state, "SettingsVideo")
        assertTrue(state.Crabe.Settings.addOption("SettingsVideo", option("crabeWindowMode")))

        screen:BuildList()
        assertEquals(idsIn(screen.listData), "gamma,resolution,crabeWindowMode")
    end,
}

suite.cases[#suite.cases + 1] = {
    "BuildList still returns the original result and runs the original once",
    function(state)
        local screen = newScreen(state, "SettingsVideo")
        state.Crabe.Settings.addOption("SettingsVideo", option("crabeWindowMode"))

        local result = screen:BuildList()
        assertEquals(result, "original-result", "the game's return value must survive")
        assertEquals(screen.buildCalls, 1, "the original must run exactly once")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a screen that builds no list does not take the menu down",
    function(state)
        local screen = newScreen(state, "SettingsVideo", { noList = true })
        state.Crabe.Settings.addOption("SettingsVideo", option("crabeWindowMode"))

        local ok, result = pcall(screen.BuildList, screen)
        assertTrue(ok, "BuildList must not raise when there is no list")
        assertEquals(result, "original-result")
    end,
}

suite.cases[#suite.cases + 1] = {
    "reopening the screen does not add the option twice",
    function(state)
        local screen = newScreen(state, "SettingsVideo")
        state.Crabe.Settings.addOption("SettingsVideo", option("crabeWindowMode"))

        for _ = 1, 5 do screen:BuildList() end
        assertEquals(idsIn(screen.listData), "gamma,resolution,crabeWindowMode")
    end,
}

suite.cases[#suite.cases + 1] = {
    "installing repeatedly does not stack wrappers",
    function(state)
        local screen = newScreen(state, "SettingsVideo")
        state.Crabe.Settings.addOption("SettingsVideo", option("crabeWindowMode"))

        for _ = 1, 11 do state.Crabe.Settings._install("SettingsVideo") end

        screen:BuildList()
        assertEquals(screen.buildCalls, 1, "the game's BuildList runs once per visit")
        assertEquals(idsIn(screen.listData), "gamma,resolution,crabeWindowMode")
    end,
}

suite.cases[#suite.cases + 1] = {
    "two mods each get their option, in the order they registered",
    function(state)
        local screen = newScreen(state, "SettingsVideo")
        state.Crabe.Registry.setCurrentOwner("modA")
        state.Crabe.Settings.addOption("SettingsVideo", option("optionA"))
        state.Crabe.Registry.setCurrentOwner("modB")
        state.Crabe.Settings.addOption("SettingsVideo", option("optionB"))
        state.Crabe.Registry.setCurrentOwner(nil)

        screen:BuildList()
        assertEquals(idsIn(screen.listData), "gamma,resolution,optionA,optionB")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a hot reload revokes a mod's option instead of duplicating it",
    function(state)
        local screen = newScreen(state, "SettingsVideo")

        state.Crabe.Registry.setCurrentOwner("modA")
        state.Crabe.Settings.addOption("SettingsVideo", option("optionA"))
        state.Crabe.Registry.setCurrentOwner(nil)

        screen:BuildList()
        assertEquals(idsIn(screen.listData), "gamma,resolution,optionA")

        state.Crabe.Mod.reload()
        assertEquals(#state.Crabe.Settings.listOptions("SettingsVideo"), 0,
                     "the reload must revoke what the mod registered")

        state.Crabe.Registry.setCurrentOwner("modA")
        state.Crabe.Settings.addOption("SettingsVideo", option("optionA"))
        state.Crabe.Registry.setCurrentOwner(nil)

        screen:BuildList()
        assertEquals(idsIn(screen.listData), "gamma,resolution,optionA",
                     "and the option must come back exactly once")
    end,
}

suite.cases[#suite.cases + 1] = {
    "the screen appearing a few ticks late is still caught",
    function(state)
        state.Crabe.Settings.addOption("SettingsVideo", option("crabeWindowMode"))
        assertFalse(state.Crabe.Settings._install("SettingsVideo"),
                    "nothing to install onto yet")

        for _ = 1, 3 do state.Game._runTicks(0.016) end

        local screen = newScreen(state, "SettingsVideo")
        state.Game._runTicks(0.016)

        screen:BuildList()
        assertEquals(idsIn(screen.listData), "gamma,resolution,crabeWindowMode",
                     "the retry must have installed the wrapper")
    end,
}

suite.cases[#suite.cases + 1] = {
    "the retry stops once it lands, and gives up on a screen never opened",
    function(state)
        state.Crabe.Settings.addOption("SettingsNeverOpened", option("x"))
        assertTrue(state.Crabe.Settings._retrying["SettingsNeverOpened"],
                   "retrying while the screen is absent")

        for _ = 1, 501 do state.Game._runTicks(0.016) end
        assertEquals(state.Crabe.Settings._retrying["SettingsNeverOpened"], nil,
                     "the budget must run out rather than poll forever")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a bad screen name or entry is refused without raising",
    function(state)
        assertFalse(state.Crabe.Settings.addOption("Settings Video", option("x")))
        assertFalse(state.Crabe.Settings.addOption("Settings'); os.exit() --", option("x")))
        assertFalse(state.Crabe.Settings.addOption("SettingsVideo", nil))
        assertFalse(state.Crabe.Settings.addOption("SettingsVideo", { text = "no id" }))
    end,
}

suite.cases[#suite.cases + 1] = {
    "installing on a screen that is not there yet is a no-op, not an error",
    function(state)
        assertFalse(state.Crabe.Settings._install("NoSuchScreen"))
        state.NotAScreen = { notBuildList = true }
        assertFalse(state.Crabe.Settings._install("NotAScreen"))
    end,
}

suite.cases[#suite.cases + 1] = {
    "the shipped window_mode mod registers its toggle through this API",
    function(state)
        local chunk = assert(loadfile(harness.path("mods", "window_mode.lua")))
        setfenv(chunk, state)
        assertTrue(pcall(chunk), "the example mod must load cleanly")

        local options = state.Crabe.Settings.listOptions("SettingsVideo")
        assertEquals(#options, 1, "exactly one option registered")
        assertEquals(options[1].id, "crabeWindowMode")
        assertEquals(options[1].widgetType, "Toggle")
        assertNotNil(options[1].get, "a Toggle needs a getter")
        assertNotNil(options[1].set, "and a setter")

        local currentMode = "windowed"
        state.Crabe.GetWindowMode = function() return currentMode end
        state.Crabe.SetWindowMode = function(m) currentMode = m end

        options[1].set(nil, "crabeWindowMode", 1)
        assertEquals(currentMode, "borderless", "value 1 sets borderless")
        options[1].set(nil, "crabeWindowMode", 0)
        assertEquals(currentMode, "windowed", "value 0 sets windowed")
        options[1].set(nil, "crabeWindowMode", true)
        assertEquals(currentMode, "borderless", "value true sets borderless")
        options[1].set(nil, "crabeWindowMode", false)
        assertEquals(currentMode, "windowed", "value false sets windowed")
    end,
}

suite.cases[#suite.cases + 1] = {
    "onBuild callback runs after BuildList and can inspect/modify screen data",
    function(state)
        local screen = newScreen(state, "SettingsVideo")
        screen.resolutionText = { "1280x720", "1920x1080" }
        screen.resolutionWidths = { 1280, 1920 }
        screen.resolutionHeights = { 720, 1080 }

        local callbackRan = false
        assertTrue(state.Crabe.Settings.onBuild("SettingsVideo", function(s)
            callbackRan = true
            s.resolutionText[#s.resolutionText + 1] = "3840x2160"
            s.resolutionWidths[#s.resolutionWidths + 1] = 3840
            s.resolutionHeights[#s.resolutionHeights + 1] = 2160
        end))

        screen:BuildList()
        assertTrue(callbackRan, "onBuild callback must execute when BuildList runs")
        assertEquals(#screen.resolutionText, 3, "callback should have added 4K resolution")
        assertEquals(screen.resolutionText[3], "3840x2160")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a choice row shows its current label and moves with LEFT/RIGHT",
    function(state)
        local screen = newScreen(state, "SettingsVideo")
        local gameMoves = {}
        screen.NextEnumValue = function(self, id, direction)
            gameMoves[#gameMoves + 1] = self.listData[tonumber(id)].id .. ":" .. direction
        end

        local current = 2
        assertTrue(state.Crabe.Settings.addOption("SettingsVideo", {
            id = "crabeFrameLimit",
            text = "Frame Rate Limit",
            widgetType = "LR_Toggle",
            choices = { "Unlimited", "30 FPS", "60 FPS" },
            get = function() return current end,
            set = function(_, _, index) current = index end,
        }))

        screen:BuildList()
        assertEquals(idsIn(screen.listData), "gamma,resolution,crabeFrameLimit")
        local row = screen.listData[3]
        assertEquals(row.enumValue, "30 FPS", "the label must follow get()")
        assertEquals(row.get, nil, "the row handed to the game carries no getter, like its resolution row")

        screen:NextEnumValue("3", "RIGHT")
        assertEquals(current, 3)
        assertEquals(row.enumValue, "60 FPS")

        screen:NextEnumValue("3", "RIGHT")
        assertEquals(current, 3, "RIGHT on the last choice stays there")

        screen:NextEnumValue("3", "LEFT")
        assertEquals(row.enumValue, "30 FPS")

        screen:NextEnumValue("2", "RIGHT")
        assertEquals(table.concat(gameMoves, ","), "resolution:RIGHT", "other rows still reach the game")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a choice row without choices, get or set is refused",
    function(state)
        newScreen(state, "SettingsVideo")
        assertFalse(state.Crabe.Settings.addOption("SettingsVideo", {
            id = "broken", text = "Broken", widgetType = "LR_Toggle", choices = {},
            get = function() return 1 end, set = function() end,
        }))
    end,
}

return suite
