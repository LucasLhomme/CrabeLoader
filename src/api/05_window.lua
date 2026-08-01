-- Crabe.SetWindowMode/GetWindowMode -- the ergonomic pair around the one real
-- C++ native (Crabe._setWindowModeNative). No native anywhere in the game
-- exposes window/fullscreen state to Lua (see docs/nativedb.md), so this
-- cannot be a Game.* wrapper around an existing engine call the way
-- everything else in api/ is; RenderHook does it directly in Win32.

Crabe._windowMode = "windowed"

function Crabe.SetWindowMode(mode)
    if mode ~= "windowed" and mode ~= "borderless" then
        error("Crabe.SetWindowMode: expected 'windowed' or 'borderless', got '" .. tostring(mode) .. "'", 2)
    end

    Crabe._setWindowModeNative(mode)
    Crabe._windowMode = mode
    return true -- the console echoes tostring() on the call's result; a 0-value return breaks that
end

-- The game never tracks this itself, so this is just the last value Crabe
-- itself set -- not a real Win32 query.
function Crabe.GetWindowMode()
    return Crabe._windowMode
end
