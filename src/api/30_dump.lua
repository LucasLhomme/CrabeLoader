-- TEMPORARY RE tool feeding docs/nativedb.md; delete once it's written.
-- Crabe.dump.all()/.natives()/.find(pattern). In-game only -- walking _G
-- before the engine finishes building its tables crashes the game.

Crabe.dump = Crabe.dump or {}

local DEFAULT_DEPTH = 2

local function sortedKeys(tbl)
    local keys = {}
    for key in pairs(tbl) do
        keys[#keys + 1] = key
    end
    table.sort(keys, function(a, b) return tostring(a) < tostring(b) end)
    return keys
end

-- "C" = engine native, "Lua" = game script (with file:line).
local function describeFunction(fn)
    if not (debug and debug.getinfo) then return "?", "" end

    local ok, info = pcall(debug.getinfo, fn, "Su")
    if not ok or type(info) ~= "table" then return "?", "" end
    if info.what == "C" then return "C", "" end

    local extra = tostring(info.source or "?") .. ":" .. tostring(info.linedefined or 0)
    if info.nups and info.nups > 0 then
        extra = extra .. " (" .. info.nups .. " upvalues)"
    end
    return "Lua", extra
end

local function describeScalar(value)
    if type(value) ~= "string" then return tostring(value) end

    local text = string.gsub(value, "[\r\n\t]", " ")
    if #text > 120 then text = string.sub(text, 1, 120) .. "..." end
    return '"' .. text .. '"'
end

local function emit(out, kind, detail, name, extra)
    local line = string.format("%-9s %-4s %-44s %s", kind, detail or "", name, extra or "")
    out[#out + 1] = (string.gsub(line, "%s+$", ""))
end

-- `seen` breaks cycles (_G._G, package.loaded._G) and reports aliases.
local function walk(out, tbl, prefix, depth, seen, stats)
    local keys = sortedKeys(tbl)

    for i = 1, #keys do
        local key = keys[i]
        local value = rawget(tbl, key)
        local name = prefix .. tostring(key)
        local kind = type(value)

        if kind == "function" then
            local origin, extra = describeFunction(value)
            local bucket = (origin == "C") and "natives" or "luaFunctions"
            stats[bucket] = stats[bucket] + 1
            emit(out, "function", origin, name, extra)

        elseif kind == "table" then
            if seen[value] then
                emit(out, "table", "", name, "-> " .. seen[value])
            else
                seen[value] = name
                stats.tables = stats.tables + 1

                local count = 0
                for _ in pairs(value) do count = count + 1 end
                emit(out, "table", "", name, "(" .. count .. " fields)")

                if depth > 0 then
                    walk(out, value, name .. ".", depth - 1, seen, stats)
                end
            end

        elseif kind == "userdata" then
            local ok, meta = pcall(getmetatable, value)
            stats.userdata = stats.userdata + 1
            emit(out, "userdata", "", name, (ok and meta) and "(has a metatable)" or "")

        else
            stats.values = stats.values + 1
            emit(out, kind, "", name, "= " .. describeScalar(value))
        end
    end
end

local function writeReport(path, lines)
    local file, err = io.open(path, "w")
    if not file then
        error("Crabe.dump: cannot write '" .. tostring(path) .. "': " .. tostring(err), 3)
    end
    file:write(table.concat(lines, "\n"), "\n")
    file:close()
end

function Crabe.dump.all(path, depth)
    path = path or "crabe_dump.txt"
    depth = depth or DEFAULT_DEPTH

    local out = {}
    local stats = { natives = 0, luaFunctions = 0, tables = 0, userdata = 0, values = 0 }
    local seen = { [_G] = "_G" }

    out[#out + 1] = "CrabeLoader reflection dump (runtime " .. tostring(Crabe.version) .. ")"
    if os and os.date then
        out[#out + 1] = "generated " .. tostring(os.date())
    end
    out[#out + 1] = "walk depth: " .. depth
    out[#out + 1] = ""

    out[#out + 1] = "=== GLOBALS (_G) ==="
    walk(out, _G, "", depth, seen, stats)

    if package and type(package.loaded) == "table" then
        out[#out + 1] = ""
        out[#out + 1] = "=== package.loaded ==="
        walk(out, package.loaded, "loaded.", 1, seen, stats)
    end

    -- Userdata metatables: the only class list this game exposes.
    if debug and debug.getregistry then
        local ok, registry = pcall(debug.getregistry)
        if ok and type(registry) == "table" then
            out[#out + 1] = ""
            out[#out + 1] = "=== registry (userdata metatables, refs) ==="
            walk(out, registry, "registry.", 1, seen, stats)
        end
    end

    out[#out + 1] = ""
    out[#out + 1] = string.format(
        "=== TOTALS: %d engine natives, %d Lua functions, %d tables, %d userdata, %d values ===",
        stats.natives, stats.luaFunctions, stats.tables, stats.userdata, stats.values)

    writeReport(path, out)
    return stats.natives .. " natives, " .. #out .. " lines -> " .. path
end

function Crabe.dump.natives(path)
    path = path or "crabe_natives.txt"

    local names = {}
    for key, value in pairs(_G) do
        if type(key) == "string" and type(value) == "function" then
            if describeFunction(value) ~= "Lua" then names[#names + 1] = key end
        end
    end
    table.sort(names)

    writeReport(path, names)
    return #names .. " natives -> " .. path
end

function Crabe.dump.find(pattern)
    if type(pattern) ~= "string" then
        error("Crabe.dump.find: expected a string pattern", 2)
    end

    local needle = string.lower(pattern)
    local matches = {}

    for key, value in pairs(_G) do
        if type(key) == "string" and string.find(string.lower(key), needle) then
            matches[#matches + 1] = key .. "  (" .. type(value) .. ")"
        end
    end
    table.sort(matches)

    for i = 1, #matches do
        print("  " .. matches[i])
    end
    return #matches .. " match(es) for '" .. pattern .. "'"
end
