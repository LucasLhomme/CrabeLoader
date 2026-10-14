-- CrabeLoader
-- File description:
-- Exposes the native .asi plugins CrabeLoader found under the Crabe.Plugins namespace.
-- Read-only: a plugin loads before the game starts and can never be loaded or unloaded from Lua.
-- Parses the one string Crabe._asiReport returns, since natives cannot build tables.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Plugins = {}

-- Every .asi plugin found, in load order, as
-- { file = "scripts/x.asi", source = "scripts", status = "loaded", detail = "at 0x..." }.
-- status is loaded, already_loaded, skipped, failed or crashed; detail says why.
-- Empty on a loader that predates the native.
function Crabe.Plugins.list()
    local rows = {}
    if not Crabe._asiReport then
        return rows
    end
    local text = Crabe._asiReport() or ""
    for file, source, status, detail in string.gmatch(text, "([^\t\n]+)\t([^\t\n]*)\t([^\t\n]*)\t([^\n]*)\n") do
        rows[#rows + 1] = { file = file, source = source, status = status, detail = detail }
    end
    return rows
end

-- Whether a plugin of that file name (any folder, any case) is loaded right now.
function Crabe.Plugins.isLoaded(name)
    if type(name) ~= "string" or name == "" then
        return false
    end
    local wanted = string.lower(name)
    for _, plugin in ipairs(Crabe.Plugins.list()) do
        local base = string.lower(string.match(plugin.file, "([^/]+)$") or plugin.file)
        if base == wanted and (plugin.status == "loaded" or plugin.status == "already_loaded") then
            return true
        end
    end
    return false
end
