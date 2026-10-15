-- CrabeLoader
-- File description:
-- A publish and subscribe bus so mods and engine hooks can talk without knowing each other.
-- A listener is removed by identity, never by index, because indices shift as others unsubscribe.
-- Emits no engine event itself; the loader and src/api/20_hooks.lua raise them.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Events = Crabe.Events or {
    _listeners = {}
}

-- Registers an event handler for the given event name, returning the handler.
function Crabe.Events.on(eventName, handler)
    if type(eventName) ~= "string" or type(handler) ~= "function" then
        error("Crabe.Events.on: expected eventName (string) and handler (function)", 2)
    end

    local list = Crabe.Events._listeners[eventName]
    if not list then
        list = {}
        Crabe.Events._listeners[eventName] = list
    end
    list[#list + 1] = handler
    Crabe.Registry.track(function()
        for i = #list, 1, -1 do
            if list[i] == handler then
                table.remove(list, i)
                break
            end
        end
    end)
    return handler
end

-- Registers a one-shot event handler that automatically unregisters itself after firing once.
function Crabe.Events.once(eventName, handler)
    if type(eventName) ~= "string" or type(handler) ~= "function" then
        error("Crabe.Events.once: expected eventName (string) and handler (function)", 2)
    end

    local wrapper
    wrapper = function(...)
        Crabe.Events.off(eventName, wrapper)
        return handler(...)
    end
    return Crabe.Events.on(eventName, wrapper)
end

-- Unregisters a previously registered event handler by identity.
function Crabe.Events.off(eventName, handler)
    local list = Crabe.Events._listeners[eventName]
    if not list then return false end

    for i = #list, 1, -1 do
        if list[i] == handler then
            table.remove(list, i)
            return true
        end
    end
    return false
end

-- Emits an event to all registered listeners safely with forwarded arguments.
function Crabe.Events.emit(eventName, ...)
    local list = Crabe.Events._listeners[eventName]
    if not list or #list == 0 then return end

    local snapshot = {}
    for i = 1, #list do
        snapshot[i] = list[i]
    end

    for _, fn in ipairs(snapshot) do
        local ok, err = pcall(fn, ...)
        if not ok then
            local msg = "! [Crabe.Events] Error in handler for '" .. tostring(eventName) .. "': " .. tostring(err)
            if Crabe.write then
                Crabe.write(msg)
            end
        end
    end
end

-- Clears registered listeners for the specified event, or all listeners if omitted.
function Crabe.Events.clear(eventName)
    if eventName then
        Crabe.Events._listeners[eventName] = nil
    else
        Crabe.Events._listeners = {}
    end
end

