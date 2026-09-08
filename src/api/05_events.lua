-- `Crabe.Events` - Unified Event Bus for Disney Infinity 3.0 Modding
-- Enables decoupled pub/sub event communication across mods and engine hooks.

Crabe = Crabe or {}
Crabe.Events = Crabe.Events or {
    _listeners = {}
}

--- Registers an event handler for the given event name.
---@param eventName string The name of the event to listen for
---@param handler function The callback function to execute when the event fires
---@return function handler The registered handler function (can be passed to off)
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
    return handler
end

--- Registers a one-shot event handler that automatically unregisters itself after firing once.
---@param eventName string The name of the event
---@param handler function The callback function to execute
---@return function wrapper The registered wrapper function
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

--- Unregisters a previously registered event handler.
---@param eventName string The name of the event
---@param handler function The handler function to remove
---@return boolean success True if removed, false otherwise
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

--- Emits an event to all registered listeners.
--- Handlers are invoked safely inside pcall to prevent failures from impacting other mods.
---@param eventName string The name of the event to fire
---@param ... any Arguments to forward to each listener
function Crabe.Events.emit(eventName, ...)
    local list = Crabe.Events._listeners[eventName]
    if not list or #list == 0 then return end

    -- Clone listener array to allow safe modification during emission
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

--- Clears registered listeners. If eventName is given, clears only that event.
--- If eventName is omitted, clears all listeners across all events.
---@param eventName? string Optional event name
function Crabe.Events.clear(eventName)
    if eventName then
        Crabe.Events._listeners[eventName] = nil
    else
        Crabe.Events._listeners = {}
    end
end

