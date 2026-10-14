-- CrabeLoader
-- File description:
-- The low-level tables: Memory for scans and patches, Hooks for chunk patching, Input for keys.
-- Everything here writes to the running process, so a wrong address corrupts the game silently.
-- Resolves no address for you; a pattern that matches nothing returns nil rather than guessing.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Memory = Crabe.Memory or {}
Crabe.Hooks = Crabe.Hooks or {}
Crabe.Input = Crabe.Input or {}

local Memory = Crabe.Memory
local Hooks = Crabe.Hooks
local Input = Crabe.Input

--- Scans process memory for an IDA-style byte pattern.
--- @param pattern string Byte pattern with wildcards
--- @return number|nil address Match address or nil
function Memory.patternScan(pattern)
    if type(pattern) ~= "string" or pattern == "" then
        error("Crabe.Memory.patternScan: expected a non-empty string pattern", 2)
    end
    if not Crabe._patternScan then return nil end
    return Crabe._patternScan(pattern)
end

--- Patches arbitrary bytes at a specified memory address.
--- @param address number Target virtual address
--- @return boolean success True if patch was applied
function Memory.patchBytes(address, hexBytes)
    if type(address) ~= "number" or address == 0 then
        error("Crabe.Memory.patchBytes: expected a valid non-zero address", 2)
    end
    if type(hexBytes) ~= "string" or hexBytes == "" then
        error("Crabe.Memory.patchBytes: expected hex string", 2)
    end
    if not Crabe._patchBytes then return false end
    return Crabe._patchBytes(address, hexBytes)
end

--- Reads a 32-bit floating point number from memory.
--- @param address number Target virtual address
--- @return number|nil value Float value or nil
function Memory.readFloat(address)
    if type(address) ~= "number" or address == 0 then return nil end
    if not Crabe._readFloat then return nil end
    return Crabe._readFloat(address)
end

--- Writes a 32-bit floating point number to memory.
--- @param address number Target virtual address
--- @return boolean success True if write succeeded
function Memory.writeFloat(address, value)
    if type(address) ~= "number" or address == 0 then return false end
    if type(value) ~= "number" then return false end
    if not Crabe._writeFloat then return false end
    return Crabe._writeFloat(address, value)
end

--- Reads a 32-bit unsigned integer from memory.
--- @param address number Target virtual address
--- @return number|nil value Integer value or nil
function Memory.readU32(address)
    if type(address) ~= "number" or address == 0 then return nil end
    if not Crabe._readU32 then return nil end
    return Crabe._readU32(address)
end

--- Writes a 32-bit unsigned integer to memory.
--- @param address number Target virtual address
--- @return boolean success True if write succeeded
function Memory.writeU32(address, value)
    if type(address) ~= "number" or address == 0 then return false end
    if type(value) ~= "number" then return false end
    if not Crabe._writeU32 then return false end
    return Crabe._writeU32(address, value)
end

--- Installs an x86 code cave with auto HDE32 calculation.
--- @param address number Target site to hook
--- @return boolean success True if cave was installed
function Memory.installCodeCave(address, hexBody, stolenLength)
    if type(address) ~= "number" or address == 0 then return false end
    if type(hexBody) ~= "string" or hexBody == "" then return false end
    if not Crabe._installCodeCave then return false end
    return Crabe._installCodeCave(address, hexBody, stolenLength or 0)
end

--- Returns a zeroed data block shared with code caves; the same name returns the same block after a hot reload.
--- @param name string Unique block name, e.g. "mymod.godmode"
--- @param size number Size in bytes
--- @return number|nil address Base address, or nil if the size exceeds the existing block
function Memory.sharedBlock(name, size)
    if type(name) ~= "string" or name == "" then return nil end
    if type(size) ~= "number" or size <= 0 then return nil end
    if not Crabe._sharedBlock then return nil end
    return Crabe._sharedBlock(name, size)
end

--- Registers a Lua patch executing after a matching chunk.
--- @param matchSubstring string Substring identifying the chunk
--- @param luaSource string Lua code to execute
function Hooks.patchChunk(matchSubstring, luaSource)
    if type(matchSubstring) ~= "string" or type(luaSource) ~= "string" then return end
    if Crabe._registerChunkPatch then
        Crabe._registerChunkPatch(matchSubstring, luaSource)
    end
end

--- Registers a Lua patch keyed on exact loadbuffer chunk name.
--- @param exactChunkName string Exact name of the chunk
--- @param luaSource string Lua code to execute
function Hooks.patchNamedChunk(exactChunkName, luaSource)
    if type(exactChunkName) ~= "string" or type(luaSource) ~= "string" then return end
    if Crabe._registerNamedPatch then
        Crabe._registerNamedPatch(exactChunkName, luaSource)
    end
end

--- Replaces chunk source before compilation.
--- @param matchSubstring string Substring identifying the chunk
--- @param replacementSource string Replacement Lua source code
function Hooks.overrideChunk(matchSubstring, replacementSource)
    if type(matchSubstring) ~= "string" or type(replacementSource) ~= "string" then return end
    if Crabe._registerLoadOverride then
        Crabe._registerLoadOverride(matchSubstring, replacementSource)
    end
end

--- Stops the game from seeing the given keys, so a mod can own them while a
--- list or prompt is up. Call with nil or an empty table to release them.
--- @param keys table|nil Array of Windows Virtual Key codes
function Input.captureKeys(keys)
    if not Crabe._setCapturedKeys then return end
    if type(keys) ~= "table" then
        Crabe._setCapturedKeys()
        return
    end
    Crabe._setCapturedKeys(unpack(keys))
end

--- XINPUT_GAMEPAD_* bits, for testing Crabe.Input.padState().buttons with Input.padHas.
Input.PAD = {
    DPAD_UP = 0x0001, DPAD_DOWN = 0x0002, DPAD_LEFT = 0x0004, DPAD_RIGHT = 0x0008,
    START = 0x0010, BACK = 0x0020, LEFT_THUMB = 0x0040, RIGHT_THUMB = 0x0080,
    LB = 0x0100, RB = 0x0200, A = 0x1000, B = 0x2000, X = 0x4000, Y = 0x8000,
}

--- Returns what the game last read from a controller slot (0-3), or nil when
--- that slot is not connected. Sticks run -32768..32767, triggers 0..255.
--- @param slot number|nil Controller slot, 0 by default
function Input.padState(slot)
    if not Crabe._padState then return nil end
    local connected, buttons, lt, rt, lx, ly, rx, ry = Crabe._padState(slot or 0)
    if not connected then return nil end
    return { buttons = buttons, leftTrigger = lt, rightTrigger = rt, leftX = lx, leftY = ly, rightX = rx, rightY = ry }
end

--- Reports whether one button is held in a padState().buttons value.
--- @param buttons number Button bitmask
--- @param mask number A single Input.PAD bit
function Input.padHas(buttons, mask)
    return math.floor((buttons or 0) / mask) % 2 == 1
end

--- Reports whether a key or mouse button is held right now, read from Windows, not from what the game sees.
--- @param vk number Windows Virtual Key code
--- @return boolean down
function Input.isKeyDown(vk)
    if type(vk) ~= "number" or not Crabe._keyDown then return false end
    return Crabe._keyDown(vk) == 1
end

--- Hands the game an idle gamepad while on, so a mod can own the controller.
--- padState keeps reading the real pad. Call with false to give it back.
--- @param captured boolean
function Input.capturePad(captured)
    if Crabe._setPadCaptured then Crabe._setPadCaptured(captured == true) end
end

--- Devices an input remap can target: Input.remapKey, remapMouseButton and remapPadButton wrap them.
Input.DEVICE = { KEYBOARD = 0, MOUSE = 1, PAD = 2 }

--- Makes the game see one physical key as another. Mods keep reading the real keys.
--- Both are DirectInput scan codes (see Input.scanCode); a `to` of 0 swallows the key.
--- Returns false, and changes nothing, on a bad code or when this loader predates remapping.
--- @param from number Scan code of the key the player presses
--- @param to number Scan code the game should receive, or 0 to swallow
--- @return boolean stored
function Input.remapKey(from, to)
    if type(from) ~= "number" or type(to) ~= "number" then return false end
    if not Crabe._inputRemap then return false end
    return Crabe._inputRemap(Input.DEVICE.KEYBOARD, from, to)
end

--- Makes the game see one mouse button as another (0-based: 0 left, 1 right, 2 middle, 3 and 4 the side buttons).
--- A negative `to` swallows the button. Only buttons are remapped; movement and the wheel pass through.
--- @param from number Button the player presses
--- @param to number Button the game should receive, or -1 to swallow
--- @return boolean stored
function Input.remapMouseButton(from, to)
    if type(from) ~= "number" or type(to) ~= "number" then return false end
    if not Crabe._inputRemap then return false end
    return Crabe._inputRemap(Input.DEVICE.MOUSE, from, to)
end

--- Makes the game see one pad button as another. Both are single Input.PAD bits; a `to` of 0 swallows the button.
--- Sticks and triggers are not buttons and pass through unchanged.
--- @param from number Input.PAD bit the player presses
--- @param to number Input.PAD bit the game should receive, or 0 to swallow
--- @return boolean stored
function Input.remapPadButton(from, to)
    if type(from) ~= "number" or type(to) ~= "number" then return false end
    if not Crabe._inputRemap then return false end
    return Crabe._inputRemap(Input.DEVICE.PAD, from, to)
end

--- Removes the remaps of one device, or of all of them when `device` is omitted.
--- @param device number|nil An Input.DEVICE value
function Input.clearRemap(device)
    if not Crabe._inputRemapClear then return end
    Crabe._inputRemapClear(device)
end

--- Returns the whole mouse wheel notches the game has read since the last call: positive up, negative down, 0 when
--- the wheel did not move or this loader predates it. Call it once per tick: each call hands the notches out once.
--- @return number notches
function Input.wheel()
    if not Crabe._mouseWheel then return 0 end
    return Crabe._mouseWheel() or 0
end

--- Returns the DirectInput scan code of a Windows virtual key, or 0 when it has none (mouse buttons, say).
--- @param vk number Windows Virtual Key code
--- @return number scanCode
function Input.scanCode(vk)
    if type(vk) ~= "number" or not Crabe._keyScanCode then return 0 end
    return Crabe._keyScanCode(vk)
end

--- Returns a key's name on the player's own keyboard layout ("Z" where a QWERTY keyboard says "W"), or "".
--- @param scanCode number DirectInput scan code
--- @return string name
function Input.keyName(scanCode)
    if type(scanCode) ~= "number" or not Crabe._keyName then return "" end
    return Crabe._keyName(scanCode)
end

--- Binds a virtual key to a Lua callback function.
--- @param vk number Windows Virtual Key code
--- @param callback function Function to invoke on key down
function Input.bindKey(vk, callback)
    if type(vk) ~= "number" or type(callback) ~= "function" then return end
    if Crabe.Events and Crabe.Events.on then
        Crabe.Events.on("keyDown", function(key, isRepeat)
            if key == vk and not isRepeat then
                callback()
            end
        end)
    end
end
