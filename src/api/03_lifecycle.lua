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

-- Which mod's chunk registered each definition, and whether that definition
-- has already had its onInit run in the current generation. Keyed by the
-- definition table itself and weak, so a mod dropped from _registered takes
-- its bookkeeping with it. Assigned separately from the constructor above
-- because that constructor does not run when the API is re-injected into a
-- VM that already has a Crabe.Mod.
Crabe.Mod._owners = Crabe.Mod._owners or setmetatable({}, { __mode = "k" })
Crabe.Mod._initDone = Crabe.Mod._initDone or setmetatable({}, { __mode = "k" })

-- Runs one mod's lifecycle callback under Crabe.Quarantine accounting (T11:
-- ten consecutive failures disable this one callback, three disabled
-- callbacks disable the mod -- see src/api/02c_quarantine.lua). Falls back
-- to a bare pcall, exactly what every dispatch* below did before, if the
-- quarantine module is somehow unavailable.
--
-- The callback runs bracketed by its owner. Dispatching a mod's callback is
-- as much "that mod executing" as running its chunk is, so whatever the
-- callback subscribes to belongs to it. Without the bracket
-- Crabe.Registry.currentOwner() is nil here, every such subscription fell to
-- the "core" owner that revokeAllMods() deliberately never revokes, and
-- Game._tickOwners recorded "core" for it too -- so a mod subscribing from
-- its onInit, which is where docs/guides/mods.md tells a mod to set itself
-- up, leaked one subscription per hot reload and had its tick failures
-- charged to the API.
local function runLifecycleCallback(modId, callbackName, fn, ...)
    local previousOwner = Crabe.Registry.currentOwner()
    Crabe.Registry.setCurrentOwner(modId)

    local ok, err
    if Crabe.Quarantine and Crabe.Quarantine.guard then
        ok, err = pcall(Crabe.Quarantine.guard, modId, callbackName, fn, ...)
    else
        ok, err = pcall(fn, ...)
    end

    -- Restored before anything else can run: a stale owner left behind here
    -- would misattribute every later core-owned subscription to this mod.
    Crabe.Registry.setCurrentOwner(previousOwner)

    if not ok and Crabe.write then
        Crabe.write("! [Crabe.Mod] " .. callbackName .. " error in '"
            .. tostring(modId) .. "': " .. tostring(err))
    end
end

-- A definition's owner is whichever mod's chunk registered it -- the id the
-- C++ loader bracketed that chunk with -- and not def.name, which the mod
-- chooses freely and which need not match its folder. `index` reproduces the
-- old label for a definition registered outside any chunk, as the API's own
-- modules and the tests do.
local function ownerOf(def, index)
    return Crabe.Mod._owners[def] or def.name or index
end

-- Runs one mod's onInit, at most once per generation.
--
-- Both routes into onInit funnel through here, which is what makes "at most
-- once" true: Crabe.Mod.register() for a mod that arrives after the lifecycle
-- is already up, and dispatchInit() for the mods registered before it. A hot
-- reload takes both routes. ModManager::reloadAllMods runs Crabe.Mod.reload(),
-- which ends in dispatchInit() and so leaves _initialized true, and then
-- re-runs every mod chunk: register() initialised each mod, and the
-- dispatchInit() closing discoverAndLoadMods() initialised it a second time.
local function initialiseMod(def, index)
    if Crabe.Mod._initDone[def] then
        return
    end
    Crabe.Mod._initDone[def] = true
    if type(def.onInit) == "function" then
        runLifecycleCallback(ownerOf(def, index), "onInit", def.onInit)
    end
end

--- Registers a mod definition containing lifecycle callbacks.
--- Runs onInit immediately if the lifecycle is already initialized.
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
    -- Captured now, because by the time a lifecycle callback runs the current
    -- owner has moved on to whatever loaded after this mod.
    Crabe.Mod._owners[def] = Crabe.Registry.currentOwner() or def.name or index
    if Crabe.Mod._initialized then
        initialiseMod(def, index)
    end
    return def
end

--- Dispatches the onInit callback to all registered mods.
--- Sets initialized state and safely captures any runtime errors.
function Crabe.Mod.dispatchInit()
    Crabe.Mod._initialized = true
    local mods = Crabe.Mod._registered
    for i = 1, #mods do
        initialiseMod(mods[i], i)
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
            runLifecycleCallback(ownerOf(mod, i), "onUpdate", mod.onUpdate, dt)
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
            runLifecycleCallback(ownerOf(mod, i), "onDraw", mod.onDraw)
        end
    end
end

--- Dispatches onShutdown callback to all mods and resets state.
--- Allows mods to release resources and persist state before unloading.
function Crabe.Mod.dispatchShutdown()
    local mods = Crabe.Mod._registered
    for i = 1, #mods do
        local mod = mods[i]
        -- This generation is over, so the next dispatchInit() must be allowed
        -- to initialise the mod again. Cleared even for a definition whose
        -- table the mod reuses across reloads, which weak keys alone would
        -- keep marked as already initialised forever.
        Crabe.Mod._initDone[mod] = nil
        if type(mod.onShutdown) == "function" then
            runLifecycleCallback(ownerOf(mod, i), "onShutdown", mod.onShutdown)
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
    -- A mod fixed on disk since the last reload gets a clean slate rather
    -- than starting pre-quarantined from its previous, broken generation.
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
