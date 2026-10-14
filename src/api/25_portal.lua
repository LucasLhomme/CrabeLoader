-- CrabeLoader
-- File description:
-- Physical & Virtual Portal subsystem: primitives only.
-- Status, RGB LEDs, virtual figures, and figure events. What a figure does in the game is the
-- business of the mod listening to the events, never of this API.
--
-- Events (Crabe.Events):
--   "portalFigurePlaced"  (slot, sku, uid, progress)  slot 1 = Player 1, 2 = Player 2, 3 = Playset / hexagon
--   "portalFigureRemoved" (slot, uid)                 uid is the 7-byte RFID UID as hex text ("" if virtual)
-- progress is what a physical character figure has saved (nil for virtual figures and pieces):
--   { level, experience, actionFlags, lastPlayed, skillBits = { n, ... } }  skillBits: set bit numbers.
-- Crabe.Portal.writeFigureProgress saves such a table back onto the figure, the way the console does
-- (older of the two copies, verified by read-back); nothing else on the figure is ever written.
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

-- Status of the portal: connection, base type, figures in each slot, diagnostic for the player.
-- type: "hid", "xbox360", "xboxone" or "none".
function Crabe.Portal.getStatus()
    if type(Crabe._portalGetStatus) ~= "function" then
        return {
            connected = false,
            type = "none",
            typeDescription = "Non disponible",
            diagnostic = "Natif de portail non présent dans ce runtime."
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
        diagnostic = (diag and diag ~= "") and diag or nil
    }
end

-- RGB LEDs of the physical base. slot: 0 = all, 1 = Player 1, 2 = Player 2, 3 = Playset / hexagon.
-- False when no base is connected.
function Crabe.Portal.setLed(slot, r, g, b)
    if type(Crabe._portalSetLed) ~= "function" then
        return false
    end
    return Crabe._portalSetLed(slot or 0, r or 0, g or 0, b or 0)
end

-- Places a virtual figure. sku: character SKU number, or a character name known to
-- Crabe.VirtualReader. False when the SKU cannot be resolved.
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

-- Removes the figure (virtual or not) from a slot.
function Crabe.Portal.removeVirtualFigure(slot)
    if type(Crabe._portalEmulateRemove) ~= "function" then
        return false
    end
    return Crabe._portalEmulateRemove(slot or 1)
end

-- Skill bits travel as the tag's 8 bytes in hex; bit n = byte[n / 8] & (1 << (n % 8)).
local function decodeSkillBits(hex)
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

local function encodeSkillBits(bits)
    local bytes = { 0, 0, 0, 0, 0, 0, 0, 0 }
    for _, n in ipairs(bits or {}) do
        if n >= 0 and n < 64 then
            local i = math.floor(n / 8) + 1
            local mask = 2 ^ (n % 8)
            if math.floor(bytes[i] / mask) % 2 == 0 then bytes[i] = bytes[i] + mask end
        end
    end
    local hex = {}
    for i = 1, 8 do hex[i] = string.format("%02x", bytes[i]) end
    return table.concat(hex)
end

-- Saves progression onto the physical figure of slot 1 or 2. Only the fields that changed are
-- written. True when a write was queued (false: no physical figure there, or nothing changed).
function Crabe.Portal.writeFigureProgress(slot, progress)
    if type(Crabe._portalWriteProgress) ~= "function" or type(progress) ~= "table" then
        return false
    end
    return Crabe._portalWriteProgress(slot, progress.level or 0, progress.experience or 0,
        encodeSkillBits(progress.skillBits), progress.actionFlags or 0, progress.lastPlayed or 0)
end

-- Drains the native event queue and re-emits each event on Crabe.Events.
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
                    skillBits = decodeSkillBits(skillHex or ""),
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
