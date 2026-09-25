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
