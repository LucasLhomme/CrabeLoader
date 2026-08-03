-- `Crabe` is the modloader namespace, `Game` the game API.
-- Loaded first: the other modules report their errors through Crabe.write.

Crabe = Crabe or {}
Crabe.version = "0.1.0"

Crabe._lines = {}
Crabe._maxLines = 200

function Crabe.write(line)
    local lines = Crabe._lines
    lines[#lines + 1] = tostring(line)

    while #lines > Crabe._maxLines do
        table.remove(lines, 1)
    end
end

function Crabe.flush()
    local lines = Crabe._lines
    if #lines == 0 then return "" end

    local joined = table.concat(lines, "\n")
    Crabe._lines = {}
    return joined
end

-- print is redirected to the overlay console. Guarded: the runtime is loaded
-- once per lua_State, and chaining the wrapper onto itself would double every
-- line.
if not Crabe._printHooked then
    Crabe._printHooked = true
    local originalPrint = print

    print = function(...)
        local parts = {}
        for i = 1, select("#", ...) do
            parts[i] = tostring((select(i, ...)))
        end
        Crabe.write(table.concat(parts, "\t"))

        if originalPrint then originalPrint(...) end
    end
end
