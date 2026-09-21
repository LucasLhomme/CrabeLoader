Game = Game or {}
Game._tickCallbacks = Game._tickCallbacks or {}
Game._deathWatchers = Game._deathWatchers or {}

--- Registers a callback function to be executed on each game engine tick.
--- Callbacks receive elapsed frame delta time in seconds.
--- The subscription is owned by the calling mod and revoked on hot reload.
function Game.onTick(fn)
    if type(fn) ~= "function" then
        error("Game.onTick: expected a function", 2)
    end
    local cbs = Game._tickCallbacks
    cbs[#cbs + 1] = fn
    -- Remove by identity, never by the index recorded here: other callbacks
    -- registered or revoked in between shift every index after this one.
    Crabe.Registry.track(function()
        for i = #cbs, 1, -1 do
            if cbs[i] == fn then
                table.remove(cbs, i)
                break
            end
        end
    end)
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
--- The subscription is owned by the calling mod and revoked on hot reload.
function Game.onDeath(playerId, fn)
    if type(fn) ~= "function" then
        error("Game.onDeath: expected a function", 2)
    end
    local watchers = Game._deathWatchers
    local watcher = { playerId = playerId, fn = fn, wasAlive = true }
    watchers[#watchers + 1] = watcher
    Crabe.Registry.track(function()
        for i = #watchers, 1, -1 do
            if watchers[i] == watcher then
                table.remove(watchers, i)
                break
            end
        end
    end)
end

-- The death pump. It registers at module-injection time, when no mod chunk is
-- running, so Crabe.Registry._current is nil and this lands under the "core"
-- owner -- which revokeAllMods() never touches. That is deliberate: if this
-- one tick callback were ever revoked, every Game.onDeath watcher would stop
-- firing after the first hot reload and nothing would report it.
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
