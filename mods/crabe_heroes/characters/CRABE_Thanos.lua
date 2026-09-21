-- New identity riding on Thor's real model/sku_id (see src/api/12_virtualreader.lua's
-- header for why: no data-driven way to bring in a new 3D model, so a "new"
-- character is always a fresh Name/Icon/Description/MetaData over a real
-- baseCharacter's model). IsLocked/IsTrialPlayable are NOT set here --
-- confirmed live to have no effect either way; selectability comes from
-- Crabe.VirtualReader.installGridUnlock(), installed automatically by
-- src/api/12_virtualreader.lua, not from anything in this row.
Crabe.VirtualReader.addCharacter({
    Name = "Thanos",
    baseCharacter = "AVG_Thor",
    Icon = "HUD_PlayerIcons_IronMan",
    Description = "New identity riding on Thor's model",
    MetaData = "StarWars,Franchise_EMP",
})
