-- CrabeLoader
-- File description:
-- Builds the per-mod environment: private globals behind a read-only proxy over the shared tables.
-- A write to a protected table raises and is logged with the offending mod, rather than passing.
-- Decides no mod identity -- the loader sets the owner around each chunk before calling create.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Sandbox = Crabe.Sandbox or {}
Crabe.Exports = Crabe.Exports or {}

--- Creates a read-only proxy over one shared namespace, blocking writes with security logging.
--- Shallow on purpose: reads return the real nested tables. Lua 5.1 has no __len,
--- and ipairs/pairs/next ignore __index, so a nested proxy is an empty table to
--- `#`, ipairs and pairs -- `#Crabe.Menu.stack` read 0 and every mod saw an empty
--- menu. What stays protected is the namespace itself: a mod cannot replace
--- Crabe.Menu or Game.X, only use them. A nested value that is itself a protected
--- namespace (_G.Crabe, _G.Game) comes back as its proxy, so _G is no way around it.
local function makeReadOnlyProxy(realTable, tableName, modName, cache)
    if type(realTable) ~= "table" then return realTable end
    cache = cache or {}
    if cache[realTable] then return cache[realTable] end

    local proxy = {}
    cache[realTable] = proxy

    local mt = {
        __index = function(t, k)
            local v = realTable[k]
            return cache[v] or v
        end,
        __newindex = function(t, k, v)
            local msg = string.format(
                "! [Crabe.Sandbox] Security Violation: Mod '%s' attempted to modify protected table '%s' at key '%s'. Mutation was blocked.",
                tostring(modName or "unknown"),
                tostring(tableName),
                tostring(k)
            )
            if Crabe.write then
                Crabe.write(msg)
            end
            error(msg, 2)
        end,
        __metatable = false
    }
    setmetatable(proxy, mt)
    return proxy
end

--- Creates an isolated sandbox environment table for a mod with protected shared namespaces.
--- Global reads fall back to _G while variable writes remain isolated.
function Crabe.Sandbox.create(modName)
    local env = {}
    env._ENV = env
    env._M = env
    env.modName = modName
    
    local proxyCache = {}
    env.Crabe = makeReadOnlyProxy(Crabe, "Crabe", modName, proxyCache)

    if Game then env.Game = makeReadOnlyProxy(Game, "Game", modName, proxyCache) end
    if table then env.table = makeReadOnlyProxy(table, "table", modName, proxyCache) end
    if string then env.string = makeReadOnlyProxy(string, "string", modName, proxyCache) end
    if math then env.math = makeReadOnlyProxy(math, "math", modName, proxyCache) end
    if coroutine then env.coroutine = makeReadOnlyProxy(coroutine, "coroutine", modName, proxyCache) end
    if os then env.os = makeReadOnlyProxy(os, "os", modName, proxyCache) end
    if debug then env.debug = makeReadOnlyProxy(debug, "debug", modName, proxyCache) end
    env._G = makeReadOnlyProxy(_G, "_G", modName, proxyCache)

    -- The namespaces themselves are still proxies, and Lua 5.1 iteration never
    -- consults a metatable: pairs(Crabe) or pairs(Game) saw no key at all.
    -- The mod's pairs/ipairs/next walk the real table instead, handing back a
    -- protected namespace met on the way as its proxy, as __index does.
    local realOf = {}
    for real, proxy in pairs(proxyCache) do
        realOf[proxy] = real
    end
    local function nextOf(t, k)
        local real = realOf[t]
        if not real then return next(t, k) end
        local key, value = next(real, k)
        if key == nil then return nil end
        return key, proxyCache[value] or value
    end
    env.next = nextOf
    env.pairs = function(t)
        if realOf[t] then return nextOf, t, nil end
        return pairs(t)
    end
    env.ipairs = function(t)
        local real = realOf[t]
        if not real then return ipairs(t) end
        return function(_, i)
            i = i + 1
            local value = real[i]
            if value == nil then return nil end
            return i, proxyCache[value] or value
        end, t, 0
    end

    setmetatable(env, {
        __index = function(t, k)
            return _G[k]
        end,
        __metatable = false
    })
    return env
end

--- Executes a function within the specified sandbox environment.
--- Sets the function environment and forwards any arguments.
function Crabe.Sandbox.run(fn, env, ...)
    local target = fn
    if type(target) == "string" then
        target = assert((loadstring or load)(target))
    end
    if setfenv then
        setfenv(target, env)
    elseif debug and debug.setupvalue then
        local i = 1
        while true do
            local name = debug.getupvalue(target, i)
            if not name then break end
            if name == "_ENV" then
                debug.setupvalue(target, i, env)
                break
            end
            i = i + 1
        end
    end
    return target(...)
end

--- Exports a value or API to the shared Crabe.Exports registry.
--- Facilitates inter-mod communication and public interface exposure.
function Crabe.Sandbox.export(name, value)
    if type(name) ~= "string" or name == "" then
        error("Crabe.Sandbox.export: expected a non-empty string name", 2)
    end
    Crabe.Exports[name] = value
    return value
end
