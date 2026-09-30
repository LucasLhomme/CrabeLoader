-- Crabe.Vfs -- src/api/28_vfs.lua

local harness = ...

local assertEquals = harness.assertEquals
local assertNil = harness.assertNil
local assertTrue = harness.assertTrue
local assertFalse = harness.assertFalse
local assertNotNil = harness.assertNotNil

local suite = { name = "Crabe.Vfs", cases = {} }

suite.cases[#suite.cases + 1] = {
    "Crabe.Vfs table and public methods are exposed",
    function(state)
        local Vfs = state.Crabe.Vfs
        assertNotNil(Vfs, "Crabe.Vfs must exist")
        assertEquals(type(Vfs.count), "function", "count is a function")
        assertEquals(type(Vfs.resolve), "function", "resolve is a function")
        assertEquals(type(Vfs.stats), "function", "stats is a function")
        assertEquals(type(Vfs.lastRedirected), "function", "lastRedirected is a function")
    end,
}

suite.cases[#suite.cases + 1] = {
    "count returns 0 when native hook is absent",
    function(state)
        local Vfs = state.Crabe.Vfs
        state.Crabe._vfsGetOverrideCount = nil
        assertEquals(Vfs.count(), 0, "count defaults to 0")
    end,
}

suite.cases[#suite.cases + 1] = {
    "count delegates to Crabe._vfsGetOverrideCount when present",
    function(state)
        local Vfs = state.Crabe.Vfs
        state.Crabe._vfsGetOverrideCount = function() return 42 end
        assertEquals(Vfs.count(), 42, "count returns value from native hook")
    end,
}

suite.cases[#suite.cases + 1] = {
    "resolve returns nil when path is empty, non-string, or native hook is absent",
    function(state)
        local Vfs = state.Crabe.Vfs
        state.Crabe._vfsResolve = nil

        assertNil(Vfs.resolve("textures/test.png"), "unhooked resolve returns nil")
        assertNil(Vfs.resolve(""), "empty string returns nil")
        assertNil(Vfs.resolve(nil), "nil path returns nil")
        assertNil(Vfs.resolve(123), "numeric path returns nil")
        assertNil(Vfs.resolve({}), "table path returns nil")
        assertNil(Vfs.resolve(true), "boolean path returns nil")
    end,
}

suite.cases[#suite.cases + 1] = {
    "resolve guards against invalid arguments before calling native hook",
    function(state)
        local Vfs = state.Crabe.Vfs
        local called = false
        state.Crabe._vfsResolve = function(p)
            called = true
            return "override/" .. p
        end

        assertNil(Vfs.resolve(""), "empty path rejected before native")
        assertFalse(called, "native hook must not be called on empty path")

        assertNil(Vfs.resolve(nil), "nil path rejected before native")
        assertFalse(called, "native hook must not be called on nil path")

        assertNil(Vfs.resolve(42), "number path rejected before native")
        assertFalse(called, "native hook must not be called on numeric path")
    end,
}

suite.cases[#suite.cases + 1] = {
    "resolve delegates to Crabe._vfsResolve and returns redirected path",
    function(state)
        local Vfs = state.Crabe.Vfs
        local passedPath = nil
        state.Crabe._vfsResolve = function(path)
            passedPath = path
            if path == "characters/alice.lua" then
                return "mods/modA/characters/alice.lua"
            end
            return nil
        end

        local resolved = Vfs.resolve("characters/alice.lua")
        assertEquals(passedPath, "characters/alice.lua", "passed path")
        assertEquals(resolved, "mods/modA/characters/alice.lua", "resolved override")

        local unmapped = Vfs.resolve("characters/bob.lua")
        assertEquals(passedPath, "characters/bob.lua", "passed unmapped path")
        assertNil(unmapped, "unmapped path resolves to nil")
    end,
}

suite.cases[#suite.cases + 1] = {
    "stats returns zeroes when native hook is absent",
    function(state)
        local Vfs = state.Crabe.Vfs
        state.Crabe._vfsGetStats = nil

        local s = Vfs.stats()
        assertEquals(type(s), "table", "stats must return a table")
        assertEquals(s.totalOverrides, 0, "totalOverrides defaults to 0")
        assertEquals(s.totalResolutions, 0, "totalResolutions defaults to 0")
        assertEquals(s.totalHits, 0, "totalHits defaults to 0")
    end,
}

suite.cases[#suite.cases + 1] = {
    "stats parses return values from Crabe._vfsGetStats",
    function(state)
        local Vfs = state.Crabe.Vfs
        state.Crabe._vfsGetStats = function()
            return 15, 120, 95
        end

        local s = Vfs.stats()
        assertEquals(s.totalOverrides, 15, "totalOverrides matches")
        assertEquals(s.totalResolutions, 120, "totalResolutions matches")
        assertEquals(s.totalHits, 95, "totalHits matches")
    end,
}

suite.cases[#suite.cases + 1] = {
    "stats safely defaults nil elements returned by Crabe._vfsGetStats",
    function(state)
        local Vfs = state.Crabe.Vfs
        state.Crabe._vfsGetStats = function()
            return 5, nil, nil
        end

        local s = Vfs.stats()
        assertEquals(s.totalOverrides, 5, "totalOverrides matches")
        assertEquals(s.totalResolutions, 0, "totalResolutions defaults to 0")
        assertEquals(s.totalHits, 0, "totalHits defaults to 0")
    end,
}

suite.cases[#suite.cases + 1] = {
    "lastRedirected returns empty string when native hook is absent",
    function(state)
        local Vfs = state.Crabe.Vfs
        state.Crabe._vfsLastRedirected = nil
        assertEquals(Vfs.lastRedirected(), "", "defaults to empty string")
    end,
}

suite.cases[#suite.cases + 1] = {
    "lastRedirected delegates to Crabe._vfsLastRedirected",
    function(state)
        local Vfs = state.Crabe.Vfs
        state.Crabe._vfsLastRedirected = function()
            return "ui/hud_override.lua"
        end
        assertEquals(Vfs.lastRedirected(), "ui/hud_override.lua", "returns last redirected path")
    end,
}

return suite
