local harness = ...

local assertEquals = harness.assertEquals
local assertTrue = harness.assertTrue
local assertFalse = harness.assertFalse

local suite = { name = "Game choreographies", cases = {} }

suite.cases[#suite.cases + 1] = {
    "the catalog lists the shipped choreographies, sorted and unique",
    function(state)
        local list = state.Game.ListChoreographies()
        assertTrue(#list > 7000, "thousands of names, got " .. #list)

        local seen = {}
        for index, name in ipairs(list) do
            assertFalse(seen[name], "duplicate " .. name)
            seen[name] = true
            assertEquals(name, name:lower(), "names are lower case")
            assertFalse(name:find("%s") ~= nil, "no whitespace in " .. name)
            if index > 1 then assertTrue(list[index - 1] < name, "sorted at " .. name) end
        end
        assertTrue(seen["rr_starbasic"], "the award choreography the game itself plays is listed")
    end,
}

suite.cases[#suite.cases + 1] = {
    "the catalog is split once and handed back as the same table",
    function(state)
        assertTrue(state.Game.ListChoreographies() == state.Game.ListChoreographies())
    end,
}

suite.cases[#suite.cases + 1] = {
    "PlayChoreography passes the player first and the name second",
    function(state)
        local calls = {}
        state.PlayAwardCho = function(...) calls[#calls + 1] = { ... } end
        state.Players_GetHostPlayerID = function() return 2 end

        assertTrue(state.Game.PlayChoreography("rr_starbasic"))
        assertTrue(state.Game.PlayChoreography("rr_hofh_mickeymouse", 1))

        assertEquals(calls[1][1], 2, "defaults to the host player")
        assertEquals(calls[1][2], "rr_starbasic")
        assertEquals(calls[2][1], 1, "an explicit player wins")
        assertEquals(calls[2][2], "rr_hofh_mickeymouse")
    end,
}

suite.cases[#suite.cases + 1] = {
    "PlayChoreography rejects an empty name and a missing native",
    function(state)
        state.PlayAwardCho = function() end
        assertFalse(pcall(state.Game.PlayChoreography, ""), "empty name")
        assertFalse(pcall(state.Game.PlayChoreography, 42), "non-string name")

        state.PlayAwardCho = nil
        local ok, err = pcall(state.Game.PlayChoreography, "rr_starbasic")
        assertFalse(ok)
        assertTrue(tostring(err):find("PlayAwardCho", 1, true) ~= nil, "the error names the native")
    end,
}

return suite
