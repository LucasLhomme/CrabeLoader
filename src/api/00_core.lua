-- CrabeLoader
-- File description:
-- Creates the Crabe and Game namespaces and the write path every other module reports through.
-- Loaded first by tools/embed_api.py, so nothing here may depend on a later module existing.
-- Declares no gameplay call; those start at src/api/10_game.lua.
--
-- Authors: @LucasLhomme

-- `Crabe` is the modloader namespace, `Game` the game API.
-- Loaded first: the other modules report their errors through Crabe.write.

Crabe = Crabe or {}
Crabe.version = "1.0.0"
Crabe.versionMajor = 1
Crabe.versionMinor = 0
Crabe.versionPatch = 0
Crabe.version = Crabe.version or "1.0.0"
Crabe.versionMajor = Crabe.versionMajor or 1
Crabe.versionMinor = Crabe.versionMinor or 0
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

Crabe._windowMode = "borderless"

-- Sets the window display mode to borderless or windowed.
function Crabe.SetWindowMode(mode)
    if mode ~= "windowed" and mode ~= "borderless" then
        error("Crabe.SetWindowMode: expected 'windowed' or 'borderless', got '" .. tostring(mode) .. "'", 2)
    end

    Crabe._setWindowModeNative(mode)
    Crabe._windowMode = mode
    return true
end

-- Returns the active window display mode from native state.
function Crabe.GetWindowMode()
    if type(Crabe._getWindowModeNative) == "function" then
        return Crabe._getWindowModeNative()
    end
    return Crabe._windowMode
end

-- Caps the game's frame rate; 0 (or nil) removes the cap. The game never caps
-- itself on PC, so this is the only limiter. Saved to crabe.toml and applied
-- again on the next launch. Values below 15 or above 1000 are clamped.
function Crabe.SetFrameLimit(fps)
    fps = tonumber(fps) or 0
    if fps < 0 then
        error("Crabe.SetFrameLimit: expected 0 (unlimited) or a positive FPS, got " .. tostring(fps), 2)
    end
    if type(Crabe._setFrameLimitNative) ~= "function" then
        error("Crabe.SetFrameLimit: the frame limiter is not available in this Lua state", 2)
    end
    Crabe._setFrameLimitNative(math.floor(fps))
    return Crabe.GetFrameLimit()
end

-- The current frame cap, 0 when unlimited.
function Crabe.GetFrameLimit()
    if type(Crabe._getFrameLimitNative) == "function" then
        return Crabe._getFrameLimitNative() or 0
    end
    return 0
end

-- Frames the game actually presents per second, measured at Present. Unlike a
-- tick counter it is not capped by the loader's 60 Hz Lua tick. 0 until the
-- first half second of rendering has been measured.
function Crabe.GetRenderFps()
    if type(Crabe._getRenderFpsNative) == "function" then
        return Crabe._getRenderFpsNative() or 0
    end
    return 0
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

-- Disk module loader for require(): enables loading submodules from disk (mods/, mods/disneyinfinitymp/, etc.)
if package and type(package.loaders) == "table" and not Crabe._diskLoaderInstalled then
    Crabe._diskLoaderInstalled = true
    table.insert(package.loaders, 2, function(modname)
        local subpath = string.gsub(modname, "%.", "/")
        local candidates = {
            "mods/disneyinfinitymp/" .. subpath .. ".lua",
            "mods/" .. subpath .. ".lua",
            "mods/" .. subpath .. "/init.lua",
            subpath .. ".lua"
        }
        for _, path in ipairs(candidates) do
            local chunk = loadfile(path)
            if chunk then
                return chunk
            end
        end
        return "\n\t[CrabeLoader] no file on disk matching '" .. modname .. "'"
    end)
end
