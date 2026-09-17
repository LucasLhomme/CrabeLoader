local kConfigPath = "crabe_window_mode.cfg"

-- Loads saved window mode from configuration file if explicitly saved.
local function loadSavedMode()
    local file = io.open(kConfigPath, "r")
    if not file then
        return nil
    end

    local content = file:read("*a")
    file:close()

    if content and content:match("^%s*windowed%s*$") then
        return "windowed"
    elseif content and content:match("^%s*borderless%s*$") then
        return "borderless"
    end
    return nil
end

-- Persists user chosen window mode to configuration file.
local function saveMode(mode)
    local file = io.open(kConfigPath, "w")
    if not file then
        return
    end
    file:write(mode)
    file:close()
end

-- Injects borderless window toggle into video settings menu list.
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

-- Watches for BuildList method assignment to hook options menu generation.
local function watchForBuildList(cls)
    local mt = getmetatable(cls)
    if not mt then
        mt = {}
        setmetatable(cls, mt)
    end

    mt.__newindex = function(t, k, v)
        rawset(t, k, v)
        if k ~= "BuildList" then
            return
        end
        mt.__newindex = nil
        installOption(t)
    end
end

local savedMode = loadSavedMode()
if savedMode then
    Crabe.SetWindowMode(savedMode)
end

if SettingsVideo then
    installOption(SettingsVideo)
else
    local rootMt = getmetatable(_G) or {}
    rootMt.__newindex = function(t, k, v)
        rawset(t, k, v)
        if k ~= "SettingsVideo" then
            return
        end
        rootMt.__newindex = nil
        watchForBuildList(v)
    end
    setmetatable(_G, rootMt)
end
