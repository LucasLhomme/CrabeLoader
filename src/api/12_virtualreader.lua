-- VirtualReader roster -- the character-select grid.
--
-- VirtualReaderPC_Data.AvatarData is a plain global table the engine reads to
-- build the grid. It exists only in the front-end/menu Lua state (the game runs
-- one state per screen context, see docs/modding.md); a world state never has
-- it.
--
-- Call these from characters/*.lua, never from a mod's Game.onTick. A row
-- inserted at mod time draws a correct-looking tile, but the game treats it as
-- unowned and opens an upsell screen instead of selecting it -- a native
-- catalog build has already run by then. characters/*.lua runs early enough,
-- right after Presentation/VirtualReaderPC_Data.lua itself finishes.
--
-- What resolves the 3D model:
--
--   sku_id  ->  this table (pushed native by VirtualReaderPC_SetData)
--           ->  Name
--           ->  Name:lower() in the engine's ActorList
--               (assets/gamedb/core/<zone>_actors.lua, Type = "Avatar")
--           ->  that entry's DNAFile  ->  assets/characters/<x>.dnax
--
-- Name is the key, not sku_id. An invented sku_id works; an unknown Name does
-- not. The one rule that must hold: Name:lower() exists in the ActorList with
-- Type = "Avatar". A row naming no actor is the usual cause of "the tile shows
-- but the character never loads".
--
-- Two operations follow from that, kept apart on purpose:
--   addCharacter    -- a new identity over an existing character's model,
--                      borrowing its sku_id. Cosmetic, always works.
--   exposeCharacter -- surface a character whose actor, .dnax, assets and
--                      skill tree already ship, but which has no catalog row.
--                      Its own model, its own abilities.
-- New 3D geometry is out of scope here -- see characters/README.md.
--
-- Selectability is handled separately: a synthetic row is drawn locked, and the
-- gate is plain Lua inside the grid screen (virtualreaderpc_gridcharacter.lua).
-- installGridUnlock() below patches it at runtime; src/api/21_virtualreader_unlock.lua
-- calls it, so callers here never deal with it.

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

-- Adds a row reusing an existing character's model and sku_id, and returns it.
-- Requires entry.Name plus exactly one of entry.baseCharacter (a row's Name,
-- whose sku_id, Icon, ProgressionTree, MetaData and store fields are inherited)
-- or entry.sku_id. Override any inherited field by passing it in `entry`.
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
        -- Ownership is checked against the store fields, not sku_id alone, so
        -- inherit the base character's rather than the empty defaults.
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

-- sku_id derived from a character name, inside the range reserved for mods.
-- Must match Gateway::allocateSku (src/gateway.cpp) exactly, or a row selects
-- an id with no registry slot behind it. Multiply-and-add, never XOR: Lua 5.1
-- has no bitwise operators, and doubles hold these intermediates exactly.
local SKU_LO, SKU_HI = 1000340, 1000999

function Crabe.VirtualReader.skuForName(name)
    local h = 0
    for i = 1, #name do
        h = (h * 31 + string.byte(name, i)) % 2147483648
    end
    return tostring(SKU_LO + h % (SKU_HI - SKU_LO + 1))
end

-- Surfaces a character the game ships in full -- actor row, .dnax, 3D assets
-- and skill tree -- but never gave a catalog row. Unlike addCharacter this is
-- no re-skin: the model, animations, voice and abilities are its own. Returns
-- the row inserted.
--
-- entry.Name             required; must match an ActorList entry with
--                        Type = "Avatar", compared lowercased. It cannot be
--                        checked from here -- the ActorList lives in the gamedb
--                        state -- so verify it against *_actors.lua by hand.
-- entry.sku_id           optional; derived from Name when omitted.
-- entry.ProgressionTree  a real assets/gamedb/core/<x>.lua, lowercased.
-- entry.Icon             optional; the grid falls back to the default icon.
function Crabe.VirtualReader.exposeCharacter(entry)
    if type(entry) ~= "table" or type(entry.Name) ~= "string" or entry.Name == "" then
        error("Crabe.VirtualReader.exposeCharacter: expected a table with at least Name (string), " ..
            "matching an ActorList actor with Type = \"Avatar\"", 2)
    end
    -- Omitted is the normal case: the row and the loader's registry slot then
    -- derive the same id. An explicit one must stay written in the file, since
    -- that is where the loader reads it from.
    if entry.sku_id == nil then
        entry.sku_id = Crabe.VirtualReader.skuForName(entry.Name)
    elseif type(entry.sku_id) ~= "string" or entry.sku_id == "" then
        error("Crabe.VirtualReader.exposeCharacter: entry.sku_id must be a non-empty string " ..
            "(or omitted, to derive one from Name)", 2)
    end
    if entry.baseCharacter then
        error("Crabe.VirtualReader.exposeCharacter: baseCharacter has no meaning here -- this " ..
            "character has its own model. Use addCharacter for a borrowed-model identity", 2)
    end
    if not (VirtualReaderPC_Data and VirtualReaderPC_Data.AvatarData) then
        error("Crabe.VirtualReader.exposeCharacter: VirtualReaderPC_Data.AvatarData not present in " ..
            "this Lua state (call this from characters/*.lua, see characters/README.md)", 2)
    end

    local clash = Crabe.VirtualReader.findCharacter(entry.Name)
    if clash then
        error("Crabe.VirtualReader.exposeCharacter: '" .. entry.Name .. "' is already in the grid", 2)
    end
    clash = Crabe.VirtualReader.findCharacter(entry.sku_id)
    if clash then
        error("Crabe.VirtualReader.exposeCharacter: sku_id " .. entry.sku_id .. " is already used by '" ..
            tostring(clash.Name) .. "' -- pick another", 2)
    end

    local row = {}
    for k, v in pairs(defaultFields) do row[k] = v end
    -- defaultFields points Icon at HUD_PlayerIcons_Default already, which is
    -- exactly the fallback we want when the character has no icon art.
    for k, v in pairs(entry) do row[k] = v end

    table.insert(VirtualReaderPC_Data.AvatarData, row)
    return row
end

-- Mutates an existing row in place -- re-tag, re-icon or relabel a shipped
-- character. sku_id is left untouched, so selection keeps working.
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

-- Disables the grid's lock check so every tile is selectable, real or
-- synthetic. See this file's header for where the gate lives.
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
