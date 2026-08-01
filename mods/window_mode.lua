-- Example mod: adds a "Borderless Window" toggle to the game's own
-- Settings > Video screen. Shows how a mod combines a real Crabe.* native
-- (Crabe.SetWindowMode/GetWindowMode, src/api/05_window.lua) with a
-- monkey-patch of the game's UI, rather than adding a separate menu of its own.
--
-- Confirmed in-game (console): SettingsVideo:BuildList(self) is what fills
-- self.listData for that screen; every existing row there (bloom, SSAO,
-- motionBlur, depthOfField, FXAA) is a plain {id, text, widgetType="Toggle",
-- get, set} entry, and text/buttonLegend are not required to be loc keys --
-- bloom's own buttonLegend is nil. BuildList rebuilds listData from scratch
-- each time it runs, so re-appending here on every call is correct, not
-- cumulative.
--
-- The timing is the hard part. SettingsVideo does not exist as a global
-- until the player first opens Settings > Video, and confirmed via the
-- console: loading Presentation/SettingsVideo.lua, constructing the screen,
-- and its first BuildList() call all happen synchronously within that same
-- frame -- polling for `SettingsVideo` via Game.onTick is always exactly one
-- frame too late for that very first open (verified: it works from the 2nd
-- open onward, but misses the 1st). The fix: watch _G itself for the moment
-- `SettingsVideo` is assigned -- a __newindex metatable is the only way to
-- observe a global being set -- then watch *that* table for the moment
-- `BuildList` is assigned onto it. Both watchers remove themselves the
-- instant they've done their one job, so nothing stays hooked longer than
-- the brief window before the player's first Settings visit.

-- Crabe.SetWindowMode itself does not persist anything -- it is a thin native
-- wrapper, deliberately side-effect-free beyond the window itself (see
-- src/api/05_window.lua). Every *other* option on this screen persists
-- because it goes through the game's own Settings_Get/Set natives, which
-- write to the game's own save data; this mod has no access to that system,
-- so it keeps its own tiny file next to loader.log instead.
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
        mt.__newindex = nil -- one-shot: found it, stop watching this class
        installOption(t)
    end
end

if SettingsVideo then
    -- Already loaded (e.g. the mod was reloaded mid-session): no race to win.
    installOption(SettingsVideo)
else
    local rootMt = getmetatable(_G) or {}
    rootMt.__newindex = function(t, k, v)
        rawset(t, k, v)
        if k ~= "SettingsVideo" then return end
        rootMt.__newindex = nil -- one-shot: stop watching every global write
        watchForBuildList(v)
    end
    setmetatable(_G, rootMt)
end

print("window_mode mod loaded")
