-- CrabeLoader
-- File description:
-- Exposes the native .asi plugins CrabeLoader found under the Crabe.Plugins namespace.
-- Also shares string values with plugins that use the plugin API (include/sdk/crabe_plugin.h).
-- A plugin loads before the game starts and can never be loaded or unloaded from Lua.
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

-- A value a native plugin shared through the plugin API (setValue), or a mod through
-- Crabe.Plugins.set; nil when the key is not set or on a loader without the bridge.
function Crabe.Plugins.get(key)
    if type(key) ~= "string" or key == "" or not Crabe._pluginGetValue then
        return nil
    end
    return Crabe._pluginGetValue(key)
end

-- Shares a value with every native plugin (getValue) and every mod. Values are stored
-- as strings: numbers and booleans are converted with tostring, nil erases the key.
-- Returns false when the key is not a non-empty string or the loader lacks the bridge.
function Crabe.Plugins.set(key, value)
    if type(key) ~= "string" or key == "" or not Crabe._pluginSetValue then
        return false
    end
    if value == nil then
        Crabe._pluginSetValue(key)
    else
        Crabe._pluginSetValue(key, tostring(value))
    end
    return true
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
