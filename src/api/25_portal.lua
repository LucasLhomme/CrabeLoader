-- CrabeLoader
-- File description:
-- Physical and virtual Disney Infinity base: status, RGB LEDs, virtual figures and figure events.
-- Primitives only: what a figure does in the game is the business of the mod listening.
-- Reference: docs/guides/portal.md.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Portal = Crabe.Portal or {}

-- True when a physical Disney Infinity base is connected and answering.
function Crabe.Portal.isConnected()
    if type(Crabe._portalIsConnected) ~= "function" then
        return false
    end
    return Crabe._portalIsConnected()
end

-- Connection, base type ("hid", "xbox360", "xboxone" or "none") and the figure SKU of each slot.
-- diagnostic tells the player what is missing when no base answers, nil otherwise.
function Crabe.Portal.getStatus()
    if type(Crabe._portalGetStatus) ~= "function" then
        return {
            connected = false,
            type = "none",
            typeDescription = "Unavailable",
            diagnostic = "This CrabeLoader build has no portal support.",
        }
    end

    local connected, typeDesc, s1, s2, s3, diag, typeName = Crabe._portalGetStatus()
    return {
        connected = connected,
        type = typeName or "none",
        typeDescription = typeDesc or "",
        slot1Sku = (s1 and s1 > 0) and s1 or nil,
        slot2Sku = (s2 and s2 > 0) and s2 or nil,
        slot3Sku = (s3 and s3 > 0) and s3 or nil,
        diagnostic = (diag and diag ~= "") and diag or nil,
    }
end

-- Sets the RGB LED of a slot: 0 = all, 1 = Player 1, 2 = Player 2, 3 = playset / hexagon.
-- False when no base is connected.
function Crabe.Portal.setLed(slot, r, g, b)
    if type(Crabe._portalSetLed) ~= "function" then
        return false
    end
    return Crabe._portalSetLed(slot or 0, r or 0, g or 0, b or 0)
end

-- Reports a virtual figure on a slot, exactly as a base would report a real one.
-- sku is a character SKU number or a character name known to Crabe.VirtualReader.
-- False when the SKU cannot be resolved.
function Crabe.Portal.placeVirtualFigure(slot, sku)
    if type(Crabe._portalEmulatePlace) ~= "function" then
        return false
    end
    local numSku = tonumber(sku)
    if not numSku and type(sku) == "string" and Crabe.VirtualReader and Crabe.VirtualReader.skuForName then
        numSku = tonumber(Crabe.VirtualReader.skuForName(sku))
    end
    if not numSku or numSku <= 0 then
        return false
    end
    return Crabe._portalEmulatePlace(slot or 1, numSku)
end

-- Reports the figure of a slot as removed, virtual or not.
function Crabe.Portal.removeVirtualFigure(slot)
    if type(Crabe._portalEmulateRemove) ~= "function" then
        return false
    end
    return Crabe._portalEmulateRemove(slot or 1)
end

-- Set skill bit numbers ({ n, ... }) from the tag's 8 skill bytes in hex.
-- Bit n is byte[n / 8] & (1 << (n % 8)), the layout the engine and the figure share.
function Crabe.Portal.decodeSkillBits(hex)
    local bits = {}
    for i = 0, 7 do
        local byte = tonumber(string.sub(hex, i * 2 + 1, i * 2 + 2), 16) or 0
        for b = 0, 7 do
            if math.floor(byte / 2 ^ b) % 2 == 1 then
                bits[#bits + 1] = i * 8 + b
            end
        end
    end
    return bits
end

-- The tag's 8 skill bytes in hex from set skill bit numbers ({ n, ... }).
-- Numbers outside 0..63 are ignored.
function Crabe.Portal.encodeSkillBits(bits)
    local bytes = { 0, 0, 0, 0, 0, 0, 0, 0 }
    for _, n in ipairs(bits or {}) do
        if n >= 0 and n < 64 then
            local i = math.floor(n / 8) + 1
            local mask = 2 ^ (n % 8)
            if math.floor(bytes[i] / mask) % 2 == 0 then
                bytes[i] = bytes[i] + mask
            end
        end
    end
    local hex = {}
    for i = 1, 8 do
        hex[i] = string.format("%02x", bytes[i])
    end
    return table.concat(hex)
end

-- Saves progression ({ level, experience, actionFlags, lastPlayed, skillBits }) onto the figure of slot 1 or 2.
-- Writes the older of the figure's two copies and reads it back, as the console does; nothing else is written.
-- True when a write was queued, false when no physical figure is there or nothing changed.
function Crabe.Portal.writeFigureProgress(slot, progress)
    if type(Crabe._portalWriteProgress) ~= "function" or type(progress) ~= "table" then
        return false
    end
    return Crabe._portalWriteProgress(slot, progress.level or 0, progress.experience or 0,
        Crabe.Portal.encodeSkillBits(progress.skillBits), progress.actionFlags or 0, progress.lastPlayed or 0)
end

-- Drains the base's events and emits "portalFigurePlaced" (slot, sku, uid, progress) or "portalFigureRemoved" (slot, uid).
-- uid is the figure's UID in hex, "" for a virtual figure; progress is nil unless a physical character figure saved one.
-- Runs on every game tick.
function Crabe.Portal.update()
    if type(Crabe._portalPollEvent) ~= "function" or not (Crabe.Events and Crabe.Events.emit) then
        return
    end
    while true do
        local eventType, slot, sku, uid, level, experience, skillHex, actionFlags, lastPlayed = Crabe._portalPollEvent()
        if not eventType or eventType == 0 then
            break
        end
        if eventType == 1 then
            local progress = nil
            if level then
                progress = {
                    level = level,
                    experience = experience,
                    actionFlags = actionFlags,
                    lastPlayed = lastPlayed,
                    skillBits = Crabe.Portal.decodeSkillBits(skillHex or ""),
                }
            end
            Crabe.Events.emit("portalFigurePlaced", slot, sku, uid or "", progress)
        elseif eventType == 2 then
            Crabe.Events.emit("portalFigureRemoved", slot, uid or "")
        end
    end
end

if Game and type(Game.onTick) == "function" then
    Game.onTick(Crabe.Portal.update)
elseif Crabe.Events and type(Crabe.Events.on) == "function" then
    Crabe.Events.on("tick", Crabe.Portal.update)
end
