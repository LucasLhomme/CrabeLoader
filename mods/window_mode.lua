-- Persistence used to live here too (a hand-rolled copy of
-- crabe_window_mode.cfg, read on load and written on every toggle), before
-- CrabeLoader had its own configuration file. As of crabe.toml (see
-- src/domain/config.hpp) that file is migrated once and never written again
-- -- by the loader or by this mod, which kept doing so would keep the
-- legacy file alive and, worse, could revert the mode on the next launch if
-- it ever went stale relative to crabe.toml. Crabe.SetWindowMode already
-- reaches RenderHook::requestWindowMode, which persists to crabe.toml
-- itself (src/presentation/render_hook.cpp), and the engine is already in
-- the configured mode by the time any mod loads -- so this mod's only job
-- now is exposing the toggle in the video settings menu.

local installed = false

-- Injects borderless window toggle into video settings menu list.
local function installOption(cls)
    if installed or not cls or not cls.BuildList then
        return
    end
    installed = true

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

if SettingsVideo then
    installOption(SettingsVideo)
elseif Game and Game.onTick then
    Game.onTick(function()
        if not installed and SettingsVideo then
            installOption(SettingsVideo)
        end
    end)
end
