-- Example mod: adds a "Borderless Window" toggle to Settings > Video by
-- monkey-patching SettingsVideo:BuildList. SettingsVideo doesn't exist until
-- the player opens that screen once, so a __newindex watcher on _G catches
-- its creation, then a second one catches BuildList being assigned onto it.

-- Crabe.SetWindowMode is a side-effect-free native wrapper; other options
-- persist via the game's own save data, which this mod can't reach, so it
-- keeps its own tiny file next to loader.log instead.
local kConfigPath = "crabe_window_mode.cfg"

local function loadSavedMode()
    local file = io.open(kConfigPath, "r")
    if not file then return "windowed" end

    local content = file:read("*a")
    file:close()

    if content and content:match("^%s*borderless%s*$") then return "borderless" end
    return "windowed"
end

local function saveMode(mode)
    local file = io.open(kConfigPath, "w")
    if not file then return end
    file:write(mode)
    file:close()
end

local function installOption(cls)
    local originalBuildList = cls.BuildList

    function cls:BuildList(...)
        local result = originalBuildList(self, ...)

        table.insert(self.listData, {
            id = "crabeWindowMode",
            text = "Borderless Window",
            widgetType = "Toggle",
            get = function() return Crabe.GetWindowMode() == "borderless" end,
            set = function()
                local newMode = Crabe.GetWindowMode() == "borderless" and "windowed" or "borderless"
                Crabe.SetWindowMode(newMode)
                saveMode(newMode)
            end,
        })

        return result
    end
end

-- Restore last session's choice immediately, before the player ever opens
-- Settings > Video. "windowed" needs no call: it is already the window's
-- native starting state (src/render_hook.cpp captures it on backend init).
if loadSavedMode() == "borderless" then
    Crabe.SetWindowMode("borderless")
end

local function watchForBuildList(cls)
    local mt = getmetatable(cls)
    if not mt then
        mt = {}
        setmetatable(cls, mt)
    end

    mt.__newindex = function(t, k, v)
        rawset(t, k, v)
        if k ~= "BuildList" then return end
        mt.__newindex = ni
        installOption(t)
    end
end

if SettingsVideo then
    installOption(SettingsVideo)
else
    local rootMt = getmetatable(_G) or {}
    rootMt.__newindex = function(t, k, v)
        rawset(t, k, v)
        if k ~= "SettingsVideo" then return end
        rootMt.__newindex = nil
        watchForBuildList(v)
    end
    setmetatable(_G, rootMt)
end

print("window_mode mod loaded")
