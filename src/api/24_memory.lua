-- ===========================================================================
-- Crabe.Memory & Crabe.Hooks - Low-Level Reverse Engineering & Hooking Toolkit
-- ===========================================================================

Crabe = Crabe or {}
Crabe.Memory = Crabe.Memory or {}
Crabe.Hooks = Crabe.Hooks or {}
Crabe.Input = Crabe.Input or {}

local Memory = Crabe.Memory
local Hooks = Crabe.Hooks
local Input = Crabe.Input

--- Scans process memory for an IDA-style byte pattern (e.g. "55 8B EC ?? 8B 45").
--- @param pattern string Byte pattern with hex characters and '?' or '??' wildcards
--- @return number|nil address Address of the first match or nil if not found
function Memory.patternScan(pattern)
    if type(pattern) ~= "string" or pattern == "" then
        error("Crabe.Memory.patternScan: expected a non-empty string pattern", 2)
    end
    if not Crabe._patternScan then return nil end
    return Crabe._patternScan(pattern)
end

--- Patches arbitrary bytes at a specified memory address.
--- @param address number Target virtual address
--- @param hexBytes string Space-separated hex bytes (e.g. "90 90 90")
--- @return boolean success True if patch was successfully applied
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
--- @return number|nil value Float value or nil if address is unreadable
function Memory.readFloat(address)
    if type(address) ~= "number" or address == 0 then return nil end
    if not Crabe._readFloat then return nil end
    return Crabe._readFloat(address)
end

--- Writes a 32-bit floating point number to memory.
--- @param address number Target virtual address
--- @param value number Float value to write
--- @return boolean success True if write succeeded
function Memory.writeFloat(address, value)
    if type(address) ~= "number" or address == 0 then return false end
    if type(value) ~= "number" then return false end
    if not Crabe._writeFloat then return false end
    return Crabe._writeFloat(address, value)
end

--- Reads a 32-bit unsigned integer from memory.
--- @param address number Target virtual address
--- @return number|nil value Integer value or nil if unreadable
function Memory.readU32(address)
    if type(address) ~= "number" or address == 0 then return nil end
    if not Crabe._readU32 then return nil end
    return Crabe._readU32(address)
end

--- Writes a 32-bit unsigned integer to memory.
--- @param address number Target virtual address
--- @param value number Integer value to write
--- @return boolean success True if write succeeded
function Memory.writeU32(address, value)
    if type(address) ~= "number" or address == 0 then return false end
    if type(value) ~= "number" then return false end
    if not Crabe._writeU32 then return false end
    return Crabe._writeU32(address, value)
end

--- Installs an x86 code cave detour.
--- Automatically computes instruction boundaries using HDE32 if stolenLength is 0 or omitted.
--- @param address number Target site to hook
--- @param hexBody string Hex string of payload instructions
--- @param stolenLength number|nil Optional stolen byte length (0 for auto HDE32 calculation)
--- @return boolean success True if cave was installed
function Memory.installCodeCave(address, hexBody, stolenLength)
    if type(address) ~= "number" or address == 0 then return false end
    if type(hexBody) ~= "string" or hexBody == "" then return false end
    if not Crabe._installCodeCave then return false end
    return Crabe._installCodeCave(address, hexBody, stolenLength or 0)
end

--- Registers a Lua patch that executes immediately after a chunk matching `matchSubstring` is executed.
--- @param matchSubstring string Substring identifying the chunk
--- @param luaSource string Lua code to execute
function Hooks.patchChunk(matchSubstring, luaSource)
    if type(matchSubstring) ~= "string" or type(luaSource) ~= "string" then return end
    if Crabe._registerChunkPatch then
        Crabe._registerChunkPatch(matchSubstring, luaSource)
    end
end

--- Registers a Lua patch keyed on the exact loadbuffer chunk name.
--- @param exactChunkName string Exact name of the chunk (e.g. "Presentation/VirtualReaderPC_Data.lua")
--- @param luaSource string Lua code to execute
function Hooks.patchNamedChunk(exactChunkName, luaSource)
    if type(exactChunkName) ~= "string" or type(luaSource) ~= "string" then return end
    if Crabe._registerNamedPatch then
        Crabe._registerNamedPatch(exactChunkName, luaSource)
    end
end

--- Replaces the source of a chunk matching `matchSubstring` before it is compiled.
--- @param matchSubstring string Substring identifying the chunk
--- @param replacementSource string Replacement Lua source code
function Hooks.overrideChunk(matchSubstring, replacementSource)
    if type(matchSubstring) ~= "string" or type(replacementSource) ~= "string" then return end
    if Crabe._registerLoadOverride then
        Crabe._registerLoadOverride(matchSubstring, replacementSource)
    end
end

--- Binds a virtual key to a Lua callback function.
--- @param vk number Windows Virtual Key code (e.g. 0x74 for F5)
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
