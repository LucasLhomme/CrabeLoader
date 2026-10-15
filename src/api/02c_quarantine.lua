-- CrabeLoader
-- File description:
-- Counts callback failures and escalates: ten in a row disables a callback, three disable the mod.
-- Identical consecutive messages log once with a repeat summary, so a broken mod cannot flood.
-- Owns no callback list; it is called by the lifecycle dispatcher and the tick loop.
--
-- Authors: @LucasLhomme

local kConsecutiveFailuresToDisableCallback = 10
local kDisabledCallbacksToDisableMod = 3
local kExemptFromModDisable = "core"

Crabe = Crabe or {}
Crabe.Quarantine = Crabe.Quarantine or {
    _callbacks = {},
    _mods = {},
}

-- Returns a formatted callback dictionary key combining mod id and callback name.
local function callbackKey(modId, callbackName)
    return tostring(modId) .. "\0" .. tostring(callbackName)
end

-- Retrieves or initializes the quarantine record table for a mod id.
local function modRecord(modId)
    local mods = Crabe.Quarantine._mods
    local record = mods[modId]
    if not record then
        record = { disabledCallbacks = 0, disabled = false }
        mods[modId] = record
    end
    return record
end

-- Returns true when the specified mod has been quarantined and disabled.
function Crabe.Quarantine.isModDisabled(modId)
    local record = Crabe.Quarantine._mods[modId]
    return record ~= nil and record.disabled
end

-- Returns true when this specific callback tripped the consecutive failure threshold.
function Crabe.Quarantine.isCallbackDisabled(modId, callbackName)
    local record = Crabe.Quarantine._callbacks[callbackKey(modId, callbackName)]
    return record ~= nil and record.disabled
end

-- Flushes a pending repeated error summary for the callback to the log.
local function flushPending(cb, prefix)
    if cb.pendingRepeats and cb.pendingRepeats > 0 and Crabe.write then
        Crabe.write(prefix .. " (repeated " .. cb.pendingRepeats .. " times, last: "
            .. tostring(cb.pendingMessage) .. ")")
    end
    cb.pendingMessage = nil
    cb.pendingRepeats = 0
end

-- Runs a function under quarantine failure tracking without propagating errors.
function Crabe.Quarantine.guard(modId, callbackName, fn, ...)
    if type(fn) ~= "function" then
        return
    end

    local mod = modRecord(modId)
    if mod.disabled then
        return
    end

    local key = callbackKey(modId, callbackName)
    local cb = Crabe.Quarantine._callbacks[key]
    if not cb then
        cb = {
            consecutiveFailures = 0, disabled = false,
            modId = modId, callbackName = callbackName,
            pendingMessage = nil, pendingRepeats = 0,
        }
        Crabe.Quarantine._callbacks[key] = cb
    end
    if cb.disabled then
        return
    end

    local ok, err = pcall(fn, ...)
    local prefix = "! [" .. tostring(modId) .. "] " .. tostring(callbackName)

    if ok then
        if cb.consecutiveFailures > 0 then
            flushPending(cb, prefix)
        end
        cb.consecutiveFailures = 0
        return
    end

    local message = tostring(err)
    cb.consecutiveFailures = cb.consecutiveFailures + 1
    cb.lastMessage = message

    if cb.pendingMessage == nil then
        cb.pendingMessage = message
        cb.pendingRepeats = 0
        if Crabe.write then Crabe.write(prefix .. ": " .. message) end
    elseif cb.pendingMessage == message then
        cb.pendingRepeats = cb.pendingRepeats + 1
    else
        flushPending(cb, prefix)
        cb.pendingMessage = message
        cb.pendingRepeats = 0
        if Crabe.write then Crabe.write(prefix .. ": " .. message) end
    end

    if cb.consecutiveFailures >= kConsecutiveFailuresToDisableCallback and not cb.disabled then
        flushPending(cb, prefix)
        cb.disabled = true
        if Crabe.write then
            Crabe.write(prefix .. " disabled after " .. kConsecutiveFailuresToDisableCallback
                .. " consecutive failures")
        end

        if modId ~= kExemptFromModDisable then
            mod.disabledCallbacks = mod.disabledCallbacks + 1
            if mod.disabledCallbacks >= kDisabledCallbacksToDisableMod and not mod.disabled then
                mod.disabled = true
                if Crabe.write then
                    Crabe.write("! [" .. tostring(modId) .. "] quarantined: "
                        .. mod.disabledCallbacks .. " callbacks disabled")
                end
            end
        end
    end
end

-- Clears all quarantine failure counters and disabled states.
function Crabe.Quarantine.reset()
    Crabe.Quarantine._callbacks = {}
    Crabe.Quarantine._mods = {}
end

-- Generates a tab-separated quarantine status report for the host application.
function Crabe.Quarantine.report()
    local lines = {}

    local disabledMods = {}
    for modId, record in pairs(Crabe.Quarantine._mods) do
        if record.disabled then
            disabledMods[#disabledMods + 1] = modId
        end
    end
    if #disabledMods > 0 then
        table.sort(disabledMods)
        lines[#lines + 1] = "MOD\t" .. table.concat(disabledMods, "\t")
    end

    local callbackLines = {}
    for _, cb in pairs(Crabe.Quarantine._callbacks) do
        if cb.consecutiveFailures > 0 or cb.disabled then
            local lastError = tostring(cb.lastMessage or ""):gsub("[\t\r\n]", " ")
            callbackLines[#callbackLines + 1] = string.format("CB\t%s\t%s\t%d\t%d\t%s",
                tostring(cb.modId), tostring(cb.callbackName), cb.consecutiveFailures,
                cb.disabled and 1 or 0, lastError)
        end
    end
    table.sort(callbackLines)
    for _, line in ipairs(callbackLines) do
        lines[#lines + 1] = line
    end

    return table.concat(lines, "\n")
end
