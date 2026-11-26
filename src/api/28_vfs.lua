-- CrabeLoader
-- File description:
-- Exposes the Virtual File System (VFS) runtime API under the Crabe.Vfs namespace.
-- Allows mods to query override counts, test asset paths, and read resolution statistics.
-- Defers file redirection and in-memory O(1) hash table lookup to native C++ services.
--
-- Authors: @LucasLhomme, @Weresthebeef

Crabe = Crabe or {}
Crabe.Vfs = {}
-- Capability flag: this build serves opted-in loose PC UI .mtb bundles as well as textures.
Crabe.Vfs.supportsLooseMaterials = true
-- Capability flag: startup dependency overlays can be generated from bank_dependencies.json.
Crabe.Vfs.supportsBankDependencyPatches = true

-- Returns total number of registered VFS file overrides.
function Crabe.Vfs.count()
    if Crabe._vfsGetOverrideCount then
        return Crabe._vfsGetOverrideCount()
    end
    return 0
end

-- Resolves virtual asset path to physical file path, or nil.
function Crabe.Vfs.resolve(path)
    if type(path) ~= "string" or path == "" then
        return nil
    end
    if Crabe._vfsResolve then
        return Crabe._vfsResolve(path)
    end
    return nil
end

-- Returns list of active VFS overrides optionally filtered by prefix.
function Crabe.Vfs.list(prefix)
    local rows = {}
    if not Crabe._vfsList then
        return rows
    end
    local text = Crabe._vfsList(type(prefix) == "string" and prefix or nil) or ""
    for path, mod in string.gmatch(text, "([^\t\n]+)\t([^\n]*)\n") do
        rows[#rows + 1] = { path = path, mod = mod }
    end
    return rows
end

-- Returns table containing VFS resolution and cache hit statistics.
function Crabe.Vfs.stats()
    if Crabe._vfsGetStats then
        local overrides, resolutions, hits = Crabe._vfsGetStats()
        return {
            totalOverrides = overrides or 0,
            totalResolutions = resolutions or 0,
            totalHits = hits or 0,
        }
    end
    return { totalOverrides = 0, totalResolutions = 0, totalHits = 0 }
end

-- Returns the most recently redirected file path string.
function Crabe.Vfs.lastRedirected()
    if Crabe._vfsLastRedirected then
        return Crabe._vfsLastRedirected()
    end
    return ""
end

-- Explains an asset's providers, resolved order and selected winner without touching disk.
function Crabe.Vfs.explain(path)
    if type(path) ~= "string" or path == "" then return nil end
    local result = { path = path, providers = {}, reason = "no mod override indexed" }
    if not Crabe._vfsExplain then return result end
    local text = Crabe._vfsExplain(path) or ""
    for line in string.gmatch(text, "[^\n]+") do
        local id, folder, physical, order, mount, alias, winner, reason =
            string.match(line, "([^\t]*)\t([^\t]*)\t([^\t]*)\t([^\t]*)\t([^\t]*)\t([^\t]*)\t([^\t]*)\t(.*)")
        if id then
            local entry = { mod = id, folder = folder, physicalPath = physical,
                order = tonumber(order), mount = mount, alias = alias == "1", reason = reason }
            result.providers[#result.providers + 1] = entry
            if winner == "1" then result.winner = entry; result.reason = reason end
        end
    end
    return result
end

-- Lists full explanations for conflicting assets, optionally restricted to a prefix.
function Crabe.Vfs.conflicts(prefix)
    local result = {}
    if not Crabe._vfsConflicts then return result end
    prefix = type(prefix) == "string" and string.lower(string.gsub(prefix, "\\", "/")) or ""
    for path in string.gmatch(Crabe._vfsConflicts() or "", "[^\n]+") do
        if string.sub(path, 1, #prefix) == prefix then result[#result + 1] = Crabe.Vfs.explain(path) end
    end
    return result
end

-- Prints an explanation in the console and returns the structured result for author tools.
function Crabe.Vfs.printExplanation(path)
    local detail = Crabe.Vfs.explain(path)
    if not detail then return nil end
    print("VFS " .. detail.path .. ": " .. detail.reason)
    for _, entry in ipairs(detail.providers) do
        print((entry == detail.winner and "  WINNER " or "  shadowed ") .. entry.mod ..
            " [order " .. tostring(entry.order) .. ", " .. entry.mount .. "] -> " .. entry.physicalPath)
    end
    return detail
end
