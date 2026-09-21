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
local function makeReadOnlyProxy(realTable, tableName, modName)
    if type(realTable) ~= "table" then return realTable end
    local proxy = {}
    local mt = {
        __index = realTable,
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
    env.Crabe = Crabe

    if Game then env.Game = makeReadOnlyProxy(Game, "Game", modName) end
    if table then env.table = makeReadOnlyProxy(table, "table", modName) end
    if string then env.string = makeReadOnlyProxy(string, "string", modName) end
    if math then env.math = makeReadOnlyProxy(math, "math", modName) end
    if coroutine then env.coroutine = makeReadOnlyProxy(coroutine, "coroutine", modName) end
    if os then env.os = makeReadOnlyProxy(os, "os", modName) end
    if debug then env.debug = makeReadOnlyProxy(debug, "debug", modName) end
    env._G = makeReadOnlyProxy(_G, "_G", modName)

    setmetatable(env, {
        __index = function(t, k)
            return _G[k]
        end
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
