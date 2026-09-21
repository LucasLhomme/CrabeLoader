-- Worked example: a mod adding its own option to one of the game's settings
-- screens. What crabe_heroes is to the character grid, this is to the menus.
--
-- The whole mod is the one call below. Everything that used to be here --
-- finding SettingsVideo, waiting for it to exist, wrapping BuildList, guarding
-- the insert, not stacking a new wrapper on every hot reload -- is the loader's
-- job now, in src/api/21_settings.lua.
--
-- Timing is the part that is not obvious, and the reason a mod cannot do this
-- by hand: the screen table does not exist when mods load. Probed in the
-- running game, SettingsVideo was nil in the front-end state immediately after
-- injection, so the previous version fell back to polling from Game.onTick and
-- never installed. Crabe.Settings remembers the option and installs it when
-- that screen's chunk loads, which is the only moment the table is both present
-- and still carrying the game's own BuildList.
--
-- The toggle itself is a convenience, not the feature: Alt+Enter already
-- switches modes (src/presentation/render_hook.cpp) and [display].windowMode in
-- Crabe/crabe.toml decides the mode at startup. Crabe.SetWindowMode persists
-- the change, so nothing here writes any file -- an earlier version kept its own
-- crabe_window_mode.cfg and could revert the mode on a later launch.

Crabe.Settings.addOption("SettingsVideo", {
    id = "crabeWindowMode",

    -- The game's own rows use "@Scn_Options_*" localisation keys, but its
    -- resolution row passes a plain computed string, so a literal is accepted.
    text = "Borderless Window",

    -- The same widget the game's own dynamicResolution, SSAO and motionBlur
    -- rows use. A Toggle needs both get and set.
    widgetType = "Toggle",

    -- The game calls these as get(self, id) and set(self, id, value); see
    -- src/api/21_settings.lua. `value` is what the player just chose, so it is
    -- what gets applied -- flipping from the current state instead would do
    -- the wrong thing whenever the menu and the loader disagree about what the
    -- current state is.
    get = function()
        return Crabe.GetWindowMode() == "borderless"
    end,

    set = function(self, id, value)
        local borderless
        if value == 1 or value == true or value == "1" then
            borderless = true
        elseif value == 0 or value == false or value == "0" then
            borderless = false
        else
            -- No explicit value or unsupported shape: toggle current mode
            borderless = (Crabe.GetWindowMode() ~= "borderless")
        end
        Crabe.SetWindowMode(borderless and "borderless" or "windowed")
    end,
})
