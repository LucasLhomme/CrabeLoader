-- CrabeLoader
-- File description:
-- Runs the mod lifecycle: register, then onInit, onUpdate, onDraw, onShutdown and hot reload.
-- Each callback runs bracketed by its owner, so anything it subscribes to is revoked on the reload.
-- Re-executes no mod file; the chunks are run again by ModManager after reload returns.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Mod = Crabe.Mod or {
    _registered = {},
    _initialized = false,
    _reloadCallbacks = {}
}

Crabe.Mod._owners = Crabe.Mod._owners or setmetatable({}, { __mode = "k" })
Crabe.Mod._initDone = Crabe.Mod._initDone or setmetatable({}, { __mode = "k" })

-- Runs a mod lifecycle callback under quarantine accounting bracketed by owner.
local function runLifecycleCallback(modId, callbackName, fn, ...)
    local previousOwner = Crabe.Registry.currentOwner()
    Crabe.Registry.setCurrentOwner(modId)

    local ok, err
    if Crabe.Quarantine and Crabe.Quarantine.guard then
        ok, err = pcall(Crabe.Quarantine.guard, modId, callbackName, fn, ...)
    else
        ok, err = pcall(fn, ...)
    end

    Crabe.Registry.setCurrentOwner(previousOwner)

    if not ok and Crabe.write then
        Crabe.write("! [Crabe.Mod] " .. callbackName .. " error in '"
            .. tostring(modId) .. "': " .. tostring(err))
    end
end

-- Resolves the owner identity for a mod definition.
local function ownerOf(def, index)
    return Crabe.Mod._owners[def] or def.name or index
end

-- Runs a mod onInit callback at most once per generation.
local function initialiseMod(def, index)
    if Crabe.Mod._initDone[def] then
        return
    end
    Crabe.Mod._initDone[def] = true
    if type(def.onInit) == "function" then
        runLifecycleCallback(ownerOf(def, index), "onInit", def.onInit)
    end
end

-- Registers a mod definition containing lifecycle callbacks.
function Crabe.Mod.register(def)
    if type(def) ~= "table" then
        error("Crabe.Mod.register: expected table definition", 2)
    end
    local mods = Crabe.Mod._registered
    local found = false
    local index = nil
    if def.name then
        for i = 1, #mods do
            if mods[i].name == def.name then
                mods[i] = def
                found = true
                index = i
                break
            end
        end
    end
    if not found then
        mods[#mods + 1] = def
        index = #mods
    end
    Crabe.Mod._owners[def] = Crabe.Registry.currentOwner() or def.name or index
    if Crabe.Mod._initialized then
        initialiseMod(def, index)
    end
    return def
end

-- Dispatches the onInit callback to all registered mods.
function Crabe.Mod.dispatchInit()
    Crabe.Mod._initialized = true
    local mods = Crabe.Mod._registered
    for i = 1, #mods do
        initialiseMod(mods[i], i)
    end
end

-- Updates coroutine scheduler and triggers onUpdate on all active mods.
function Crabe.Mod.dispatchUpdate(dt)
    if Crabe.Scheduler and Crabe.Scheduler.update then
        Crabe.Scheduler.update(dt)
    end
    local mods = Crabe.Mod._registered
    for i = 1, #mods do
        local mod = mods[i]
        if type(mod.onUpdate) == "function" then
            runLifecycleCallback(ownerOf(mod, i), "onUpdate", mod.onUpdate, dt)
        end
    end
end

-- Dispatches onDraw callback to all mods within the active ImGui frame.
function Crabe.Mod.dispatchDraw()
    local mods = Crabe.Mod._registered
    for i = 1, #mods do
        local mod = mods[i]
        if type(mod.onDraw) == "function" then
            runLifecycleCallback(ownerOf(mod, i), "onDraw", mod.onDraw)
        end
    end
end

-- Dispatches onShutdown callback to all mods and resets state.
function Crabe.Mod.dispatchShutdown()
    local mods = Crabe.Mod._registered
    for i = 1, #mods do
        local mod = mods[i]
        Crabe.Mod._initDone[mod] = nil
        if type(mod.onShutdown) == "function" then
            runLifecycleCallback(ownerOf(mod, i), "onShutdown", mod.onShutdown)
        end
    end
    Crabe.Mod._initialized = false
end

-- Registers a callback to be executed when mod reload is requested.
function Crabe.Mod.onReload(callback)
    if type(callback) == "function" then
        local callbacks = Crabe.Mod._reloadCallbacks
        table.insert(callbacks, callback)
        Crabe.Registry.track(function()
            for i = #callbacks, 1, -1 do
                if callbacks[i] == callback then
                    table.remove(callbacks, i)
                    break
                end
            end
        end)
    end
end

-- Shuts down registered mods, revokes subscriptions, and reinitializes.
function Crabe.Mod.reload()
    Crabe.Mod.dispatchShutdown()
    Crabe.Registry.revokeAllMods()
    Crabe.Mod._registered = {}
    if Crabe.Scheduler and Crabe.Scheduler.clear then
        Crabe.Scheduler.clear()
    end
    if Crabe.Quarantine and Crabe.Quarantine.reset then
        Crabe.Quarantine.reset()
    end
    if Crabe.Events and Crabe.Events.emit then
        Crabe.Events.emit("reload")
    end
    local callbacks = Crabe.Mod._reloadCallbacks
    for i = 1, #callbacks do
        local ok, err = pcall(callbacks[i])
        if not ok and Crabe.write then
            Crabe.write("! [Crabe.Mod] reload callback error: " .. tostring(err))
        end
    end
    Crabe.Mod.dispatchInit()
end
