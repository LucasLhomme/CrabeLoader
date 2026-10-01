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

--- Creates a read-only proxy table blocking writes with security logging.
--- Now uses deep proxying and a cache to preserve reference equality and prevent cyclic loops.
local function makeReadOnlyProxy(realTable, tableName, modName, cache)
    if type(realTable) ~= "table" then return realTable end
    cache = cache or {}
    if cache[realTable] then return cache[realTable] end

    local proxy = {}
    cache[realTable] = proxy
    
    local mt = {
        __index = function(t, k)
            local v = realTable[k]
            if type(v) == "table" then
                return makeReadOnlyProxy(v, tableName .. "." .. tostring(k), modName, cache)
            end
            return v
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

--- Creates an isolated sandbox environment table for a mod with deep-frozen protection.
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
