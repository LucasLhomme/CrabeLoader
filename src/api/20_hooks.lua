Game = Game or {}
Game._tickCallbacks = Game._tickCallbacks or {}
Game._deathWatchers = Game._deathWatchers or {}

--- Registers a callback function to be executed on each game engine tick.
--- Callbacks receive elapsed frame delta time in seconds.
function Game.onTick(fn)
    if type(fn) ~= "function" then
        error("Game.onTick: expected a function", 2)
    end
    Game._tickCallbacks[#Game._tickCallbacks + 1] = fn
end

--- Central tick dispatcher driven from C++ Loader::runTicks at ~60 Hz.
--- Updates mod lifecycle, coroutine scheduler, tick callbacks, and events.
function Game._runTicks(dt)
    if Crabe.Mod and Crabe.Mod.dispatchUpdate then
        Crabe.Mod.dispatchUpdate(dt)
    end
    local callbacks = Game._tickCallbacks
    for i = 1, #callbacks do
        local ok, err = pcall(callbacks[i], dt)
        if not ok and Crabe.write then
            Crabe.write("! onTick handler #" .. i .. ": " .. tostring(err))
        end
    end
    if Crabe.Events and Crabe.Events.emit then
        Crabe.Events.emit("tick", dt)
        Crabe.Events.emit("update", dt)
    end
end

--- Registers a callback fired when a player character transitions to dead.
--- Polled per tick via Game.IsCharacterDead to detect state changes.
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
                if Crabe.Events and Crabe.Events.emit then
                    Crabe.Events.emit("playerDeath", watcher.playerId)
                end
            end
            watcher.wasAlive = not dead
        end
    end
end)

if Crabe.VirtualReader and type(Crabe.VirtualReader.installGridUnlock) == "function" then
    Crabe.VirtualReader.installGridUnlock()
end
