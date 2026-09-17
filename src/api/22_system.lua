-- System: display, video settings, pause, saves, multiplayer, and the debug
-- tooling the studio shipped in the retail build.
--
-- Video setters are pending until committed: settingsvideo.lua sets its flags
-- and then calls Settings_ApplyVideoSettings() once (:243, :305). So every
-- Set* here changes intent only, and Game.ApplyVideoSettings() is what the
-- player sees. Game.SaveSettings() persists across sessions.

Game = Game or {}

-- ---------------------------------------------------------------------------
-- HUD and display
-- ---------------------------------------------------------------------------

-- challengeadvertisement.lua:79 and :127 bracket a screen with true then
-- false, so this is a suppression flag rather than a toggle: pass false to
-- bring the HUD back. Argument order is bool first, player second.
function Game.SuppressHud(suppressed, playerId)
    if type(suppressed) ~= "boolean" then
        error("Game.SuppressHud: expected a boolean", 2)
    end
    Crabe.native("UI_SuppressHud", "Game.SuppressHud")(suppressed, Crabe.hostPlayer(playerId))
    return suppressed
end

-- Rebuilds the HUD after something has left it in a bad state. System is a
-- table of nested natives, so it is reached through the table, not _G.
function Game.ReloadHud(playerId)
    if type(System) ~= "table" or type(System.ReloadHUD) ~= "function" then
        error("Game.ReloadHud: System.ReloadHUD is not available in this Lua state", 2)
    end
    System.ReloadHUD(Crabe.hostPlayer(playerId))
end

function Game.ScreenSize()
    return Crabe.native("UI_ScreenWidth", "Game.ScreenSize")(),
           Crabe.native("UI_ScreenHeight", "Game.ScreenSize")()
end

-- ---------------------------------------------------------------------------
-- Video settings
-- ---------------------------------------------------------------------------

-- The toggles share one shape, so they are generated rather than written out
-- nine times. Each name is the Settings_ suffix; the natives are resolved at
-- call time because this file loads before they exist.
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

-- Commits every pending video setting. Nothing above takes visible effect
-- until this runs.
function Game.ApplyVideoSettings()
    Crabe.native("Settings_ApplyVideoSettings", "Game.ApplyVideoSettings")()
end

function Game.SaveSettings()
    Crabe.native("Settings_Save", "Game.SaveSettings")()
end

function Game.GetGamma()
    return Crabe.native("Settings_GetGamma", "Game.GetGamma")()
end

function Game.SetGamma(value)
    value = tonumber(value)
    if not value then error("Game.SetGamma: expected a number", 2) end
    Crabe.native("Settings_SetGamma", "Game.SetGamma")(value)
    return value
end

-- settings.lua:129 passes difficultyIndex - 1, so the native is 0-based while
-- the screen above it counts from 1. This wrapper speaks the native's index.
function Game.GetDifficulty()
    return Crabe.native("Settings_GetDifficulty", "Game.GetDifficulty")()
end

function Game.SetDifficulty(index)
    index = tonumber(index)
    if not index then error("Game.SetDifficulty: expected a numeric index", 2) end
    Crabe.native("Settings_SetDifficulty", "Game.SetDifficulty")(index)
    return index
end

-- ---------------------------------------------------------------------------
-- Pause and exit
-- ---------------------------------------------------------------------------

-- pausemenu.lua:16 passes (playerNum, false, false); what the two booleans
-- select is not explained by any call site, and every site passes false.
function Game.PauseGame(playerId)
    Crabe.native("Pause_PauseGame", "Game.PauseGame")(Crabe.hostPlayer(playerId), false, false)
end

function Game.UnpauseGame()
    Crabe.native("Pause_UnPauseFromPausedScreenIfPaused", "Game.UnpauseGame")()
end

function Game.QuitGame()
    Crabe.native("UI_QuitGame", "Game.QuitGame")()
end

-- ---------------------------------------------------------------------------
-- Saves
-- ---------------------------------------------------------------------------

function Game.CanSave()
    return Crabe.native("SaveLoad_CanSave", "Game.CanSave")() == true
end

function Game.IsSaving()
    return Crabe.native("SaveLoad_IsSaving", "Game.IsSaving")() == true
end

-- Refuses to start a save the engine says it cannot take, and refuses to
-- overlap with one already running -- both would be silently dropped.
function Game.SaveWorld()
    if not Game.CanSave() then
        error("Game.SaveWorld: the game reports it cannot save right now", 2)
    end
    if Game.IsSaving() then
        error("Game.SaveWorld: a save is already in progress", 2)
    end
    Crabe.native("SaveLoad_AutoSaveWorld", "Game.SaveWorld")()
end

function Game.SaveProfile()
    Crabe.native("SaveLoad_SaveProfile", "Game.SaveProfile")()
end

function Game.ListSaves()
    return Crabe.native("SaveLoad_GetSavedGamesList", "Game.ListSaves")()
end

-- ---------------------------------------------------------------------------
-- Multiplayer
-- ---------------------------------------------------------------------------
--
-- This is the real meaning of UI_LockGame / UI_UnlockGame: the host locking
-- the online session so nobody else can join. It unlocks no content -- see
-- 17_unlock.lua for that.

function Game.IsSessionLocked()
    return Crabe.native("UI_GameIsLocked", "Game.IsSessionLocked")() == true
end

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

function Game.KickPlayer(playerNumber)
    if not tonumber(playerNumber) then
        error("Game.KickPlayer: expected a player number", 2)
    end
    Crabe.native("UI_KickPlayer", "Game.KickPlayer")(tonumber(playerNumber))
end

function Game.PlayerCounts()
    return {
        total = Crabe.native("Players_NumPlayers", "Game.PlayerCounts")(),
        localPlayers = Crabe.native("Players_NumLocalPlayers", "Game.PlayerCounts")(),
        max = Crabe.native("Players_MaxPlayers", "Game.PlayerCounts")(),
    }
end

-- ---------------------------------------------------------------------------
-- Debug tooling left in the retail build
-- ---------------------------------------------------------------------------

-- UI_AllowTestUI only decides whether the pause menu draws the button
-- (pausemenu.lua:772); it does not gate the native. So this reports what the
-- build claims while OpenTestUI ignores it.
function Game.TestUIAllowed()
    return Crabe.native("UI_AllowTestUI", "Game.TestUIAllowed")() == true
end

function Game.OpenTestUI(playerId)
    Crabe.native("UI_TriggerTestUI", "Game.OpenTestUI")(Crabe.hostPlayer(playerId))
end

function Game.OpenToyTree(playerId)
    Crabe.native("UI_TriggerToyTree", "Game.OpenToyTree")(Crabe.hostPlayer(playerId), false)
end

-- Persistent globals survive across sessions, which makes them the natural
-- store for the menu's own settings as well as a window into the game's.
function Game.GetGlobal(key)
    if type(key) ~= "string" or key == "" then
        error("Game.GetGlobal: key must be a non-empty string", 2)
    end
    return Crabe.native("PersistentData_GetGlobal", "Game.GetGlobal")(key)
end

function Game.SetGlobal(key, value)
    if type(key) ~= "string" or key == "" then
        error("Game.SetGlobal: key must be a non-empty string", 2)
    end
    Crabe.native("PersistentData_SetGlobal", "Game.SetGlobal")(key, value)
    return value
end

function Game.IsGlobalSet(name)
    if type(name) ~= "string" or name == "" then
        error("Game.IsGlobalSet: name must be a non-empty string", 2)
    end
    return Crabe.native("UI_IsGlobalSet", "Game.IsGlobalSet")(name) == true
end

-- ---------------------------------------------------------------------------
-- Synthetic input
-- ---------------------------------------------------------------------------

-- Feeds the game an input action as though the pad or keyboard had produced
-- it. Valid names are the numeric keys of the KeyMap table -- "DEFAULT_A",
-- "KEY_SPACEBAR", "RBN_TB_PLACE" and 249 others.
function Game.PressButton(buttonName)
    if type(buttonName) ~= "string" or buttonName == "" then
        error("Game.PressButton: buttonName must be a non-empty string", 2)
    end
    Crabe.native("ToyboxController_ButtonDown", "Game.PressButton")(buttonName)
    return buttonName
end

function Game.ReleaseButton(buttonName)
    if type(buttonName) ~= "string" or buttonName == "" then
        error("Game.ReleaseButton: buttonName must be a non-empty string", 2)
    end
    Crabe.native("ToyboxController_ButtonUp", "Game.ReleaseButton")(buttonName)
    return buttonName
end

-- The names the engine will accept, read from KeyMap rather than hardcoded so
-- this cannot drift from the build.
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
