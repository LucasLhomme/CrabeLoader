-- CrabeLoader
-- File description:
-- System surface: display and video settings, pause, saving, and the debug tooling retail shipped.
-- Several of these reach tooling the studio left in the retail build, which is why they exist.
-- Changes no window mode directly; that request is posted to the render thread by a native.
--
-- Authors: @LucasLhomme

Game = Game or {}

-- Toggles HUD suppression for the specified player.
function Game.SuppressHud(suppressed, playerId)
    if type(suppressed) ~= "boolean" then
        error("Game.SuppressHud: expected a boolean", 2)
    end
    Crabe.native("UI_SuppressHud", "Game.SuppressHud")(suppressed, Crabe.hostPlayer(playerId))
    return suppressed
end

-- Rebuilds and reloads the HUD for the player.
function Game.ReloadHud(playerId)
    if type(System) ~= "table" or type(System.ReloadHUD) ~= "function" then
        error("Game.ReloadHud: System.ReloadHUD is not available in this Lua state", 2)
    end
    System.ReloadHUD(Crabe.hostPlayer(playerId))
end

-- Returns screen width and height in pixels.
function Game.ScreenSize()
    return Crabe.native("UI_ScreenWidth", "Game.ScreenSize")(),
           Crabe.native("UI_ScreenHeight", "Game.ScreenSize")()
end

local VIDEO_TOGGLES = {
    Bloom = "Bloom", SSAO = "SSAO", FXAA = "FXAA",
    MotionBlur = "MotionBlur", DepthOfField = "DepthOfField",
    DynamicResolution = "DynamicResolution",
}

Game.VideoToggles = {}

for label, suffix in pairs(VIDEO_TOGGLES) do
    Game.VideoToggles[label] = {
        get = function()
            return Crabe.native("Settings_Get" .. suffix .. "Enabled", "Game.VideoToggles." .. label)() == true
        end,
        set = function(on)
            if type(on) ~= "boolean" then
                error("Game.VideoToggles." .. label .. ".set: expected a boolean", 2)
            end
            Crabe.native("Settings_Set" .. suffix .. "Enabled", "Game.VideoToggles." .. label)(on)
            return on
        end,
    }
end

-- Commits pending video settings changes to the engine.
function Game.ApplyVideoSettings()
    Crabe.native("Settings_ApplyVideoSettings", "Game.ApplyVideoSettings")()
end

-- Returns active render resolution width and height.
function Game.GetResolution()
    if type(Crabe._getRenderResolutionNative) == "function" then
        local width, height = Crabe._getRenderResolutionNative()
        if width and height then
            return width, height
        end
    end
    return Crabe.native("UI_GetCurrentResolution", "Game.GetResolution")()
end

-- Sets saved startup resolution width and height.
function Game.SetResolution(width, height)
    width = tonumber(width)
    height = tonumber(height)
    if not width or not height then
        error("Game.SetResolution: expected numeric width and height", 2)
    end
    Crabe.native("Settings_SetResolution", "Game.SetResolution")(width, height)
    return width, height
end

-- Saves current engine settings to disk.
function Game.SaveSettings()
    Crabe.native("Settings_Save", "Game.SaveSettings")()
end

-- Returns current gamma setting value.
function Game.GetGamma()
    return Crabe.native("Settings_GetGamma", "Game.GetGamma")()
end

-- Sets engine gamma value.
function Game.SetGamma(value)
    value = tonumber(value)
    if not value then error("Game.SetGamma: expected a number", 2) end
    Crabe.native("Settings_SetGamma", "Game.SetGamma")(value)
    return value
end

-- Returns current 0-based difficulty setting index.
function Game.GetDifficulty()
    return Crabe.native("Settings_GetDifficulty", "Game.GetDifficulty")()
end

-- Sets engine difficulty setting index.
function Game.SetDifficulty(index)
    index = tonumber(index)
    if not index then error("Game.SetDifficulty: expected a numeric index", 2) end
    Crabe.native("Settings_SetDifficulty", "Game.SetDifficulty")(index)
    return index
end

-- Pauses the game for the player.
function Game.PauseGame(playerId)
    Crabe.native("Pause_PauseGame", "Game.PauseGame")(Crabe.hostPlayer(playerId), false, false)
end

-- Unpauses the game if currently paused.
function Game.UnpauseGame()
    Crabe.native("Pause_UnPauseFromPausedScreenIfPaused", "Game.UnpauseGame")()
end

-- Quits the game application.
function Game.QuitGame()
    Crabe.native("UI_QuitGame", "Game.QuitGame")()
end

-- Returns true if game engine allows saving right now.
function Game.CanSave()
    return Crabe.native("SaveLoad_CanSave", "Game.CanSave")() == true
end

-- Returns true if a save operation is currently in progress.
function Game.IsSaving()
    return Crabe.native("SaveLoad_IsSaving", "Game.IsSaving")() == true
end

-- Triggers autosave of current world state.
function Game.SaveWorld()
    if not Game.CanSave() then
        error("Game.SaveWorld: the game reports it cannot save right now", 2)
    end
    if Game.IsSaving() then
        error("Game.SaveWorld: a save is already in progress", 2)
    end
    Crabe.native("SaveLoad_AutoSaveWorld", "Game.SaveWorld")()
end

-- Saves current player profile state.
function Game.SaveProfile()
    Crabe.native("SaveLoad_SaveProfile", "Game.SaveProfile")()
end

-- Returns list of saved game files.
function Game.ListSaves()
    return Crabe.native("SaveLoad_GetSavedGamesList", "Game.ListSaves")()
end

-- Returns true if online multiplayer session is locked.
function Game.IsSessionLocked()
    return Crabe.native("UI_GameIsLocked", "Game.IsSessionLocked")() == true
end

-- Sets online multiplayer session lock state.
function Game.SetSessionLocked(locked)
    if type(locked) ~= "boolean" then
        error("Game.SetSessionLocked: expected a boolean", 2)
    end

    if locked then
        Crabe.native("UI_LockGame", "Game.SetSessionLocked")()
    else
        Crabe.native("UI_UnlockGame", "Game.SetSessionLocked")()
    end
    return locked
end

-- Kicks player by number from multiplayer session.
function Game.KickPlayer(playerNumber)
    if not tonumber(playerNumber) then
        error("Game.KickPlayer: expected a player number", 2)
    end
    Crabe.native("UI_KickPlayer", "Game.KickPlayer")(tonumber(playerNumber))
end

-- Returns table with total, local, and maximum player counts.
function Game.PlayerCounts()
    return {
        total = Crabe.native("Players_NumPlayers", "Game.PlayerCounts")(),
        localPlayers = Crabe.native("Players_NumLocalPlayers", "Game.PlayerCounts")(),
        max = Crabe.native("Players_MaxPlayers", "Game.PlayerCounts")(),
    }
end

-- Returns true if retail test UI is allowed.
function Game.TestUIAllowed()
    return Crabe.native("UI_AllowTestUI", "Game.TestUIAllowed")() == true
end

-- Triggers engine test UI for the player.
function Game.OpenTestUI(playerId)
    Crabe.native("UI_TriggerTestUI", "Game.OpenTestUI")(Crabe.hostPlayer(playerId))
end

-- Triggers engine toy tree interface.
function Game.OpenToyTree(playerId)
    Crabe.native("UI_TriggerToyTree", "Game.OpenToyTree")(Crabe.hostPlayer(playerId), false)
end

-- Reads persistent global value by string key.
function Game.GetGlobal(key)
    if type(key) ~= "string" or key == "" then
        error("Game.GetGlobal: key must be a non-empty string", 2)
    end
    return Crabe.native("PersistentData_GetGlobal", "Game.GetGlobal")(key)
end

-- Writes persistent global value by string key.
function Game.SetGlobal(key, value)
    if type(key) ~= "string" or key == "" then
        error("Game.SetGlobal: key must be a non-empty string", 2)
    end
    Crabe.native("PersistentData_SetGlobal", "Game.SetGlobal")(key, value)
    return value
end

-- Returns true if persistent global is defined.
function Game.IsGlobalSet(name)
    if type(name) ~= "string" or name == "" then
        error("Game.IsGlobalSet: name must be a non-empty string", 2)
    end
    return Crabe.native("UI_IsGlobalSet", "Game.IsGlobalSet")(name) == true
end

-- Sends synthetic button press event by key name.
function Game.PressButton(buttonName)
    if type(buttonName) ~= "string" or buttonName == "" then
        error("Game.PressButton: buttonName must be a non-empty string", 2)
    end
    Crabe.native("ToyboxController_ButtonDown", "Game.PressButton")(buttonName)
    return buttonName
end

-- Sends synthetic button release event by key name.
function Game.ReleaseButton(buttonName)
    if type(buttonName) ~= "string" or buttonName == "" then
        error("Game.ReleaseButton: buttonName must be a non-empty string", 2)
    end
    Crabe.native("ToyboxController_ButtonUp", "Game.ReleaseButton")(buttonName)
    return buttonName
end

-- Returns sorted list of supported button key names.
function Game.ListButtons()
    if type(KeyMap) ~= "table" then
        error("Game.ListButtons: KeyMap is not available in this Lua state", 2)
    end

    local names = {}
    for key, value in pairs(KeyMap) do
        if type(key) == "string" and type(value) == "number" then
            names[#names + 1] = key
        end
    end
    table.sort(names)
    return names
end
