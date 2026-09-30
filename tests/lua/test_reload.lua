local harness = ...

local assertEquals = harness.assertEquals
local assertNil = harness.assertNil
local assertTrue = harness.assertTrue
local assertContains = harness.assertContains

local suite = { name = "Crabe.Mod.reload", cases = {} }

local function registerThreeMods(state, handlers)
    local Crabe, Game = state.Crabe, state.Game

    Crabe.Registry.setCurrentOwner("modA")
    Game.onTick(handlers.tick)
    Crabe.Registry.setCurrentOwner(nil)

    Crabe.Registry.setCurrentOwner("modB")
    Game.onDeath(0, handlers.death)
    Crabe.Registry.setCurrentOwner(nil)

    Crabe.Registry.setCurrentOwner("modC")
    Crabe.Events.on("ping", handlers.ping)
    Crabe.Registry.setCurrentOwner(nil)
end

suite.cases[#suite.cases + 1] = {
    "fifty reload cycles return every registry to baseline",
    function(state)
        local Crabe, Game = state.Crabe, state.Game

        local baseTicks = #Game._tickCallbacks
        local baseCount = Crabe.Registry.count()

        local handlers = {
            tick = function() end,
            death = function() end,
            ping = function() end,
        }

        registerThreeMods(state, handlers)

        local loadedTicks = #Game._tickCallbacks
        local loadedCount = Crabe.Registry.count()

        assertEquals(loadedTicks, baseTicks + 1, "one mod tick callback on top of the baseline")
        assertEquals(loadedCount, baseCount + 3, "three mod subscriptions on top of the baseline")
        assertEquals(#Game._deathWatchers, 1, "one death watcher")
        assertEquals(#Crabe.Events._listeners["ping"], 1, "one ping listener")

        for cycle = 1, 50 do
            Crabe.Mod.reload()
            registerThreeMods(state, handlers)

            assertEquals(#Game._tickCallbacks, loadedTicks, "tick callbacks after cycle " .. cycle)
            assertEquals(Crabe.Registry.count(), loadedCount, "registry count after cycle " .. cycle)
            assertEquals(#Game._deathWatchers, 1, "death watchers after cycle " .. cycle)
            assertEquals(#Crabe.Events._listeners["ping"], 1, "ping listeners after cycle " .. cycle)
        end

        Crabe.Mod.reload()

        assertEquals(#Game._tickCallbacks, baseTicks, "tick callbacks back to baseline")
        assertEquals(Crabe.Registry.count(), baseCount, "registry count back to baseline")
        assertEquals(#Game._deathWatchers, 0, "no death watcher left")
        assertEquals(#Crabe.Events._listeners["ping"], 0, "no ping listener left")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a throwing revoke does not abort the remaining revocations",
    function(state)
        local Crabe, Game = state.Crabe, state.Game

        local baseTicks = #Game._tickCallbacks

        Crabe.Registry.setCurrentOwner("modBad")
        Game.onTick(function() end)                            -- 1, before the thrower
        Crabe.Registry.track(function() error("boom") end)     -- 2, the thrower
        Game.onTick(function() end)                            -- 3, after the thrower
        Crabe.Events.on("zap", function() end)                 -- 4
        Crabe.Registry.setCurrentOwner(nil)

        assertEquals(#Crabe.Registry._byOwner["modBad"], 4, "four subscriptions recorded")
        assertEquals(#Game._tickCallbacks, baseTicks + 2, "two mod tick callbacks")

        Crabe.flush()
        Crabe.Mod.reload()

        assertEquals(#Game._tickCallbacks, baseTicks,
                     "both tick callbacks revoked, the one before the thrower and the one after")
        assertEquals(#Crabe.Events._listeners["zap"], 0, "the event listener was revoked")
        assertNil(Crabe.Registry._byOwner["modBad"], "the bucket is dropped even so")
        assertContains(Crabe.flush(), "revoke failed for 'modBad'", "the failure is reported")
    end,
}

suite.cases[#suite.cases + 1] = {
    "core-owned subscriptions survive a reload",
    function(state)
        local Crabe, Game = state.Crabe, state.Game

        assertNil(Crabe.Registry._current, "no mod is executing")

        local ticks, events = 0, 0
        Game.onTick(function() ticks = ticks + 1 end)
        Crabe.Events.on("coreEvt", function() events = events + 1 end)

        local coreBucket = #Crabe.Registry._byOwner["core"]
        local coreTicks = #Game._tickCallbacks

        for _ = 1, 10 do
            Crabe.Mod.reload()
        end

        assertEquals(#Crabe.Registry._byOwner["core"], coreBucket, "core bucket unchanged")
        assertEquals(#Game._tickCallbacks, coreTicks, "core tick callbacks unchanged")

        Game._runTicks(0.016)
        Crabe.Events.emit("coreEvt")

        assertEquals(ticks, 1, "the core tick callback still fires")
        assertEquals(events, 1, "the core event listener still fires")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a re-registered death watcher fires exactly once per death",
    function(state)
        local Crabe, Game = state.Crabe, state.Game
        local natives = state.CrabeTestNatives
        local fired = 0

        local function register()
            Crabe.Registry.setCurrentOwner("modD")
            Game.onDeath(0, function() fired = fired + 1 end)
            Crabe.Registry.setCurrentOwner(nil)
        end

        register()
        Crabe.Mod.reload()
        register()

        assertEquals(#Game._deathWatchers, 1,
                     "the reload revoked the first watcher, so only the new one is left")

        natives.dead[0] = false
        Game._runTicks(0.016)
        assertEquals(fired, 0, "alive: no death reported")
        assertTrue(#natives.deadPolls > 0, "the death pump polled, so the watcher is really live")

        natives.dead[0] = true
        Game._runTicks(0.016)
        assertEquals(fired, 1, "the alive-to-dead transition fires once")

        Game._runTicks(0.016)
        assertEquals(fired, 1, "staying dead does not fire again")
    end,
}

suite.cases[#suite.cases + 1] = {
    "reload callbacks are revoked with their mod",
    function(state)
        local Crabe = state.Crabe
        local calls = 0

        Crabe.Registry.setCurrentOwner("modE")
        Crabe.Mod.onReload(function() calls = calls + 1 end)
        Crabe.Registry.setCurrentOwner(nil)

        assertEquals(#Crabe.Mod._reloadCallbacks, 1, "registered")

        Crabe.Mod.reload()

        assertEquals(#Crabe.Mod._reloadCallbacks, 0, "revoked with the mod")

        Crabe.Mod.reload()
        Crabe.Mod.reload()
        assertEquals(calls, 0, "a revoked reload callback never runs again")
    end,
}

suite.cases[#suite.cases + 1] = {
    "reload shuts the previous generation down and re-initialises the next",
    function(state)
        local Crabe = state.Crabe
        local shutdowns, inits = 0, 0

        Crabe.Mod.register({
            name = "modF",
            onInit = function() inits = inits + 1 end,
            onShutdown = function() shutdowns = shutdowns + 1 end,
        })
        Crabe.Mod.dispatchInit()
        assertEquals(inits, 1, "initialised once")

        Crabe.Mod.reload()

        assertEquals(shutdowns, 1, "the previous generation was told to shut down")
        assertEquals(#Crabe.Mod._registered, 0, "the mod list is cleared for the reload")
        assertEquals(inits, 1, "nothing re-registered, so nothing was re-initialised")
    end,
}

return suite
