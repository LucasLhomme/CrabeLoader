-- Crabe.Plugins -- src/api/32_plugins.lua

local harness = ...

local assertEquals = harness.assertEquals
local assertTrue = harness.assertTrue
local assertFalse = harness.assertFalse

local suite = { name = "Crabe.Plugins", cases = {} }

local REPORT = "A.asi\tgame folder\tloaded\tat 0x10000000\n"
    .. "scripts/old.asi\tscripts\tskipped\tlisted in [plugins].skip in crabe.toml\n"
    .. "mods/beta/x64.asi\tmod com.test.beta\tfailed\tnot built for 32-bit Windows (a 64-bit build?)\n"
    .. "plugins/shared.asi\tplugins\talready_loaded\t\n"

suite.cases[#suite.cases + 1] = {
    "list is empty on a loader without the native",
    function(state)
        state.Crabe._asiReport = nil
        assertEquals(#state.Crabe.Plugins.list(), 0, "no native, no plugin")
        assertFalse(state.Crabe.Plugins.isLoaded("A.asi"), "nothing is loaded")
    end,
}

suite.cases[#suite.cases + 1] = {
    "list parses one row per plugin, in load order, empty detail included",
    function(state)
        state.Crabe._asiReport = function() return REPORT end
        local plugins = state.Crabe.Plugins.list()
        assertEquals(#plugins, 4, "four plugins")
        assertEquals(plugins[1].file, "A.asi", "file")
        assertEquals(plugins[1].source, "game folder", "source with a space")
        assertEquals(plugins[1].status, "loaded", "status")
        assertEquals(plugins[1].detail, "at 0x10000000", "detail")
        assertEquals(plugins[2].detail, "listed in [plugins].skip in crabe.toml", "skip reason")
        assertEquals(plugins[3].source, "mod com.test.beta", "mod source")
        assertEquals(plugins[4].status, "already_loaded", "already loaded")
        assertEquals(plugins[4].detail, "", "empty detail")
    end,
}

suite.cases[#suite.cases + 1] = {
    "isLoaded matches the file name in any folder and any case, loaded plugins only",
    function(state)
        state.Crabe._asiReport = function() return REPORT end
        assertTrue(state.Crabe.Plugins.isLoaded("a.ASI"), "loaded, other case")
        assertTrue(state.Crabe.Plugins.isLoaded("shared.asi"), "already loaded by another loader counts")
        assertFalse(state.Crabe.Plugins.isLoaded("old.asi"), "skipped is not loaded")
        assertFalse(state.Crabe.Plugins.isLoaded("x64.asi"), "failed is not loaded")
        assertFalse(state.Crabe.Plugins.isLoaded(""), "empty name")
        assertFalse(state.Crabe.Plugins.isLoaded(nil), "no name")
    end,
}

return suite
