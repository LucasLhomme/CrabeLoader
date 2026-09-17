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

--- Binds a virtual key to a Lua callback function.
--- @param vk number Windows Virtual Key code
--- @param callback function Function to invoke on key down
function Input.bindKey(vk, callback)
    if type(vk) ~= "number" or type(callback) ~= "function" then return end
    if Crabe.Events and Crabe.Events.on then
        Crabe.Events.on("keyDown", function(key)
            if key == vk then
                callback()
            end
        end)
    end
end
