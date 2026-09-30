-- Crabe.Events -- src/api/05_events.lua

local harness = ...

local assertEquals = harness.assertEquals
local assertNil = harness.assertNil
local assertTrue = harness.assertTrue
local assertFalse = harness.assertFalse
local assertContains = harness.assertContains

local suite = { name = "Crabe.Events", cases = {} }

suite.cases[#suite.cases + 1] = {
    "on returns the handler it registered",
    function(state)
        local Events = state.Crabe.Events
        local handler = function() end

        -- The return value is what a caller holds on to for Events.off, so it
        -- has to be the handler itself, not a wrapper and not a boolean.
        assertEquals(Events.on("evt", handler), handler, "on return value")
        assertEquals(#Events._listeners["evt"], 1, "listener count")
    end,
}

suite.cases[#suite.cases + 1] = {
    "on refuses a bad event name or handler",
    function(state)
        local Events = state.Crabe.Events

        assertFalse(pcall(Events.on, 42, function() end), "numeric event name")
        assertFalse(pcall(Events.on, "evt", "not a function"), "string handler")
        assertFalse(pcall(Events.on, "evt", nil), "nil handler")
    end,
}

suite.cases[#suite.cases + 1] = {
    "emit calls every listener in registration order with the emitted arguments",
    function(state)
        local Events = state.Crabe.Events
        local seen = {}

        Events.on("evt", function(a, b) seen[#seen + 1] = "first:" .. a .. ":" .. b end)
        Events.on("evt", function(a, b) seen[#seen + 1] = "second:" .. a .. ":" .. b end)

        Events.emit("evt", "x", "y")

        assertEquals(#seen, 2, "listener call count")
        assertEquals(seen[1], "first:x:y", "first listener")
        assertEquals(seen[2], "second:x:y", "second listener")
    end,
}

suite.cases[#suite.cases + 1] = {
    "emit on an event with no listeners is a no-op",
    function(state)
        local Events = state.Crabe.Events
        Events.emit("nobodyIsListening", 1, 2, 3)
        assertNil(Events._listeners["nobodyIsListening"], "no bucket is created")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a throwing listener does not stop the others and is reported",
    function(state)
        local Crabe = state.Crabe
        local reached = false

        Crabe.Events.on("evt", function() error("handler exploded") end)
        Crabe.Events.on("evt", function() reached = true end)

        Crabe.flush()
        Crabe.Events.emit("evt")

        assertTrue(reached, "the listener after the thrower still runs")
        assertContains(Crabe.flush(), "handler exploded", "the failure is reported")
    end,
}

suite.cases[#suite.cases + 1] = {
    "off removes by identity and reports whether it found anything",
    function(state)
        local Events = state.Crabe.Events
        local kept, dropped = function() end, function() end

        Events.on("evt", kept)
        Events.on("evt", dropped)

        assertTrue(Events.off("evt", dropped), "off returns true when it removes")
        assertEquals(#Events._listeners["evt"], 1, "one listener left")
        assertEquals(Events._listeners["evt"][1], kept, "the right one was kept")

        assertFalse(Events.off("evt", dropped), "removing twice reports false")
        assertFalse(Events.off("noSuchEvent", kept), "unknown event reports false")
    end,
}

suite.cases[#suite.cases + 1] = {
    "once fires exactly once and removes itself",
    function(state)
        local Events = state.Crabe.Events
        local fired = 0

        local wrapper = Events.once("evt", function(value)
            fired = fired + 1
            assertEquals(value, "payload", "once handler argument")
        end)

        assertEquals(#Events._listeners["evt"], 1, "registered")

        Events.emit("evt", "payload")
        assertEquals(fired, 1, "fired on the first emit")
        assertEquals(#Events._listeners["evt"], 0, "self-removed")

        Events.emit("evt", "payload")
        Events.emit("evt", "payload")
        assertEquals(fired, 1, "never fires again")

        -- The wrapper, not the caller's handler, is what was registered: it is
        -- the thing Events.off would need.
        assertEquals(type(wrapper), "function", "once returns the wrapper")
    end,
}

suite.cases[#suite.cases + 1] = {
    "once and on can coexist on the same event",
    function(state)
        local Events = state.Crabe.Events
        local onceCount, onCount = 0, 0

        Events.once("evt", function() onceCount = onceCount + 1 end)
        Events.on("evt", function() onCount = onCount + 1 end)

        Events.emit("evt")
        Events.emit("evt")

        assertEquals(onceCount, 1, "the one-shot fired once")
        assertEquals(onCount, 2, "the persistent listener fired twice")
    end,
}

suite.cases[#suite.cases + 1] = {
    "clear drops one event, or all of them",
    function(state)
        local Events = state.Crabe.Events

        Events.on("a", function() end)
        Events.on("b", function() end)

        Events.clear("a")
        assertNil(Events._listeners["a"], "the named event is cleared")
        assertEquals(#Events._listeners["b"], 1, "the other event survives")

        Events.clear()
        assertNil(Events._listeners["b"], "clearing everything drops the rest")
        assertEquals(next(Events._listeners), nil, "the listener table is empty")
    end,
}

return suite
