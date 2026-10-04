-- CrabeLoader
-- File description:
-- Exposes the Virtual File System (VFS) runtime API under the Crabe.Vfs namespace.
-- Allows mods to query override counts, test asset paths, and read resolution statistics.
-- Defers file redirection and in-memory O(1) hash table lookup to native C++ services.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Vfs = {}

function Crabe.Vfs.count()
    if Crabe._vfsGetOverrideCount then
        return Crabe._vfsGetOverrideCount()
    end
    return 0
end

function Crabe.Vfs.resolve(path)
    if type(path) ~= "string" or path == "" then
        return nil
    end
    if Crabe._vfsResolve then
        return Crabe._vfsResolve(path)
    end
    return nil
end

-- Every override whose virtual path starts with `prefix` (all of them when
-- absent), as { path = "characters/tcw_enemies/x.zip", mod = "<mod folder>" }.
-- Paths are normalized: lowercase, forward slashes. Empty on a loader that
-- predates the native.
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

function Crabe.Vfs.lastRedirected()
    if Crabe._vfsLastRedirected then
        return Crabe._vfsLastRedirected()
    end
    return ""
end
