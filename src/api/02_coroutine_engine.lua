-- CrabeLoader
-- File description:
-- Implements Crabe.spawn and Crabe.wait: cooperative fibers resumed once their delay has elapsed.
-- A fiber that runs to completion never enters the scheduler, so only waiting fibers are tracked.
-- Drives no frame itself; Crabe.Scheduler.update is called from the tick dispatcher.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Scheduler = Crabe.Scheduler or {
    _fibers = {},
    _currentTime = 0
}

--- Yields execution of the calling coroutine fiber for a duration.
--- Must be called inside a coroutine fiber spawned by Crabe.spawn.
function Crabe.wait(milliseconds)
    local co = coroutine.running()
    if not co then
        error("Crabe.wait: must be called within a coroutine fiber", 2)
    end
    return coroutine.yield(tonumber(milliseconds) or 0)
end

Wait = Crabe.wait
Crabe.Wait = Crabe.wait

--- Spawns a coroutine fiber and schedules it for periodic resumption.
--- Executes immediately until completion or first yield.
function Crabe.spawn(fn, ...)
    if type(fn) ~= "function" then
        error("Crabe.spawn: expected a function argument", 2)
    end
    local co = coroutine.create(fn)
    local ok, delay = coroutine.resume(co, ...)
    if not ok then
        if Crabe.write then
            Crabe.write("! [Crabe.Scheduler] Fiber spawn error: " .. tostring(delay))
        end
        return nil
    end
    if coroutine.status(co) ~= "dead" then
        local fiber = {
            co = co,
            wakeTime = Crabe.Scheduler._currentTime + (tonumber(delay) or 0)
        }
        table.insert(Crabe.Scheduler._fibers, fiber)
    end
    return co
end

--- Resumes sleeping fibers whose scheduled wake timestamp has elapsed.
--- Advances internal scheduler clock by delta time in seconds.
function Crabe.Scheduler.update(dt)
    local dtMs = (tonumber(dt) or 0) * 1000
    Crabe.Scheduler._currentTime = Crabe.Scheduler._currentTime + dtMs
    local current = Crabe.Scheduler._fibers
    Crabe.Scheduler._fibers = {}
    for i = 1, #current do
        local fiber = current[i]
        if Crabe.Scheduler._currentTime >= fiber.wakeTime then
            local ok, delay = coroutine.resume(fiber.co)
            if ok and coroutine.status(fiber.co) ~= "dead" then
                fiber.wakeTime = Crabe.Scheduler._currentTime + (tonumber(delay) or 0)
                table.insert(Crabe.Scheduler._fibers, fiber)
            elseif not ok then
                if Crabe.write then
                    Crabe.write("! [Crabe.Scheduler] Fiber error: " .. tostring(delay))
                end
            end
        else
            table.insert(Crabe.Scheduler._fibers, fiber)
        end
    end
end

--- Clears all active coroutine fibers from the scheduler.
--- Resets the pending fibers list to an empty state.
function Crabe.Scheduler.clear()
    Crabe.Scheduler._fibers = {}
end
