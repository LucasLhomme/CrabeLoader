-- Mace Windu -- his REAL model, not a re-skin of someone else.
--
-- Reference implementation of route B (see README.md): everything except the
-- catalog row ships with the game, so one exposeCharacter call is the whole mod.
--
--   actor      assets/gamedb/core/toybox_actors.lua
--              { Parms = "DNAFile=characters/TCW_MaceWindu.dnax",
--                Type = "Avatar", Name = "tcw_macewindu" }
--   dnax       assets/characters/tcw_macewindu.dnax  (22.5 KB)
--              Identification: ActorTypeMask = "AV_MaceWindu",
--              DialogID = "MCE", LoadAudio = "TCW_MaceWindu"
--   3D assets  assets/characters/tcw_macewindu/  -- 8.1 MB across
--              tcw_macewindu.zip, _robe.zip, _pieces.zip, _teamup.zip, mg.zip
--   abilities  assets/gamedb/core/in3_tcw_macewindu.lua -- a complete tree:
--              MACEWINDU_FINISHER, MACEWINDU_RICOCHET_LIGHTSABER,
--              MACEWINDU_SUPERJUMP, MACEWINDU_CHARGEATTACK, 6 ground combos
--
-- He was used as a mission giver in the Clone Wars playset, which is why the
-- gameplay is all there but the shop-facing art never was.
--
-- No sku_id: the loader derives one from Name (1000444 for this name) and
-- builds the matching figure registry slot in memory before the game reads the
-- gateway, so nothing in the game's own files is touched. Pass sku_id only to
-- pin a specific value.
--
-- Icon: no HUD_PlayerIcons_MaceWindu asset ships, so it is deliberately left
-- unset -- the grid falls back to HUD_PlayerIcons_Default instead of failing.
-- Swap in a real icon name once the art exists (a .mtb in assets/ui/icons/).
--
-- Description and MetaData are ordinary hand-editable fields -- a localisation
-- key and the brand/filter tags. Keeping them here is what makes the file
-- self-contained: share the .lua and nothing else has to travel with it.
--
-- skilltrees/tcw_macewindu.patch restores Type = "Avatar" on the actor row.
-- It is a no-op on a clean install, and on this one too since core.zip takes
-- precedence over the hand-edited loose toybox_actors.lua, but it stands as
-- the worked example of editing an ActorList without touching a game file.
Crabe.VirtualReader.exposeCharacter({
    Name = "TCW_MaceWindu",
    ProgressionTree = "IN3_TCW_MaceWindu",
    Description = "Upsell_TCW_MaceWindu_desc",
    MetaData = "StarWars,Franchise_TCW",
})
