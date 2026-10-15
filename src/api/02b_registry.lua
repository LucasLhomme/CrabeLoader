-- CrabeLoader
-- File description:
-- Records each revocable subscription against the mod that made it, with the closure undoing it.
-- Anything registered while no mod is executing belongs to core and is never revoked by a reload.
-- Revokes nothing on its own; Crabe.Mod.reload decides when, in src/api/03_lifecycle.lua.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Registry = Crabe.Registry or { _byOwner = {}, _current = nil }

-- Sets the mod whose code is currently executing.
function Crabe.Registry.setCurrentOwner(name)
    Crabe.Registry._current = name
end

-- Returns the mod whose code is currently executing, or nil when none is.
function Crabe.Registry.currentOwner()
    return Crabe.Registry._current
end

-- Records a revocable subscription against the current owner.
function Crabe.Registry.track(revoke)
    if type(revoke) ~= "function" then
        error("Crabe.Registry.track: expected a function", 2)
    end
    local owner = Crabe.Registry._current or "core"
    local list = Crabe.Registry._byOwner[owner]
    if not list then
        list = {}
        Crabe.Registry._byOwner[owner] = list
    end
    list[#list + 1] = revoke
    return revoke
end

-- Revokes every subscription owned by a mod, preserving core subscriptions.
function Crabe.Registry.revokeAllMods()
    for owner, list in pairs(Crabe.Registry._byOwner) do
        if owner ~= "core" then
            for i = #list, 1, -1 do
                local ok, err = pcall(list[i])
                if not ok and Crabe.write then
                    Crabe.write("! [Crabe.Registry] revoke failed for '"
                        .. tostring(owner) .. "': " .. tostring(err))
                end
            end
            Crabe.Registry._byOwner[owner] = nil
        end
    end
end

-- Returns the number of live subscriptions across all owners.
function Crabe.Registry.count()
    local n = 0
    for _, list in pairs(Crabe.Registry._byOwner) do n = n + #list end
    return n
end
