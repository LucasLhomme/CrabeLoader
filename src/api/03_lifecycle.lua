Crabe = Crabe or {}
Crabe.Mod = Crabe.Mod or {
    _registered = {},
    _initialized = false,
    _reloadCallbacks = {}
}

--- Registers a mod definition containing lifecycle callbacks.
--- Runs onInit immediately if the lifecycle is already initialized.
function Crabe.Mod.register(def)
    if type(def) ~= "table" then
        error("Crabe.Mod.register: expected table definition", 2)
    end
    local mods = Crabe.Mod._registered
    local found = false
    if def.name then
        for i = 1, #mods do
            if mods[i].name == def.name then
                mods[i] = def
                found = true
                break
            end
        end
    end
    if not found then
        mods[#mods + 1] = def
    end
    if Crabe.Mod._initialized and type(def.onInit) == "function" then
        local ok, err = pcall(def.onInit)
        if not ok and Crabe.write then
            Crabe.write("! [Crabe.Mod] onInit error in '" .. tostring(def.name or "mod") .. "': " .. tostring(err))
        end
    end
    return def
end

--- Dispatches the onInit callback to all registered mods.
--- Sets initialized state and safely captures any runtime errors.
function Crabe.Mod.dispatchInit()
    Crabe.Mod._initialized = true
    local mods = Crabe.Mod._registered
    for i = 1, #mods do
        local mod = mods[i]
        if type(mod.onInit) == "function" then
            local ok, err = pcall(mod.onInit)
            if not ok and Crabe.write then
                Crabe.write("! [Crabe.Mod] onInit error in '" .. tostring(mod.name or i) .. "': " .. tostring(err))
            end
        end
    end
end

--- Updates coroutine scheduler and triggers onUpdate on all mods.
--- Propagates frame delta time in seconds to active mods.
function Crabe.Mod.dispatchUpdate(dt)
    if Crabe.Scheduler and Crabe.Scheduler.update then
        Crabe.Scheduler.update(dt)
    end
    local mods = Crabe.Mod._registered
    for i = 1, #mods do
        local mod = mods[i]
        if type(mod.onUpdate) == "function" then
            local ok, err = pcall(mod.onUpdate, dt)
            if not ok and Crabe.write then
                Crabe.write("! [Crabe.Mod] onUpdate error in '" .. tostring(mod.name or i) .. "': " .. tostring(err))
            end
        end
    end
end

--- Dispatches onDraw callback to all mods within the active ImGui frame.
--- Protects overlay rendering pipeline from Lua callback errors.
function Crabe.Mod.dispatchDraw()
    local mods = Crabe.Mod._registered
    for i = 1, #mods do
        local mod = mods[i]
        if type(mod.onDraw) == "function" then
            local ok, err = pcall(mod.onDraw)
            if not ok and Crabe.write then
                Crabe.write("! [Crabe.Mod] onDraw error in '" .. tostring(mod.name or i) .. "': " .. tostring(err))
            end
        end
    end
end

--- Dispatches onShutdown callback to all mods and resets state.
--- Allows mods to release resources and persist state before unloading.
function Crabe.Mod.dispatchShutdown()
    local mods = Crabe.Mod._registered
    for i = 1, #mods do
        local mod = mods[i]
        if type(mod.onShutdown) == "function" then
            local ok, err = pcall(mod.onShutdown)
            if not ok and Crabe.write then
                Crabe.write("! [Crabe.Mod] onShutdown error in '" .. tostring(mod.name or i) .. "': " .. tostring(err))
            end
        end
    end
    Crabe.Mod._initialized = false
end

--- Registers a callback to be executed when mod reload is requested.
--- Enables reload handlers to re-execute mod scripts from disk.
--- The subscription is owned by the calling mod and revoked on hot reload.
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

--- Shuts down registered mods, clears registries, and reinitializes.
--- Emits reload event, invokes reload callbacks, and dispatches init.
function Crabe.Mod.reload()
    Crabe.Mod.dispatchShutdown()
    -- Revocation is what makes reload idempotent: without it every reload
    -- left the previous generation's onTick / onDeath / event listeners /
    -- reload callbacks behind, so each one ran once more per frame than
    -- before. Mods get their teardown notice from dispatchShutdown above;
    -- by this point they own nothing. Core-owned subscriptions survive.
    Crabe.Registry.revokeAllMods()
    Crabe.Mod._registered = {}
    if Crabe.Scheduler and Crabe.Scheduler.clear then
        Crabe.Scheduler.clear()
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
