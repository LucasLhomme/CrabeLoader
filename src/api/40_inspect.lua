-- Crabe.inspect: read-only inspection of the game's binary, live from Lua.
-- Wraps the C++ introspection natives (src/lua_runtime.cpp) so a question
-- becomes a console one-liner instead of a rebuild.

Crabe.inspect = Crabe.inspect or {}

local inspect = Crabe.inspect

-- Little-endian byte pattern for a 32-bit value, in the "AA BB CC DD" form
-- _scanPattern expects. Written with arithmetic rather than bit ops because
-- this is Lua 5.1 without the bit library.
local function leBytes(value)
    return string.format("%02X %02X %02X %02X",
        value % 256,
        math.floor(value / 256) % 256,
        math.floor(value / 65536) % 256,
        math.floor(value / 16777216) % 256)
end

-- Reads `count` bytes at `address` back as one little-endian integer.
local function readInt(address, count)
    local hex = Crabe._readBytes(address, count or 4)
    if not hex then return nil end

    local value, scale = 0, 1
    for byte in hex:gmatch("%x%x") do
        value = value + tonumber(byte, 16) * scale
        scale = scale * 256
    end
    return value
end

inspect.readInt = readInt

function inspect.base()
    return Crabe._moduleBase()
end

-- Absolute address -> RVA, i.e. the offset a static disassembler shows.
function inspect.rva(address)
    if not address then return nil end
    return address - Crabe._moduleBase()
end

function inspect.bytes(address, count)
    return Crabe._readBytes(address, count or 32)
end

-- Every address a `call rel32` in the first `maxScan` bytes at `address`
-- targets, in order, as a table.
function inspect.calls(address, maxScan)
    local joined = Crabe._findCalls(address, maxScan or 128)
    if not joined then return {} end

    local out = {}
    for value in joined:gmatch("%d+") do
        out[#out + 1] = tonumber(value)
    end
    return out
end

-- Addresses of every `push <address>` (opcode 68) referring to `address`.
-- This is how you get from a string to the code that uses it.
function inspect.xrefs(address, limit)
    local pattern = "68 " .. leBytes(address)
    local out = {}

    local hit = Crabe._scanPattern(pattern)
    while hit and #out < (limit or 16) do
        out[#out + 1] = hit
        hit = Crabe._scanPattern(pattern, hit)
    end
    return out
end

-- Address of a native the *game* registers with its Lua VM. Different lookup
-- from Crabe._findGameNative (stdlib-style {name,fn} table) -- the game emits
-- code per native, function pointer 6 bytes before the push naming it.
function inspect.native(name)
    local stringAddress = Crabe._findString(name)

    while stringAddress do
        for _, hit in ipairs(inspect.xrefs(stringAddress)) do
            -- Only a `mov eax, imm32` immediately before counts; the same
            -- string is also pushed by ordinary call sites.
            if Crabe._readBytes(hit - 7, 1) == "B8" then
                return readInt(hit - 6, 4)
            end
        end
        stringAddress = Crabe._findString(name, stringAddress)
    end
    return nil
end

-- Printable strings of at least `minLength` in [from, to). Strings are
-- packed by subsystem, so dumping around a known name finds its neighbours.
function inspect.strings(from, to, minLength)
    minLength = minLength or 5

    local out = {}
    local address = from

    while address < to do
        local chunkSize = math.min(256, to - address)
        local hex = Crabe._readBytes(address, chunkSize)
        if not hex then break end

        local current, start, offset = "", address, 0
        for byte in hex:gmatch("%x%x") do
            local code = tonumber(byte, 16)
            if code >= 32 and code < 127 then
                if current == "" then start = address + offset end
                current = current .. string.char(code)
            else
                if #current >= minLength then
                    out[#out + 1] = { address = start, text = current }
                end
                current = ""
            end
            offset = offset + 1
        end
        if #current >= minLength then
            out[#out + 1] = { address = start, text = current }
        end

        address = address + chunkSize
    end
    return out
end

-- Convenience: resolve a native and print its address, RVA, and what it calls.
function inspect.dumpNative(name)
    local address = inspect.native(name)
    if not address then
        print("inspect: '" .. name .. "' is not registered by the game")
        return nil
    end

    print(string.format("%s @ 0x%X (RVA 0x%X)", name, address, inspect.rva(address)))
    print("  " .. tostring(inspect.bytes(address, 48)))

    for index, target in ipairs(inspect.calls(address, 96)) do
        print(string.format("  call #%d -> 0x%X (RVA 0x%X)", index, target, inspect.rva(target)))
    end
    return address
end
