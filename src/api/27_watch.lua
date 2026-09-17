-- "Find out what writes to this address", the one tool this loader never had.
--
-- Every cheat here so far was built by scanning byte signatures copied from a
-- community Cheat Engine table and trusting their labels. The velocity sites
-- are what that method costs: all four fire, the multiply runs, and the player
-- moves at exactly the same speed, because nobody ever checked those AOBs
-- describe the store that decides anything.
--
-- The method this replaces it with is the opposite way round. Find a field by
-- watching it change in game, point a hardware breakpoint at it, and let the
-- CPU name the instruction. src/debug_watch.cpp does the trapping.

Crabe = Crabe or {}
Crabe.Watch = Crabe.Watch or {}

local Watch = Crabe.Watch

local function required(name)
    local fn = Crabe[name]
    if type(fn) ~= "function" then
        error("Crabe.Watch: " .. name .. " is missing -- the loader was built without debug_watch.cpp", 3)
    end
    return fn
end

-- Traps the next write to `address`. `size` is 1, 2 or 4 bytes and defaults to
-- 4, a float or a pointer; `readAlso` catches loads too, which is far noisier
-- and only worth it when a write watch has come back empty.
--
-- One address at a time. x86 has four debug registers and sharing them with
-- whatever else might want one is not worth the bookkeeping, so this uses DR0
-- and nothing else.
function Watch.arm(address, size, readAlso)
    if type(address) ~= "number" or address == 0 then
        error("Crabe.Watch.arm: expected an address", 2)
    end

    size = size or 4
    if size ~= 1 and size ~= 2 and size ~= 4 then
        error("Crabe.Watch.arm: size must be 1, 2 or 4", 2)
    end

    -- The hardware cannot express an unaligned watch. It would not fail, it
    -- would silently watch the aligned address below -- a wrong answer that
    -- looks exactly like a right one.
    if address % size ~= 0 then
        error(string.format("Crabe.Watch.arm: 0x%X is not %d-byte aligned", address, size), 2)
    end

    if not required("_watchArm")(address, size, readAlso and true or false) then
        error("Crabe.Watch.arm: no thread accepted the breakpoint", 2)
    end
    return true
end

-- Watches a field of the movement structure, which is the one the position
-- and velocity both live in. Installs the pointer capture if it is not up yet.
function Watch.armMove(offset, size, readAlso)
    if type(offset) ~= "number" then
        error("Crabe.Watch.armMove: expected a numeric offset", 2)
    end

    local object = Crabe.Cheats and Crabe.Cheats.moveObject and Crabe.Cheats.moveObject() or 0
    if object == 0 then
        error("Crabe.Watch.armMove: no movement structure yet -- call " ..
              "Crabe.Cheats.trackPosition() and then walk a few steps", 2)
    end
    return Watch.arm(object + offset, size, readAlso)
end

function Watch.disarm()
    return required("_watchDisarm")() and true or false
end

function Watch.reset()
    return required("_watchReset")() and true or false
end

-- address, hits, armed
function Watch.status()
    local address, count, armed = required("_watchStatus")()
    return address or 0, count or 0, (armed or 0) ~= 0
end

-- Rows sorted by how often each instruction fired, which is the sort that
-- matters: a per-frame writer is the one holding the field down, and a
-- one-shot writer is usually the initialisation nobody wants to patch.
--
-- `rva` is the useful field. ASLR moves the image every run, so the absolute
-- address cannot be compared against a disassembly and the RVA can.
function Watch.hits()
    local hit = required("_watchHit")
    local rows = {}

    for index = 1, 32 do
        local address, rva, count, eax, ecx, edx, esi, edi = hit(index)
        if not address then break end

        rows[#rows + 1] = {
            address = address, rva = rva, count = count,
            eax = eax, ecx = ecx, edx = edx, esi = esi, edi = edi
        }
    end
    return rows
end

function Watch.report()
    return required("_watchReport")()
end

-- What the menu shows. The register dump is here because it answers the
-- question that always comes next: a store through esi tells you which object
-- was written, and whether it was the player or something else entirely.
function Watch.summary()
    local address, count, armed = Watch.status()
    if address == 0 and count == 0 then return "not watching anything" end

    local lines = {}
    lines[#lines + 1] = string.format("%s 0x%X -- %d instruction(s)",
                                      armed and "watching" or "stopped at", address, count)

    for index, row in ipairs(Watch.hits()) do
        if index > 6 then break end
        lines[#lines + 1] = string.format("  rva 0x%X  x%d  eax=0x%X esi=0x%X",
                                          row.rva, row.count, row.eax, row.esi)
    end

    if #lines == 1 then
        lines[#lines + 1] = "  nothing has touched it yet -- move, or take a hit"
    end
    return table.concat(lines, "\n")
end

-- The store that reported a trap is the instruction *ending* at `rva`: a data
-- breakpoint fires once the write has retired. Disassembling backwards from
-- there is the only way to find where the opcode starts, so dump the bytes
-- before it and read them by hand.
function Watch.bytesBefore(rva, count)
    count = count or 24
    local base = Crabe._moduleBase and Crabe._moduleBase() or 0
    if base == 0 then error("Crabe.Watch.bytesBefore: no module base", 2) end

    return Crabe._readBytes(base + rva - count, count)
end
