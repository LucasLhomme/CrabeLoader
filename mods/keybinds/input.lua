local Input = {}

local VK_BACK, VK_SHIFT, VK_CONTROL, VK_MENU, VK_ESCAPE = 0x08, 0x10, 0x11, 0x12, 0x1B

local MOUSE_BY_VK = { [0x01] = 0, [0x02] = 1, [0x04] = 2, [0x05] = 3, [0x06] = 4 }
local GENERIC_MODIFIERS = { [VK_SHIFT] = true, [VK_CONTROL] = true, [VK_MENU] = true }
local NOT_CAPTURED = { [VK_SHIFT] = true, [VK_CONTROL] = true, [VK_MENU] = true, [VK_ESCAPE] = true, [VK_BACK] = true }

local PAD = Crabe.Input.PAD
local PAD_BUTTONS = {
    PAD.DPAD_UP, PAD.DPAD_DOWN, PAD.DPAD_LEFT, PAD.DPAD_RIGHT, PAD.START, PAD.BACK,
    PAD.LEFT_THUMB, PAD.RIGHT_THUMB, PAD.LB, PAD.RB, PAD.A, PAD.B, PAD.X, PAD.Y,
}

local edges = {}
local wheelNotches = 0

Input.CAPTURE_KEYS = {}
for vk = 8, 254 do Input.CAPTURE_KEYS[#Input.CAPTURE_KEYS + 1] = vk end

-- Reads the first controller slot, or nil when no controller is connected.
local function readPad()
    return Crabe.Input.padState(0)
end

-- Reports whether one button is held in a controller reading that may be nil.
local function padButton(pad, mask)
    return pad ~= nil and Crabe.Input.padHas(pad.buttons, mask)
end

-- Reports a press only on the tick an input goes down, remembering its state under `name`.
local function rising(name, down)
    local pressed = down and not edges[name]
    edges[name] = down
    return pressed
end

-- Names a virtual key the way the remap names it: a 0-based mouse button or a DirectInput scan code.
-- Returns nothing for a key that has no scan code.
local function physical(vk)
    if MOUSE_BY_VK[vk] then return "mouse", MOUSE_BY_VK[vk] end
    local scan = Crabe.Input.scanCode(vk)
    if scan ~= 0 then return "keyboard", scan end
end

--- Marks every key and button held right now as already down, so only a fresh press is captured.
function Input.primeCapture()
    for vk = 1, 254 do edges[vk] = Crabe.Input.isKeyDown(vk) end
    local pad = readPad()
    for _, mask in ipairs(PAD_BUTTONS) do edges["pad" .. mask] = padButton(pad, mask) end
    edges.cancel = Crabe.Input.isKeyDown(VK_ESCAPE) or Crabe.Input.isKeyDown(VK_BACK)
end

--- Reports whether Escape or Backspace was just pressed; neither can be captured.
function Input.captureCancelled()
    return rising("cancel", Crabe.Input.isKeyDown(VK_ESCAPE) or Crabe.Input.isKeyDown(VK_BACK))
end

--- Lists every key, mouse button and pad button held right now as { device, code, isDown }.
--- `isDown()` asks whether that input is still held.
function Input.heldInputs()
    local held = {}
    for vk = 1, 254 do
        if not GENERIC_MODIFIERS[vk] and Crabe.Input.isKeyDown(vk) then
            local device, code = physical(vk)
            if device then
                held[#held + 1] = { device = device, code = code, isDown = function() return Crabe.Input.isKeyDown(vk) end }
            end
        end
    end
    local pad = readPad()
    for _, mask in ipairs(PAD_BUTTONS) do
        if padButton(pad, mask) then
            held[#held + 1] = { device = "pad", code = mask, isDown = function() return padButton(readPad(), mask) end }
        end
    end
    return held
end

--- Returns the first key, mouse button or pad button newly pressed as { device, code }, or nil.
--- A key's code is its DirectInput scan code, a mouse button's is 0-based and a pad button's is its XInput bit.
function Input.pollCapture()
    local found = nil
    for vk = 1, 254 do
        local pressed = (not NOT_CAPTURED[vk]) and rising(vk, Crabe.Input.isKeyDown(vk))
        if pressed and not found then
            local device, code = physical(vk)
            if device then found = { device = device, code = code } end
        end
    end
    local pad = readPad()
    for _, mask in ipairs(PAD_BUTTONS) do
        if rising("pad" .. mask, padButton(pad, mask)) and not found then found = { device = "pad", code = mask } end
    end
    return found
end

--- Reads the mouse wheel once for this tick; call it first thing in the tick.
--- A notch nobody takes is dropped at the next call, so a page opened later does not scroll by it.
function Input.pollWheel()
    wheelNotches = Crabe.Input.wheel()
end

--- Returns the wheel notches of this tick and keeps them from anyone else: positive is up, negative down.
function Input.takeWheel()
    local notches = wheelNotches
    wheelNotches = 0
    return notches
end

return Input
