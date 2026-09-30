local harness = ...

local assertEquals = harness.assertEquals
local assertNil = harness.assertNil
local assertTrue = harness.assertTrue
local assertFalse = harness.assertFalse
local assertContains = harness.assertContains

local function noop() end

local suite = { name = "Crabe.Registry", cases = {} }

suite.cases[#suite.cases + 1] = {
    "track attributes a subscription to the current owner",
    function(state)
        local Registry = state.Crabe.Registry
        local base = Registry.count()

        Registry.setCurrentOwner("modA")
        Registry.track(noop)
        Registry.track(noop)
        Registry.setCurrentOwner(nil)

        assertEquals(Registry.count(), base + 2, "count after two tracks")
        assertEquals(#Registry._byOwner["modA"], 2, "modA bucket")
        assertNil(Registry._current, "owner cleared")
    end,
}

suite.cases[#suite.cases + 1] = {
    "track returns the revoke it was given",
    function(state)
        local Registry = state.Crabe.Registry
        local revoke = function() end
        assertEquals(Registry.track(revoke), revoke, "track return value")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a subscription made with no owner falls to core",
    function(state)
        local Registry = state.Crabe.Registry
        assertNil(Registry._current, "no mod is executing at injection time")

        local baseCore = #Registry._byOwner["core"]
        Registry.track(noop)

        assertEquals(#Registry._byOwner["core"], baseCore + 1, "core bucket")
    end,
}

suite.cases[#suite.cases + 1] = {
    "track refuses anything that is not a function",
    function(state)
        local Registry = state.Crabe.Registry

        for _, bad in ipairs({ "revoke", 42, {} }) do
            local ok, err = pcall(Registry.track, bad)
            assertFalse(ok, "track(" .. type(bad) .. ") must raise")
            assertContains(tostring(err), "expected a function", "track error message")
        end
        local ok = pcall(Registry.track, nil)
        assertFalse(ok, "track(nil) must raise")
    end,
}

suite.cases[#suite.cases + 1] = {
    "revokeAllMods runs every mod revoke exactly once and empties the buckets",
    function(state)
        local Registry = state.Crabe.Registry
        local calls = {}

        Registry.setCurrentOwner("modA")
        Registry.track(function() calls[#calls + 1] = "a1" end)
        Registry.track(function() calls[#calls + 1] = "a2" end)
        Registry.setCurrentOwner("modB")
        Registry.track(function() calls[#calls + 1] = "b1" end)
        Registry.setCurrentOwner(nil)

        local baseCore = #Registry._byOwner["core"]

        Registry.revokeAllMods()

        assertEquals(#calls, 3, "revoke call count")
        assertNil(Registry._byOwner["modA"], "modA bucket dropped")
        assertNil(Registry._byOwner["modB"], "modB bucket dropped")
        assertEquals(#Registry._byOwner["core"], baseCore, "core bucket untouched")
        assertEquals(Registry.count(), baseCore, "only core subscriptions remain")
    end,
}

suite.cases[#suite.cases + 1] = {
    "revokeAllMods never revokes a core subscription",
    function(state)
        local Registry = state.Crabe.Registry
        local coreRevoked = false

        Registry.track(function() coreRevoked = true end)
        local coreBucket = #Registry._byOwner["core"]

        Registry.revokeAllMods()

        assertFalse(coreRevoked, "a core revoke must not run")
        assertEquals(#Registry._byOwner["core"], coreBucket, "core bucket size")
    end,
}

suite.cases[#suite.cases + 1] = {
    "revokeAllMods is idempotent",
    function(state)
        local Registry = state.Crabe.Registry
        local calls = 0

        Registry.setCurrentOwner("modA")
        Registry.track(function() calls = calls + 1 end)
        Registry.setCurrentOwner(nil)

        Registry.revokeAllMods()
        Registry.revokeAllMods()
        Registry.revokeAllMods()

        assertEquals(calls, 1, "a revoked subscription is not revoked again")
    end,
}

suite.cases[#suite.cases + 1] = {
    "count sums every bucket",
    function(state)
        local Registry = state.Crabe.Registry
        local base = Registry.count()

        Registry.setCurrentOwner("modA")
        Registry.track(noop)
        Registry.setCurrentOwner("modB")
        Registry.track(noop)
        Registry.track(noop)
        Registry.setCurrentOwner(nil)
        Registry.track(noop)

        assertEquals(Registry.count(), base + 4, "count across core, modA and modB")
        assertTrue(Registry.count() > 0, "the API's own subscriptions are counted")
    end,
}

return suite
