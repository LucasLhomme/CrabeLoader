-- `Crabe` is the modloader namespace, `Game` the game API.
-- Loaded first: the other modules report their errors through Crabe.write.

Crabe = Crabe or {}
Crabe.version = Crabe.version or "0.2.0"
Crabe.versionMajor = Crabe.versionMajor or 0
Crabe.versionMinor = Crabe.versionMinor or 2
Crabe.versionPatch = Crabe.versionPatch or 0

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

-- Window Mode (merged from 05_window.lua)
Crabe._windowMode = "windowed"

function Crabe.SetWindowMode(mode)
    if mode ~= "windowed" and mode ~= "borderless" then
        error("Crabe.SetWindowMode: expected 'windowed' or 'borderless', got '" .. tostring(mode) .. "'", 2)
    end

    Crabe._setWindowModeNative(mode)
    Crabe._windowMode = mode
    return true
end

function Crabe.GetWindowMode()
    return Crabe._windowMode
end

-- Generic helpers
function Crabe.native(name, caller)
    local fn = rawget(_G, name)
    if type(fn) ~= "function" then
        error((caller or "Crabe.native") .. ": game engine native '" .. name .. "' is not present in this Lua state", 3)
    end
    return fn
end

function Crabe.hostPlayer(playerId)
    return playerId or (type(Players_GetHostPlayerID) == "function" and Players_GetHostPlayerID() or 0)
end

function Crabe.splitList(csv)
    local out = {}
    if type(csv) ~= "string" then return out end

    for item in string.gmatch(csv, "([^,]+)") do
        local trimmed = string.gsub(item, "^%s*(.-)%s*$", "%1")
        if trimmed ~= "" then out[#out + 1] = trimmed end
    end
    return out
end
