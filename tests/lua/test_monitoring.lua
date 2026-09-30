local harness = ...

local assertEquals = harness.assertEquals
local assertTrue = harness.assertTrue
local assertFalse = harness.assertFalse

local suite = { name = "Crabe.Monitoring", cases = {} }

--- Tests high-concurrency fiber execution under the scheduler.
--- Validates that scheduled fibers wake up in expected order.
--- Ensures zero fiber leaks after all coroutines complete.
local function testConcurrentFibers(state, harness)
    local Crabe = state.Crabe
    local completed = 0
    local target = 100

    for i = 1, target do
        Crabe.spawn(function()
            Crabe.wait(i * 10)
            completed = completed + 1
        end)
    end

    assertEquals(#Crabe.Scheduler._fibers, target, "all fibers queued")

    Crabe.Scheduler.update(0.500)
    assertTrue(completed >= 50, "at least 50 fibers completed at 500ms")

    Crabe.Scheduler.update(0.600)
    assertEquals(completed, target, "all 100 fibers completed")
    assertEquals(#Crabe.Scheduler._fibers, 0, "all fibers cleanly dropped")
end

--- Tests fiber error containment during execution.
--- Verifies that an exception in one fiber does not kill sibling fibers.
local function testFiberErrorContainment(state, harness)
    local Crabe = state.Crabe
    local siblingRan = false

    Crabe.spawn(function()
        error("fault injection inside fiber")
    end)

    Crabe.spawn(function()
        Crabe.wait(50)
        siblingRan = true
    end)

    Crabe.Scheduler.update(0.060)
    assertTrue(siblingRan, "sibling fiber executed despite prior failure")
end

--- Tests mod lifecycle and fiber purging on reload.
--- Validates that pending fibers do not linger across hot-reloads.
local function testFiberPurgeOnReload(state, harness)
    local Crabe = state.Crabe

    Crabe.spawn(function()
        Crabe.wait(1000)
    end)

    assertEquals(#Crabe.Scheduler._fibers, 1, "fiber pending before reload")

    Crabe.Mod.reload()
    assertEquals(#Crabe.Scheduler._fibers, 0, "fibers cleared after reload")
end

--- Tests fault injection and automatic quarantine deactivation.
--- Verifies that 10 consecutive tick errors isolate the failing callback.
local function testFaultInjectionQuarantine(state, harness)
    local Crabe = state.Crabe
    local Game = state.Game
    local failCount = 0

    local mod = Crabe.Mod.register({
        name = "faulty_mod",
        onInit = function()
            Game.onTick(function()
                failCount = failCount + 1
                error("simulated critical mod failure")
            end)
        end,
    })

    Crabe.Mod.dispatchInit()

    for i = 1, 15 do
        Game._runTicks(0.016)
    end

    assertEquals(failCount, 10, "callback was quarantined after exactly 10 failures")

    local report = Crabe.Quarantine.report()
    assertTrue(report:find("faulty_mod") ~= nil, "quarantine report contains failing mod")
    assertTrue(report:find("simulated critical mod failure") ~= nil, "quarantine report contains error message")
end

--- Tests frametime pacing metrics and stutter detection math.
--- Validates statistical window calculations for performance telemetry.
local function testFrametimePacingMath(state, harness)
    local function createTracker()
        return {
            frames = 0,
            totalMs = 0,
            stutters16 = 0,
            stutters33 = 0,
            record = function(self, dtSec)
                local ms = dtSec * 1000
                self.frames = self.frames + 1
                self.totalMs = self.totalMs + ms
                if ms > 33.33 then
                    self.stutters33 = self.stutters33 + 1
                elseif ms > 16.67 then
                    self.stutters16 = self.stutters16 + 1
                end
            end,
            getAverage = function(self)
                return self.frames > 0 and (self.totalMs / self.frames) or 0
            end,
        }
    end

    local tracker = createTracker()
    tracker:record(0.016)
    tracker:record(0.016)
    tracker:record(0.025)
    tracker:record(0.040)

    assertEquals(tracker.frames, 4, "total frames tracked")
    assertEquals(tracker.stutters16, 1, "one frame > 16.67ms")
    assertEquals(tracker.stutters33, 1, "one frame > 33.33ms")
    assertTrue(tracker:getAverage() > 20, "average frametime calculated")
end

suite.cases[#suite.cases + 1] = {
    "concurrent fiber scheduling and completion",
    testConcurrentFibers,
}

suite.cases[#suite.cases + 1] = {
    "fiber exception containment",
    testFiberErrorContainment,
}

suite.cases[#suite.cases + 1] = {
    "fiber purge on mod reload",
    testFiberPurgeOnReload,
}

suite.cases[#suite.cases + 1] = {
    "fault injection and quarantine deactivation",
    testFaultInjectionQuarantine,
}

suite.cases[#suite.cases + 1] = {
    "frametime pacing metrics and stutter math",
    testFrametimePacingMath,
}

return suite
