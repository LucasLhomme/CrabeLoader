-- CrabeLoader Mod: resolution.lua
-- File description:
-- Adds 4K UHD, Ultrawide (21:9 & 32:9), 16:10, Steam Deck and smaller laptop/retro modes to the
-- game's own Video Settings resolution list, without hex-editing DisneyInfinity3.exe.
--
-- Listing a mode is not enough on its own: the engine only keeps a saved resolution it
-- recognises, and clamps anything else to 2560x1440 at boot. CrabeLoader widens that check
-- natively (EngineResolution), so a mode chosen here survives the restart the game asks for.
-- On an ultrawide or 16:10 mode the 3D fills the screen and the menus and HUD, drawn for 16:9,
-- stay centred with bars on both sides.
--
-- Also adds a "Frame Rate Limit" row to the same screen, from Unlimited to 180 FPS. The game
-- never caps its frame rate on PC (vsync is off), so the cap is the loader's: Crabe.SetFrameLimit
-- applies at once, with no restart, and saves the value to Crabe/crabe.toml for the next
-- launches. The F5 menu shows the same setting; both read and write the one value.
--
-- Authors: @LucasLhomme

local RESOLUTION_PRESETS = {
    -- 4:3 & 5:4 Classic
    { 640,  480 },
    { 800,  600 },
    { 1024, 768 },
    { 1280, 960 },
    { 1280, 1024 },
    { 1600, 1200 },

    -- 16:10 Handheld / Laptop
    { 1280, 800 },   -- Steam Deck native
    { 1440, 900 },
    { 1680, 1050 },
    { 1920, 1200 },
    { 2560, 1600 },

    -- 16:9 Standard & HD
    { 960,  540 },
    { 1024, 576 },
    { 1136, 640 },
    { 1280, 720 },   -- 720p
    { 1360, 768 },
    { 1366, 768 },
    { 1600, 900 },
    { 1920, 1080 },  -- 1080p FHD
    { 2560, 1440 },  -- 1440p QHD

    -- 21:9 & 32:9 Ultrawide
    { 2560, 1080 },  -- 21:9 UW
    { 3440, 1440 },  -- 21:9 UWQHD
    { 3840, 1600 },  -- 21:9 UW
    { 5120, 1440 },  -- 32:9 Super Ultrawide
    { 5120, 2160 },  -- 21:9 5K2K

    -- High-End & 4K UHD / 5K / 8K
    { 3840, 2160 },  -- 4K UHD
    { 5120, 2880 },  -- 5K
    { 7680, 4320 },  -- 8K UHD
}

local function injectResolutions(self)
    if not self or type(self.resolutionText) ~= "table" then return end

    local curW, curH = Game.GetResolution()

    local seen = {}
    local list = {}

    local function addRes(w, h)
        w = tonumber(w)
        h = tonumber(h)
        if not w or not h or w <= 0 or h <= 0 then return end
        local key = string.format("%dx%d", w, h)
        if not seen[key] then
            seen[key] = true
            list[#list + 1] = { width = w, height = h, text = key }
        end
    end

    -- Keep every mode the engine enumerated from the display, then the presets, then the
    -- running mode in case it is neither.
    if self.resolutionWidths and self.resolutionHeights then
        for i = 1, #self.resolutionWidths do
            addRes(self.resolutionWidths[i], self.resolutionHeights[i])
        end
    end
    for _, res in ipairs(RESOLUTION_PRESETS) do
        addRes(res[1], res[2])
    end
    addRes(curW, curH)

    table.sort(list, function(a, b)
        if a.width ~= b.width then return a.width < b.width end
        return a.height < b.height
    end)

    self.resolutionWidths = {}
    self.resolutionHeights = {}
    self.resolutionText = {}

    local activeIndex = 1
    for i, res in ipairs(list) do
        self.resolutionWidths[i] = res.width
        self.resolutionHeights[i] = res.height
        self.resolutionText[i] = res.text
        if res.width == curW and res.height == curH then
            activeIndex = i
        end
    end

    self.resolutionIndex = activeIndex
    self.oldResolutionIndex = activeIndex

    Crabe.write(string.format("[ResolutionMod] Populated %d resolutions in SettingsVideo (active: %dx%d)",
        #list, curW, curH))
end

Crabe.Settings.onBuild("SettingsVideo", injectResolutions)

-- Frame rate limit. 0 is "no cap".
local FRAME_LIMITS = { 0, 30, 45, 60, 75, 90, 120, 144, 160, 180, 240 }

local frameLimitLabels = {}
for i, fps in ipairs(FRAME_LIMITS) do
    frameLimitLabels[i] = (fps == 0) and "Unlimited" or (fps .. " FPS")
end

Crabe.Settings.addOption("SettingsVideo", {
    id = "crabeFrameLimit",
    text = "Frame Rate Limit",
    widgetType = "LR_Toggle",
    choices = frameLimitLabels,

    -- A cap set elsewhere (crabe.toml by hand, a Lua call) that is not in the
    -- list shows as Unlimited until the player picks one; it is not changed.
    get = function()
        local current = Crabe.GetFrameLimit()
        for i, fps in ipairs(FRAME_LIMITS) do
            if fps == current then return i end
        end
        return 1
    end,

    set = function(self, id, index)
        local fps = FRAME_LIMITS[index]
        if fps ~= nil then
            Crabe.SetFrameLimit(fps)
        end
    end,
})
