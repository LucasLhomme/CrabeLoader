-- `Crabe.Registry` - Ownership registry for revocable subscriptions.
--
-- Every registration a mod makes (onTick, onDeath, event listeners, reload
-- callbacks) is recorded here against the mod that made it, together with a
-- closure that undoes exactly that one registration. Hot reload then becomes
-- revocation: Crabe.Mod.reload() calls revokeAllMods(), every mod-owned
-- subscription is removed, and the mod files are re-executed from disk by
-- C++ (ModManager::reloadAllMods) to register fresh ones.
--
-- Registrations made while no mod is executing -- the API's own modules at
-- injection time -- fall to the "core" owner and are never revoked.

Crabe = Crabe or {}
Crabe.Registry = Crabe.Registry or { _byOwner = {}, _current = nil }

--- Sets the mod whose code is currently executing. The C++ loader sets this
--- before running a mod chunk and clears it after.
function Crabe.Registry.setCurrentOwner(name)
    Crabe.Registry._current = name
end

--- Records a revocable subscription against the current owner.
--- `revoke` is called with no arguments when that owner is unloaded.
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

--- Revokes every subscription owned by a mod. Subscriptions owned by "core"
--- are never revoked -- they belong to the API itself and survive reloads.
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

--- Number of live subscriptions, for diagnostics and tests.
function Crabe.Registry.count()
    local n = 0
    for _, list in pairs(Crabe.Registry._byOwner) do n = n + #list end
    return n
end
