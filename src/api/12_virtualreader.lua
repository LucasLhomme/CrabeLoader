-- CrabeLoader
-- File description:
-- The VirtualReader roster: the character select grid, and adding or editing an entry in it.
-- A character is keyed by sku, and a name lookup exists only because skus are unreadable by hand.
-- Does not swap the played character -- Game.SetCharacter in src/api/11_avatar.lua does.
--
-- Authors: @LucasLhomme

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

local pendingExposes = {}
local pendingAdds = {}

-- Flushes pending character exposures and additions into AvatarData.
function Crabe.VirtualReader._flushPending(targetAvatarData)
    local vrData = rawget(_G, "VirtualReaderPC_Data")
    local avatarData = targetAvatarData or (vrData and vrData.AvatarData)
    if type(avatarData) ~= "table" then return false end

    if #pendingExposes > 0 then
        local toProcess = pendingExposes
        pendingExposes = {}
        for _, entry in ipairs(toProcess) do
            local alreadyThere = false
            for _, row in ipairs(avatarData) do
                if row.Name == entry.Name or row.sku_id == entry.sku_id then
                    alreadyThere = true
                    break
                end
            end
            if not alreadyThere then
                local row = {}
                for k, v in pairs(defaultFields) do row[k] = v end
                for k, v in pairs(entry) do row[k] = v end
                table.insert(avatarData, row)
            end
        end
    end

    if #pendingAdds > 0 then
        local toProcess = pendingAdds
        pendingAdds = {}
        for _, entry in ipairs(toProcess) do
            local alreadyThere = false
            for _, row in ipairs(avatarData) do
                if row.Name == entry.Name then
                    alreadyThere = true
                    break
                end
            end
            if not alreadyThere then
                local row = {}
                for k, v in pairs(defaultFields) do row[k] = v end
                local skuId = entry.sku_id
                if entry.baseCharacter then
                    for _, r in ipairs(avatarData) do
                        if r.Name == entry.baseCharacter or r.sku_id == entry.baseCharacter then
                            skuId = skuId or r.sku_id
                            row.Icon = r.Icon
                            row.ProgressionTree = r.ProgressionTree
                            row.MetaData = r.MetaData
                            row.SteamDLCAppId = r.SteamDLCAppId
                            row.PCSKU = r.PCSKU
                            row.WINRTSKU = r.WINRTSKU
                            break
                        end
                    end
                end
                for k, v in pairs(entry) do
                    if k ~= "baseCharacter" then row[k] = v end
                end
                row.sku_id = skuId
                table.insert(avatarData, row)
            end
        end
    end
    return true
end

local native_SetData = nil

-- Ensures VirtualReaderPC_SetData is hooked to flush pending entries.
local function ensureSetDataHooked()
    if native_SetData then return end
    local candidate = rawget(_G, "VirtualReaderPC_SetData")
    if type(candidate) == "function" then
        native_SetData = candidate
        _G.VirtualReaderPC_SetData = function(data)
            if type(data) == "table" and type(rawget(data, "AvatarData")) == "table" then
                if not rawget(_G, "VirtualReaderPC_Data") then
                    rawset(_G, "VirtualReaderPC_Data", data)
                end
                Crabe.VirtualReader._flushPending(data.AvatarData)
            else
                Crabe.VirtualReader._flushPending()
            end
            return native_SetData(data)
        end
    end
end
ensureSetDataHooked()

-- Returns live roster AvatarData list, or nil if unavailable.
function Crabe.VirtualReader.listCharacters()
    Crabe.VirtualReader._flushPending()
    if not (VirtualReaderPC_Data and VirtualReaderPC_Data.AvatarData) then
        return nil
    end
    return VirtualReaderPC_Data.AvatarData
end

-- Finds an existing roster row by Name or sku_id, or returns nil.
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

-- Adds a character row borrowing a base character model and SKU.
function Crabe.VirtualReader.addCharacter(entry)
    if type(entry) ~= "table" or type(entry.Name) ~= "string" or entry.Name == "" then
        error("Crabe.VirtualReader.addCharacter: expected a table with at least Name (string)", 2)
    end

    if not (VirtualReaderPC_Data and VirtualReaderPC_Data.AvatarData) then
        table.insert(pendingAdds, entry)
        return entry
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

-- Returns the SKU id allocated for the specified character name.
function Crabe.VirtualReader.skuForName(name)
    local skus = Crabe.VirtualReader._skus
    if skus then
        if skus[name] then return skus[name] end
        if type(name) == "string" then
            local lowerName = string.lower(name)
            for k, v in pairs(skus) do
                if type(k) == "string" and string.lower(k) == lowerName then
                    return v
                end
            end
        end
    end
    if type(name) == "string" and name ~= "" then
        local sku = nil
        if type(Crabe._allocateSku) == "function" then
            sku = Crabe._allocateSku(name)
        else
            local h = 0
            for i = 1, #name do
                local b = string.byte(name, i)
                h = (h * 31 + b) % 2147483648
            end
            sku = tostring(1000340 + (h % 660))
        end
        Crabe.VirtualReader._skus = Crabe.VirtualReader._skus or {}
        Crabe.VirtualReader._skus[name] = sku
        return sku
    end
    return nil
end

-- Returns list of custom and modded characters found in the roster.
function Crabe.VirtualReader.getModdedCharacters()
    local list = Crabe.VirtualReader.listCharacters()
    local custom = {}
    local seen = {}
    if list then
        for _, row in ipairs(list) do
            local skuNum = tonumber(row.sku_id)
            if skuNum and ((skuNum >= 1000340 and skuNum <= 1000999) or skuNum >= 2000000) then
                table.insert(custom, row)
                seen[row.Name] = true
            end
        end
    end
    if pendingAdds then
        for _, entry in ipairs(pendingAdds) do
            if entry.Name and not seen[entry.Name] then
                table.insert(custom, {
                    Name = entry.Name,
                    sku_id = entry.sku_id or Crabe.VirtualReader.skuForName(entry.Name),
                    Icon = entry.Icon or "default",
                })
                seen[entry.Name] = true
            end
        end
    end
    if Crabe.VirtualReader._skus then
        for name, sku in pairs(Crabe.VirtualReader._skus) do
            if not seen[name] then
                table.insert(custom, {
                    Name = name,
                    sku_id = sku,
                    Icon = "default",
                })
                seen[name] = true
            end
        end
    end
    return custom
end

-- Exposes an unlisted engine character in the character select grid.
function Crabe.VirtualReader.exposeCharacter(entry)
    if type(entry) ~= "table" or type(entry.Name) ~= "string" or entry.Name == "" then
        error("Crabe.VirtualReader.exposeCharacter: expected a table with at least Name (string), " ..
            "matching an ActorList actor with Type = \"Avatar\"", 2)
    end
    if entry.sku_id == nil then
        entry.sku_id = Crabe.VirtualReader.skuForName(entry.Name)
        if entry.sku_id == nil then
            error("Crabe.VirtualReader.exposeCharacter: could not resolve sku_id for '" ..
                tostring(entry.Name) .. "'", 2)
        end
    elseif type(entry.sku_id) ~= "string" or entry.sku_id == "" then
        error("Crabe.VirtualReader.exposeCharacter: entry.sku_id must be a non-empty string " ..
            "(or omitted, to derive one from Name)", 2)
    end
    if entry.baseCharacter then
        error("Crabe.VirtualReader.exposeCharacter: baseCharacter has no meaning here -- this " ..
            "character has its own model. Use addCharacter for a borrowed-model identity", 2)
    end

    if not (VirtualReaderPC_Data and VirtualReaderPC_Data.AvatarData) then
        table.insert(pendingExposes, entry)
        return entry
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
    for k, v in pairs(entry) do row[k] = v end

    table.insert(VirtualReaderPC_Data.AvatarData, row)
    return row
end

-- Mutates an existing character roster row in place.
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

local gridUnlockPatchedClass = nil

-- Patches grid character selection to unlock all character tiles.
local function patchGridCharacter()
    ensureSetDataHooked()
    Crabe.VirtualReader._flushPending()

    local vrpc = rawget(_G, "VirtualReaderPC")
    if type(vrpc) == "table" and type(vrpc.IsOnboardTutorialActive) == "function" then
        vrpc.IsOnboardTutorialActive = function() return false end
    end

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

-- Installs tick hook to continuously patch grid character unlock.
function Crabe.VirtualReader.installGridUnlock()
    Game.onTick(patchGridCharacter)
    patchGridCharacter()
end
