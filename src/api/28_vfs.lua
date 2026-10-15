-- CrabeLoader
-- File description:
-- Exposes the Virtual File System (VFS) runtime API under the Crabe.Vfs namespace.
-- Allows mods to query override counts, test asset paths, and read resolution statistics.
-- Defers file redirection and in-memory O(1) hash table lookup to native C++ services.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Vfs = {}

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
