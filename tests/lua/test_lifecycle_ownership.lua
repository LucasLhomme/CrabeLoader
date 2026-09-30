local harness = ...

local assertEquals = harness.assertEquals
local assertNil = harness.assertNil

local suite = { name = "Crabe.Mod lifecycle ownership", cases = {} }

local function runLoaderPass(state, modId, chunk)
    local Crabe = state.Crabe
    Crabe.Registry.setCurrentOwner(modId)
    chunk()
    Crabe.Registry.setCurrentOwner(nil)
    Crabe.Mod.dispatchInit()
end

local function bucketSize(state, owner)
    local list = state.Crabe.Registry._byOwner[owner]
    return list and #list or 0
end

suite.cases[#suite.cases + 1] = {
    "ten loader reload cycles leak nothing when the mod subscribes from onInit",
    function(state)
        local Crabe, Game = state.Crabe, state.Game

        local baseTicks = #Game._tickCallbacks
        local baseCount = Crabe.Registry.count()
        local inits = 0

        local function chunk()
            Crabe.Mod.register({
                name = "Hot Reloadable Mod",
                onInit = function()
                    inits = inits + 1
                    Game.onTick(function() end)
                end,
            })
        end

        runLoaderPass(state, "local.hot_mod", chunk)

        assertEquals(inits, 1, "onInit ran once for the first generation")
        assertEquals(#Game._tickCallbacks, baseTicks + 1, "one tick callback on top of the baseline")

        for cycle = 1, 10 do
            Crabe.Mod.reload()
            runLoaderPass(state, "local.hot_mod", chunk)

            assertEquals(#Game._tickCallbacks, baseTicks + 1,
                         "tick callbacks after cycle " .. cycle)
            assertEquals(inits, cycle + 1,
                         "onInit ran exactly once for generation " .. (cycle + 1))
        end

        Crabe.Mod.reload()

        assertEquals(#Game._tickCallbacks, baseTicks, "tick callbacks back to baseline")
        assertEquals(Crabe.Registry.count(), baseCount, "registry count back to baseline")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a subscription made from onInit is owned by the mod, not by core",
    function(state)
        local Crabe, Game = state.Crabe, state.Game

        local coreBefore = bucketSize(state, "core")
        local tickFn = function() end

        runLoaderPass(state, "local.owned", function()
            Crabe.Mod.register({
                name = "Owned",
                onInit = function() Game.onTick(tickFn) end,
            })
        end)

        assertEquals(bucketSize(state, "local.owned"), 1, "the mod owns the subscription")
        assertEquals(bucketSize(state, "core"), coreBefore, "core gained nothing")

        assertEquals(Game._tickOwners[tickFn], "local.owned",
                     "quarantine charges the failures to the mod")
    end,
}

suite.cases[#suite.cases + 1] = {
    "onInit runs once whether register or dispatchInit gets there first",
    function(state)
        local Crabe = state.Crabe
        local inits = 0

        Crabe.Mod.dispatchInit()

        Crabe.Mod.register({ name = "late", onInit = function() inits = inits + 1 end })
        assertEquals(inits, 1, "a mod arriving after the lifecycle is up initialises at once")

        Crabe.Mod.dispatchInit()
        Crabe.Mod.dispatchInit()
        assertEquals(inits, 1, "later dispatchInit passes do not initialise it again")

        Crabe.Mod.dispatchShutdown()
        Crabe.Mod.dispatchInit()
        assertEquals(inits, 2, "after a shutdown the next generation initialises it again")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a definition table reused across reloads still initialises",
    function(state)
        local Crabe = state.Crabe
        local inits = 0

        local def = { name = "reused", onInit = function() inits = inits + 1 end }

        Crabe.Mod.register(def)
        Crabe.Mod.dispatchInit()
        assertEquals(inits, 1, "initialised on the first generation")

        for cycle = 1, 5 do
            Crabe.Mod.reload()
            Crabe.Mod.register(def)
            Crabe.Mod.dispatchInit()
            assertEquals(inits, cycle + 1, "initialised again on generation " .. (cycle + 1))
        end
    end,
}

suite.cases[#suite.cases + 1] = {
    "the owner bracket puts back whatever it displaced",
    function(state)
        local Crabe = state.Crabe

        Crabe.Mod.register({ name = "thrower", onInit = function() error("boom") end })

        Crabe.Registry.setCurrentOwner("outer")
        Crabe.Mod.dispatchInit()
        assertEquals(Crabe.Registry.currentOwner(), "outer",
                     "the chunk owner survives a dispatch that threw")

        Crabe.Registry.setCurrentOwner(nil)
        Crabe.Mod.dispatchShutdown()
        Crabe.Mod.dispatchInit()
        assertNil(Crabe.Registry.currentOwner(), "and nil is restored as nil")

        Crabe.flush()
    end,
}

return suite
