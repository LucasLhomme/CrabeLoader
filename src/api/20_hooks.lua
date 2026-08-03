-- Per-frame callbacks. The engine has no event system and no global Update to
-- wrap, so _runTicks is driven from C++ (Loader::runTicks) at ~60 Hz.

Game._tickCallbacks = Game._tickCallbacks or {}

function Game.onTick(fn)
    if type(fn) ~= "function" then
        error("Game.onTick: expected a function", 2)
    end
    Game._tickCallbacks[#Game._tickCallbacks + 1] = fn
end

function Game._runTicks(dt)
    local callbacks = Game._tickCallbacks

    for i = 1, #callbacks do
        -- Isolated: one mod erroring must not stop the others.
        local ok, err = pcall(callbacks[i], dt)
        if not ok then
            Crabe.write("! onTick handler #" .. i .. ": " .. tostring(err))
        end
    end
end

-- There is no death event in the engine (docs/nativedb.md), so this polls
-- Game.IsCharacterDead every tick and fires once on the alive -> dead edge.
Game._deathWatchers = Game._deathWatchers or {}

function Game.onDeath(playerId, fn)
    if type(fn) ~= "function" then
        error("Game.onDeath: expected a function", 2)
    end
    Game._deathWatchers[#Game._deathWatchers + 1] = { playerId = playerId, fn = fn, wasAlive = true }
end

Game.onTick(function()
    for _, watcher in ipairs(Game._deathWatchers) do
        local ok, dead = pcall(Game.IsCharacterDead, watcher.playerId)
        if ok then
            if dead and watcher.wasAlive then
                watcher.fn(watcher.playerId)
            end
            watcher.wasAlive = not dead
        end
    end
end)
