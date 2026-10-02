local harness = ...

local assertEquals = harness.assertEquals
local assertTrue = harness.assertTrue
local assertFalse = harness.assertFalse
local assertNil = harness.assertNil
local assertNotNil = harness.assertNotNil

local suite = { name = "Crabe.Input gamepad", cases = {} }

local function fakePad(state, connected, buttons, lt, rt, lx, ly, rx, ry)
    local calls = {}
    state.Crabe._padState = function(slot)
        calls[#calls + 1] = slot
        return connected, buttons, lt, rt, lx, ly, rx, ry
    end
    return calls
end

suite.cases[#suite.cases + 1] = {
    "PAD holds the XInput button bits",
    function(state)
        local PAD = state.Crabe.Input.PAD
        assertEquals(PAD.DPAD_UP, 0x0001)
        assertEquals(PAD.DPAD_DOWN, 0x0002)
        assertEquals(PAD.DPAD_LEFT, 0x0004)
        assertEquals(PAD.DPAD_RIGHT, 0x0008)
        assertEquals(PAD.START, 0x0010)
        assertEquals(PAD.BACK, 0x0020)
        assertEquals(PAD.LB, 0x0100)
        assertEquals(PAD.RB, 0x0200)
        assertEquals(PAD.A, 0x1000)
        assertEquals(PAD.B, 0x2000)
        assertEquals(PAD.X, 0x4000)
        assertEquals(PAD.Y, 0x8000)
    end,
}

suite.cases[#suite.cases + 1] = {
    "padState is nil when the loader has no pad native",
    function(state)
        state.Crabe._padState = nil
        assertNil(state.Crabe.Input.padState(0))
    end,
}

suite.cases[#suite.cases + 1] = {
    "padState is nil while the slot is disconnected",
    function(state)
        fakePad(state, false, 0, 0, 0, 0, 0, 0, 0)
        assertNil(state.Crabe.Input.padState(0))
    end,
}

suite.cases[#suite.cases + 1] = {
    "padState maps the native's eight values and defaults to slot 0",
    function(state)
        local calls = fakePad(state, true, 0x1204, 255, 7, -32768, 32767, -1, 12)
        local pad = state.Crabe.Input.padState()
        assertNotNil(pad)
        assertEquals(calls[1], 0, "slot")
        assertEquals(pad.buttons, 0x1204)
        assertEquals(pad.leftTrigger, 255)
        assertEquals(pad.rightTrigger, 7)
        assertEquals(pad.leftX, -32768)
        assertEquals(pad.leftY, 32767)
        assertEquals(pad.rightX, -1)
        assertEquals(pad.rightY, 12)
    end,
}

suite.cases[#suite.cases + 1] = {
    "padState passes the requested slot through",
    function(state)
        local calls = fakePad(state, true, 0, 0, 0, 0, 0, 0, 0)
        state.Crabe.Input.padState(3)
        assertEquals(calls[1], 3)
    end,
}

suite.cases[#suite.cases + 1] = {
    "padHas tests one bit at a time, the top bit included",
    function(state)
        local Input = state.Crabe.Input
        local held = Input.PAD.RB + Input.PAD.DPAD_LEFT + Input.PAD.Y
        assertTrue(Input.padHas(held, Input.PAD.RB))
        assertTrue(Input.padHas(held, Input.PAD.DPAD_LEFT))
        assertTrue(Input.padHas(held, Input.PAD.Y))
        assertFalse(Input.padHas(held, Input.PAD.LB))
        assertFalse(Input.padHas(held, Input.PAD.DPAD_RIGHT))
        assertFalse(Input.padHas(held, Input.PAD.A))
        assertFalse(Input.padHas(nil, Input.PAD.A), "a missing bitmask holds nothing")
    end,
}

suite.cases[#suite.cases + 1] = {
    "capturePad forwards a strict boolean and is a no-op without the native",
    function(state)
        local seen = {}
        state.Crabe._setPadCaptured = function(on) seen[#seen + 1] = on end

        state.Crabe.Input.capturePad(true)
        state.Crabe.Input.capturePad(false)
        state.Crabe.Input.capturePad(1)
        state.Crabe.Input.capturePad(nil)
        assertEquals(#seen, 4)
        assertEquals(seen[1], true)
        assertEquals(seen[2], false)
        assertEquals(seen[3], false, "only true captures")
        assertEquals(seen[4], false)

        state.Crabe._setPadCaptured = nil
        state.Crabe.Input.capturePad(true)
    end,
}

return suite
