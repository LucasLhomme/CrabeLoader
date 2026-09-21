-- CrabeLoader
-- File description:
-- Counts callback failures and escalates: ten in a row disables a callback, three disable the mod.
-- Identical consecutive messages log once with a repeat summary, so a broken mod cannot flood.
-- Owns no callback list; it is called by the lifecycle dispatcher and the tick loop.
--
-- Authors: @LucasLhomme

-- `Crabe.Quarantine` -- per-callback failure counters and escalation.
--
-- Without this, one broken mod callback that errors every tick writes a
-- fresh log line ~60 times a second. This module is what makes a callback
-- fail loudly once and then go quiet, instead of drowning the console:
--
--   * ten *consecutive* failures of one callback disable that callback --
--     "consecutive" is load-bearing: nine failures then a success resets
--     the count to zero, it never reaches ten;
--   * three of a mod's callbacks disabled disables the whole mod, so
--     nothing owned by it (any callback, including ones that have not
--     failed yet) runs again until the next hot reload;
--   * an unbroken run of identical error messages from one callback is
--     logged once immediately, then collapsed into a single "(repeated N
--     times, last: ...)" summary the moment the run ends (a different
--     message, a success, or the callback being disabled), rather than
--     printing the same line every tick.
--
-- Counting lives here, in Lua, rather than in C++, because only this layer
-- has callback-level granularity. src/application/loader.cpp only ever
-- calls the opaque entry points Crabe.Mod.dispatchUpdate()/dispatchDraw()/
-- dispatchInit()/dispatchShutdown() and (via LuaCall::callTick) Game.
-- _runTicks(dt) -- it has no visibility into which mod owns which callback,
-- and reaching into Crabe.Mod._registered from C++ to get it would be
-- gameplay logic in the core (Invariant I1). Every counter update below
-- runs inside a pcall already executing on the single Lua thread that owns
-- this state (Invariant I3) -- the same thread loader.cpp's runTicks and
-- dispatchModDraw call in from -- so nothing here is a new entry point,
-- just bookkeeping around calls that were already happening.
--
-- application::Loader polls Crabe.Quarantine.report() the same way it polls
-- Crabe.flush() (see drainQuarantineReport), and folds it into crabe.toml's
-- [quarantine] section and the ImGui overlay.

local kConsecutiveFailuresToDisableCallback = 10
local kDisabledCallbacksToDisableMod = 3

-- The API's own tick subscriptions (the death-watch pump in
-- src/api/20_hooks.lua, installed at module-injection time under owner
-- "core") are counted the same way a mod's callback is -- a broken one still
-- gets disabled after ten consecutive failures, so it stops spamming too --
-- but "core" itself is never quarantined wholesale. Doing that would let an
-- unrelated fragile bit of the API's own plumbing take out the API for every
-- mod loaded after it, which is a much bigger hammer than one broken
-- callback warrants.
local kExemptFromModDisable = "core"

Crabe = Crabe or {}
Crabe.Quarantine = Crabe.Quarantine or {
    _callbacks = {}, -- "<mod>\0<callback>" -> record
    _mods = {},      -- mod -> { disabledCallbacks = 0, disabled = false }
}

local function callbackKey(modId, callbackName)
    return tostring(modId) .. "\0" .. tostring(callbackName)
end

local function modRecord(modId)
    local mods = Crabe.Quarantine._mods
    local record = mods[modId]
    if not record then
        record = { disabledCallbacks = 0, disabled = false }
        mods[modId] = record
    end
    return record
end

--- True when `modId` has had three of its callbacks disabled and is fully
--- quarantined -- guard() then skips everything owned by it, unconditionally.
function Crabe.Quarantine.isModDisabled(modId)
    local record = Crabe.Quarantine._mods[modId]
    return record ~= nil and record.disabled
end

--- True when this specific (modId, callbackName) pair tripped the
--- consecutive-failure threshold.
function Crabe.Quarantine.isCallbackDisabled(modId, callbackName)
    local record = Crabe.Quarantine._callbacks[callbackKey(modId, callbackName)]
    return record ~= nil and record.disabled
end

-- Flushes a pending "repeated N times" summary for `cb`, if there is one to
-- flush. Called whenever a run of identical failures ends: the message
-- changes, the callback succeeds, or it gets disabled.
local function flushPending(cb, prefix)
    if cb.pendingRepeats and cb.pendingRepeats > 0 and Crabe.write then
        Crabe.write(prefix .. " (repeated " .. cb.pendingRepeats .. " times, last: "
            .. tostring(cb.pendingMessage) .. ")")
    end
    cb.pendingMessage = nil
    cb.pendingRepeats = 0
end

--- Runs `fn(...)` under quarantine accounting for `callbackName` owned by
--- `modId`, exactly like the bare `pcall(fn, ...)` every dispatch site used
--- before this existed -- the failure is caught and counted, never
--- propagated. Returns nothing.
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

--- Clears every counter and disabled flag. Called on hot reload
--- (Crabe.Mod.reload, src/api/03_lifecycle.lua) so a mod fixed on disk gets
--- a clean slate instead of starting pre-quarantined from its previous run.
function Crabe.Quarantine.reset()
    Crabe.Quarantine._callbacks = {}
    Crabe.Quarantine._mods = {}
end

--- A flat, tab-separated report for application::Loader to poll (mirrors
--- Crabe.flush(), see 00_core.lua): one "MOD" line naming every fully
--- disabled mod (only emitted if there is at least one), then one "CB" line
--- per callback that has failed at least once. Parsed on the C++ side by
--- crabe::domain::QuarantineSnapshot::parseReport (src/domain/config.hpp),
--- which is what src/domain/config.cpp writes into crabe.toml's
--- [quarantine] section and the ImGui overlay reads.
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
