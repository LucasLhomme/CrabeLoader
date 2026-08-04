-- VirtualReader roster. VirtualReaderPC_Data.AvatarData (game Lua, loaded by
-- Presentation/VirtualReaderPC_Data.lua) is a plain global table the engine
-- reads to populate the character-select grid. It only exists in the
-- front-end/menu Lua state (the game runs one Lua state per screen context,
-- see docs/modding.md); a world/gameplay state never has it.
--
-- Call these from characters/*.lua, not from a mod's Game.onTick: confirmed
-- live that a mod-time table.insert draws a correct-looking tile (name/icon/
-- description) but the game still treats it as unowned -- an upsell/purchase
-- screen instead of selecting it, some native-side catalog build has already
-- run by the time a mod gets to run. characters/*.lua runs earlier (right
-- after VirtualReaderPC_Data.lua itself finishes), see characters/README.md.
--
-- sku_id resolution is the other hard constraint, confirmed live: the grid
-- (VirtualReaderPC_GetItemByPage) reads this same Lua table for display, so a
-- made-up sku_id still draws a correct-looking tile -- but selecting it calls
-- VirtualReaderPC_SetCurrentCharacter(sku_id), which resolves against a
-- separate, native-side catalog of real characters and falls back to
-- whatever avatar was last active if the id isn't in it. There is no
-- data-driven way to introduce a brand new 3D model. A "new" character is
-- therefore always: a new identity (Name/Icon/Description/MetaData) and its
-- own skill tree (ProgressionTree, see skilltrees/), riding on an existing,
-- real character's model via a borrowed sku_id (entry.baseCharacter below) --
-- exactly how this has historically been done in the DI3 modding scene.
-- A synthetic Name shows a real icon only if entry.Icon names an existing
-- HUD_PlayerIcons_* asset (reuse the base character's, or another real
-- character's); a genuinely new icon image is an asset-drop question, not
-- something this API can do on its own.
--
-- A synthetic row is otherwise a normal-looking tile that the grid still
-- shows as locked (upsell/purchase screen on click) -- three row fields were
-- tried and confirmed live to have no effect (IsLocked/IsTrialPlayable,
-- SteamDLCAppId/PCSKU/WINRTSKU, calling VirtualReaderPC_SetCurrentCharacter
-- directly). The actual gate turned out to be plain Lua in the grid SCREEN
-- itself (presentation/virtualreaderpc_gridcharacter.lua's DoSelectGridItem/
-- GetGridButtonState), found by diffing that file's vanilla decompile
-- against the shipped "Breeze" DI3 mod's patched copy of the same file:
-- Breeze hardcodes the lock check to false instead of reading it back from
-- the grid's list data. installGridUnlock() below ports that patch as a
-- runtime monkey-patch (same technique mods/window_mode.lua uses on
-- SettingsVideo:BuildList), called unconditionally from
-- src/api/21_virtualreader_unlock.lua (see that file for why not here) --
-- addCharacter's callers don't need to know any of this exists.

Crabe.VirtualReader = Crabe.VirtualReader or {}

local defaultFields = {
    SteamDLCAppId = "",
    PCSKU = "",
    WINRTSKU = "",
    Icon = "HUD_PlayerIcons_Default",
    Description = "",
    VideoLink = "",
    ProgressionTree = "",
    CostumeCoin = "",
    MetaData = "Disney,Franchise_DIS",
}

-- Live listing, mostly useful to check whether a sku_id/Name is already
-- taken before adding one. nil in a Lua state without VirtualReaderPC_Data.
function Crabe.VirtualReader.listCharacters()
    if not (VirtualReaderPC_Data and VirtualReaderPC_Data.AvatarData) then
        return nil
    end
    return VirtualReaderPC_Data.AvatarData
end

-- Finds an existing row by Name or sku_id (nil if none).
function Crabe.VirtualReader.findCharacter(nameOrSkuId)
    local list = Crabe.VirtualReader.listCharacters()
    if not list then return nil end
    for _, row in ipairs(list) do
        if row.Name == nameOrSkuId or row.sku_id == nameOrSkuId then
            return row
        end
    end
    return nil
end

-- entry.Name (string) is required. For the sku_id (the one field that must
-- resolve to a real character, see file header), pass exactly one of:
--   entry.baseCharacter = "AVG_Thor"   -- Name of an existing row; its
--                                         sku_id is copied automatically
--   entry.sku_id = "1000103"           -- a real sku_id directly
-- ProgressionTree should name a real gamedb/core entry for this new identity
-- to have its own abilities (see skilltrees/README.md) -- reusing the base
-- character's own ProgressionTree is fine too and is the default if
-- baseCharacter is given and ProgressionTree is not.
-- Returns the row actually inserted.
function Crabe.VirtualReader.addCharacter(entry)
    if type(entry) ~= "table" or type(entry.Name) ~= "string" or entry.Name == "" then
        error("Crabe.VirtualReader.addCharacter: expected a table with at least Name (string)", 2)
    end
    if not (VirtualReaderPC_Data and VirtualReaderPC_Data.AvatarData) then
        error("Crabe.VirtualReader.addCharacter: VirtualReaderPC_Data.AvatarData not present in this Lua state " ..
            "(only the front-end/menu state has it -- call this from Game.onTick and check " ..
            "VirtualReaderPC_Data ~= nil first, see docs/modding.md)", 2)
    end

    local row = {}
    for k, v in pairs(defaultFields) do row[k] = v end

    local skuId = entry.sku_id
    if entry.baseCharacter then
        local base = Crabe.VirtualReader.findCharacter(entry.baseCharacter)
        if not base then
            error("Crabe.VirtualReader.addCharacter: baseCharacter '" .. tostring(entry.baseCharacter) ..
                "' not found (check Crabe.VirtualReader.listCharacters() for real Name/sku_id values)", 2)
        end
        skuId = skuId or base.sku_id
        row.Icon = base.Icon
        row.ProgressionTree = base.ProgressionTree
        row.MetaData = base.MetaData
        -- Entitlement/ownership is plausibly checked against one of these
        -- (Steam DLC ownership, PC/WinRT store SKU), not sku_id alone --
        -- confirmed live that sku_id alone still leaves the tile locked
        -- (see characters/CRABE_Thanos.lua). defaultFields left them at ""
        -- for a synthetic row; ride the base character's real values instead.
        row.SteamDLCAppId = base.SteamDLCAppId
        row.PCSKU = base.PCSKU
        row.WINRTSKU = base.WINRTSKU
    end

    if type(skuId) ~= "string" or skuId == "" then
        error("Crabe.VirtualReader.addCharacter: no sku_id -- pass entry.baseCharacter (an existing " ..
            "character's Name, to borrow its model) or entry.sku_id (a real one) directly", 2)
    end

    for k, v in pairs(entry) do
        if k ~= "baseCharacter" then row[k] = v end
    end
    row.sku_id = skuId

    table.insert(VirtualReaderPC_Data.AvatarData, row)
    return row
end

-- Mutates fields on an EXISTING character's row in place -- e.g. add
-- "StarWars" to MetaData so it shows up in that category/filter, or
-- relabel/re-icon it. sku_id is left untouched, so selection always keeps
-- working; this is the reliable way to change how a real character is
-- presented without any of addCharacter's sku_id caveats.
function Crabe.VirtualReader.editCharacter(nameOrSkuId, patch)
    if type(patch) ~= "table" then
        error("Crabe.VirtualReader.editCharacter: patch must be a table", 2)
    end
    local row = Crabe.VirtualReader.findCharacter(nameOrSkuId)
    if not row then
        error("Crabe.VirtualReader.editCharacter: no character found for '" .. tostring(nameOrSkuId) .. "'", 2)
    end
    for k, v in pairs(patch) do row[k] = v end
    return row
end

-- Disables the character-select grid's lock check (see this file's header
-- for how that was found) so every tile is selectable, real or synthetic.
-- VirtualReaderPC_GridCharacter only exists once the character-select
-- screen has been opened at least once, hence the poll; re-checks the
-- identity in case the screen's class table gets reassigned on reload.
-- Bodies are copied verbatim from Breeze's patched file, not reinvented, to
-- match a change already proven across its userbase rather than improvise
-- a new one.
local gridUnlockPatchedClass = nil

local function patchGridCharacter()
    local cls = rawget(_G, "VirtualReaderPC_GridCharacter")
    if type(cls) ~= "table" then return false end
    if gridUnlockPatchedClass == cls then return true end

    if type(cls.DoSelectGridItem) ~= "function" or type(cls.GetGridButtonState) ~= "function" then
        return false
    end

    function cls:GetGridButtonState(info)
        local state = "available"
        if info.IsDead then
            state = "dead"
        end
        return state
    end

    function cls:DoSelectGridItem()
        if self:IsOnboardTutorialActive() then
            local onboardState = UI_GetOnboardTutorialStateString()
            if onboardState ~= "ToyboxCharacterSelection" then
                return
            end
        end
        if self.selectedCharacter ~= nil and self.selectedCharacter ~= "" then
            self.IsTypingFilter = false
            if self:IsOnboardTutorialActive() then
                local onboardState = UI_GetOnboardTutorialStateString()
                if onboardState == "ToyboxCharacterSelection" then
                    UI_SetOnboardTutorialState("ToyboxLoadoutPlay")
                end
            end
            Sfx_Play2D("GenericList_Cancel")
            local newSkuId = self:GetItemData(tonumber(self.selectedCharacter), "skuId")
            VirtualReaderPC_SetCurrentCharacter(newSkuId)
            self:ExitScreen()
            self:ReturnToLoadout()
        end
    end

    gridUnlockPatchedClass = cls
    return true
end

-- No "already patched, skip" outer check on the caller's side: the sibling
-- (much more RE'd) DI3 mod project notes VirtualReaderPC_Loadout gets
-- reassigned to a fresh table on screen reload, and the same is plausible
-- here, so every tick re-checks identity via patchGridCharacter()'s own
-- `gridUnlockPatchedClass == cls` comparison -- that's what keeps repeated
-- calls cheap once nothing has changed, not a call-once guard.
--
-- NOT called here: this module (12_virtualreader.lua) loads before
-- 20_hooks.lua (src/api/*.lua loads in filename-numeric order, confirmed in
-- lua_runtime.cpp), which is where Game.onTick is defined -- calling it here
-- would fail with "attempt to call a nil value" every single load, silently
-- (the loader logs and moves on to the next module). Call this from a module
-- that loads after 20_hooks.lua instead (see src/api/21_virtualreader_unlock.lua).
function Crabe.VirtualReader.installGridUnlock()
    Game.onTick(patchGridCharacter)
    patchGridCharacter()
end
