-- Installs the character-select grid unlock (Crabe.VirtualReader.installGridUnlock,
-- src/api/12_virtualreader.lua). Needs its own file loading after 20_hooks.lua:
-- it calls Game.onTick, which 20_hooks.lua defines, and src/api/*.lua loads in
-- filename-numeric order -- calling it from within 12_virtualreader.lua itself
-- failed silently every load (Game.onTick was still nil at that point).
Crabe.VirtualReader.installGridUnlock()
