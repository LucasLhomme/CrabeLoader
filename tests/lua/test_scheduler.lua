local harness = ...

local assertEquals = harness.assertEquals
local assertTrue = harness.assertTrue
local assertFalse = harness.assertFalse
local assertContains = harness.assertContains

local suite = { name = "Crabe.Scheduler", cases = {} }

suite.cases[#suite.cases + 1] = {
    "spawn runs the fiber immediately, up to its first wait",
    function(state)
        local Crabe = state.Crabe
        local steps = {}

        Crabe.spawn(function()
            steps[#steps + 1] = "before"
            Crabe.wait(100)
            steps[#steps + 1] = "after"
        end)

        assertEquals(#steps, 1, "the fiber ran up to the wait")
        assertEquals(steps[1], "before", "and no further")
        assertEquals(#Crabe.Scheduler._fibers, 1, "the suspended fiber is scheduled")
    end,
}

suite.cases[#suite.cases + 1] = {
    "update resumes a fiber only once its wait has elapsed",
    function(state)
        local Crabe = state.Crabe
        local resumed = false

        Crabe.spawn(function()
            Crabe.wait(100)
            resumed = true
        end)

        Crabe.Scheduler.update(0.050)
        assertFalse(resumed, "50 ms is not 100 ms")
        assertEquals(#Crabe.Scheduler._fibers, 1, "still pending")

        Crabe.Scheduler.update(0.060)
        assertTrue(resumed, "110 ms total resumes it")
        assertEquals(#Crabe.Scheduler._fibers, 0, "a dead fiber is dropped")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a fiber that runs to completion never enters the scheduler",
    function(state)
        local Crabe = state.Crabe
        local ran = false

        Crabe.spawn(function() ran = true end)

        assertTrue(ran, "it ran")
        assertEquals(#Crabe.Scheduler._fibers, 0, "nothing left to resume")
    end,
}

suite.cases[#suite.cases + 1] = {
    "spawn refuses a non-function and reports a fiber that throws",
    function(state)
        local Crabe = state.Crabe

        assertFalse(pcall(Crabe.spawn, "not a function"), "spawn(string) must raise")

        Crabe.flush()
        local result = Crabe.spawn(function() error("fiber exploded") end)

        assertEquals(result, nil, "a fiber that dies on its first resume returns nil")
        assertContains(Crabe.flush(), "fiber exploded", "the failure is reported")
    end,
}

suite.cases[#suite.cases + 1] = {
    "clear drops every pending fiber",
    function(state)
        local Crabe = state.Crabe
        local resumed = 0

        for _ = 1, 3 do
            Crabe.spawn(function()
                Crabe.wait(10)
                resumed = resumed + 1
            end)
        end
        assertEquals(#Crabe.Scheduler._fibers, 3, "three fibers pending")

        Crabe.Scheduler.clear()
        assertEquals(#Crabe.Scheduler._fibers, 0, "cleared")

        Crabe.Scheduler.update(1.0)
        assertEquals(resumed, 0, "a cleared fiber is never resumed")
    end,
}

suite.cases[#suite.cases + 1] = {
    "reload clears the scheduler",
    function(state)
        local Crabe = state.Crabe
        local resumed = false

        Crabe.spawn(function()
            Crabe.wait(10)
            resumed = true
        end)

        Crabe.Mod.reload()

        assertEquals(#Crabe.Scheduler._fibers, 0, "reload cleared the fibers")
        Crabe.Scheduler.update(1.0)
        assertFalse(resumed, "a fiber from before the reload never resumes")
    end,
}

return suite
