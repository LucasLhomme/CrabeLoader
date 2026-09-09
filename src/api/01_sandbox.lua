Crabe = Crabe or {}
Crabe.Sandbox = Crabe.Sandbox or {}
Crabe.Exports = Crabe.Exports or {}

--- Creates an isolated sandbox environment table for a mod.
--- Global reads fall back to _G while variable writes remain isolated.
function Crabe.Sandbox.create(modName)
    local env = {}
    env._G = _G
    env._ENV = env
    env._M = env
    env.modName = modName
    env.Crabe = Crabe
    setmetatable(env, { __index = _G })
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
