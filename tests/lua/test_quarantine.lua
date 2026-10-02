local harness = ...

local assertEquals = harness.assertEquals
local assertTrue = harness.assertTrue
local assertFalse = harness.assertFalse
local assertContains = harness.assertContains

local suite = { name = "Crabe.Quarantine", cases = {} }

local function failer(message)
    return function() error(message or "boom", 0) end
end

local function counting()
    local calls = 0
    local fn = function() calls = calls + 1 end
    return fn, function() return calls end
end


suite.cases[#suite.cases + 1] = {
    "ten consecutive failures disable the callback, not the mod",
    function(state)
        local Q = state.Crabe.Quarantine
        local fn = failer()

        for _ = 1, 9 do
            Q.guard("modA", "onUpdate", fn)
        end
        assertFalse(Q.isCallbackDisabled("modA", "onUpdate"), "not yet disabled at nine")

        Q.guard("modA", "onUpdate", fn)
        assertTrue(Q.isCallbackDisabled("modA", "onUpdate"), "disabled at ten")
        assertFalse(Q.isModDisabled("modA"), "one disabled callback does not disable the mod")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a success resets the consecutive counter to zero",
    function(state)
        local Q = state.Crabe.Quarantine
        local fail = failer()
        local succeed = function() end

        for _ = 1, 9 do
            Q.guard("modA", "onUpdate", fail)
        end
        Q.guard("modA", "onUpdate", succeed)
        assertFalse(Q.isCallbackDisabled("modA", "onUpdate"), "a success before ten resets the streak")

        for _ = 1, 9 do
            Q.guard("modA", "onUpdate", fail)
        end
        assertFalse(Q.isCallbackDisabled("modA", "onUpdate"), "still not disabled: only nine since the reset")

        Q.guard("modA", "onUpdate", fail)
        assertTrue(Q.isCallbackDisabled("modA", "onUpdate"), "the tenth since the reset disables it")
    end,
}

suite.cases[#suite.cases + 1] = {
    "three disabled callbacks disable the mod, and everything it owns then stops running",
    function(state)
        local Q = state.Crabe.Quarantine
        local fail = failer()

        for _ = 1, 10 do Q.guard("modB", "onInit", fail) end
        assertTrue(Q.isCallbackDisabled("modB", "onInit"))
        assertFalse(Q.isModDisabled("modB"), "one down")

        for _ = 1, 10 do Q.guard("modB", "onUpdate", fail) end
        assertTrue(Q.isCallbackDisabled("modB", "onUpdate"))
        assertFalse(Q.isModDisabled("modB"), "two down")

        for _ = 1, 10 do Q.guard("modB", "onDraw", fail) end
        assertTrue(Q.isModDisabled("modB"), "three down disables the mod")

        local fn, calls = counting()
        Q.guard("modB", "onShutdown", fn)
        assertEquals(calls(), 0, "a disabled mod's callback is never even attempted")
        assertFalse(Q.isCallbackDisabled("modB", "onShutdown"),
            "never attempted means never counted as failing, either")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a disabled callback is skipped, not retried",
    function(state)
        local Q = state.Crabe.Quarantine
        local calls = 0
        local fn = function() calls = calls + 1; error("boom", 0) end

        for _ = 1, 10 do Q.guard("modC", "onUpdate", fn) end
        assertEquals(calls, 10, "called through to the tenth failure")

        for _ = 1, 5 do Q.guard("modC", "onUpdate", fn) end
        assertEquals(calls, 10, "never called again once disabled")
    end,
}

suite.cases[#suite.cases + 1] = {
    "guard tolerates a missing or non-function callback",
    function(state)
        local Q = state.Crabe.Quarantine
        Q.guard("modD", "onUpdate", nil)
        Q.guard("modD", "onUpdate", "not a function")
        assertFalse(Q.isCallbackDisabled("modD", "onUpdate"))
    end,
}


suite.cases[#suite.cases + 1] = {
    "an unbroken run of identical failures logs once, then a single repeat summary",
    function(state)
        local Crabe = state.Crabe
        local Q = Crabe.Quarantine
        local fail = failer("boom")
        local base = #Crabe._lines

        for _ = 1, 5 do Q.guard("modE", "onDraw", fail) end
        assertEquals(#Crabe._lines, base + 1, "five identical failures produce exactly one line so far")
        assertContains(Crabe._lines[base + 1], "boom")

        Q.guard("modE", "onDraw", function() end) -- ends the streak
        assertEquals(#Crabe._lines, base + 2, "the streak's end flushes exactly one summary line")
        assertContains(Crabe._lines[base + 2], "repeated 4 times")
        assertContains(Crabe._lines[base + 2], "boom")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a change in the error message flushes the previous streak immediately",
    function(state)
        local Crabe = state.Crabe
        local Q = Crabe.Quarantine
        local base = #Crabe._lines

        Q.guard("modF", "onUpdate", failer("first"))
        Q.guard("modF", "onUpdate", failer("first"))
        Q.guard("modF", "onUpdate", failer("second"))

        assertEquals(#Crabe._lines, base + 3,
            "first failure, one repeat flushed on the message change, then the new message")
        assertContains(Crabe._lines[base + 1], "first")
        assertContains(Crabe._lines[base + 2], "repeated 1 times")
        assertContains(Crabe._lines[base + 3], "second")
    end,
}


suite.cases[#suite.cases + 1] = {
    "reset clears every counter and disabled flag",
    function(state)
        local Q = state.Crabe.Quarantine
        local fail = failer()

        for _ = 1, 10 do Q.guard("modG", "onInit", fail) end
        for _ = 1, 10 do Q.guard("modG", "onUpdate", fail) end
        for _ = 1, 10 do Q.guard("modG", "onDraw", fail) end
        assertTrue(Q.isModDisabled("modG"))

        Q.reset()

        assertFalse(Q.isModDisabled("modG"))
        assertFalse(Q.isCallbackDisabled("modG", "onInit"))
        assertEquals(Q.report(), "", "a fresh state reports nothing")

        local fn, calls = counting()
        Q.guard("modG", "onInit", fn)
        assertEquals(calls(), 1)
    end,
}


suite.cases[#suite.cases + 1] = {
    "report lists disabled mods and every callback that has failed, but not ones that never failed",
    function(state)
        local Q = state.Crabe.Quarantine

        for _ = 1, 10 do Q.guard("modH", "onInit", failer()) end
        for _ = 1, 10 do Q.guard("modH", "onUpdate", failer()) end
        for _ = 1, 10 do Q.guard("modH", "onDraw", failer()) end
        assertTrue(Q.isModDisabled("modH"))

        Q.guard("modI", "onUpdate", failer("transient"))
        Q.guard("modI", "onUpdate", failer("transient"))
        Q.guard("modI", "onUpdate", function() end) -- never disabled

        Q.guard("modJ", "onUpdate", function() end) -- never fails at all

        local report = Q.report()
        assertContains(report, "MOD\tmodH")
        assertContains(report, "CB\tmodH\tonInit\t10\t1\t")
        assertContains(report, "CB\tmodH\tonUpdate\t10\t1\t")
        assertContains(report, "CB\tmodH\tonDraw\t10\t1\t")
        assertFalse(report:find("modJ", 1, true) ~= nil, "a callback that never failed is not reported")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a callback that failed but was not disabled is reported with disabled = 0",
    function(state)
        local Q = state.Crabe.Quarantine

        Q.guard("modK", "onUpdate", failer("oops"))
        Q.guard("modK", "onUpdate", failer("oops"))
        Q.guard("modK", "onUpdate", failer("oops"))

        local report = Q.report()
        assertContains(report, "CB\tmodK\tonUpdate\t3\t0\t")
        assertFalse(report:find("MOD\t", 1, true) ~= nil, "modK was never disabled, so no MOD line at all")
    end,
}


suite.cases[#suite.cases + 1] = {
    "Crabe.Mod.dispatchUpdate quarantines a mod's onUpdate after ten ticks",
    function(state)
        local Crabe = state.Crabe
        local calls = 0

        Crabe.Mod.register({
            name = "flakyMod",
            onUpdate = function() calls = calls + 1; error("boom", 0) end,
        })

        for _ = 1, 10 do
            Crabe.Mod.dispatchUpdate(0.016)
        end
        assertEquals(calls, 10)
        assertTrue(Crabe.Quarantine.isCallbackDisabled("flakyMod", "onUpdate"))

        Crabe.Mod.dispatchUpdate(0.016)
        assertEquals(calls, 10, "dispatchUpdate no longer calls a quarantined callback")
    end,
}


suite.cases[#suite.cases + 1] = {
    "Game._runTicks quarantines a tick callback under the mod that registered it",
    function(state)
        local Crabe = state.Crabe
        local Game = state.Game
        local calls = 0

        Crabe.Registry.setCurrentOwner("tickMod")
        local fn = function() calls = calls + 1; error("boom", 0) end
        Game.onTick(fn)
        Crabe.Registry.setCurrentOwner(nil)

        local key = "onTick:" .. tostring(fn)
        for _ = 1, 10 do
            Game._runTicks(0.016)
        end
        assertEquals(calls, 10)
        assertTrue(Crabe.Quarantine.isCallbackDisabled("tickMod", key))

        Game._runTicks(0.016)
        assertEquals(calls, 10, "a quarantined tick callback is skipped, not retried")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a tick callback registered with no mod running is owned by 'core'",
    function(state)
        local Game = state.Game
        local fn = function() end
        Game.onTick(fn)
        assertEquals(Game._tickOwners[fn], "core")
    end,
}

return suite
