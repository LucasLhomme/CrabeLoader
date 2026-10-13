-- CrabeLoader
-- File description:
-- Physical & Virtual Portal subsystem:
-- Hardware portal status, RGB LED control, event dispatch, and automatic character swapping.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Portal = Crabe.Portal or {}

-- Checks if a physical Disney Infinity portal is connected and active.
function Crabe.Portal.isConnected()
    if type(Crabe._portalIsConnected) ~= "function" then
        return false
    end
    return Crabe._portalIsConnected()
end

-- Retrieves detailed status of the portal (connection, type, placed figures, and diagnostic).
function Crabe.Portal.getStatus()
    if type(Crabe._portalGetStatus) ~= "function" then
        return {
            connected = false,
            type = "None",
            typeDescription = "Non disponible",
            slot1Sku = nil,
            slot2Sku = nil,
            slot3Sku = nil,
            diagnostic = "Natif de portail non présent dans ce runtime."
        }
    end

    local connected, typeDesc, s1, s2, s3, diag = Crabe._portalGetStatus()
    return {
        connected = connected,
        typeDescription = typeDesc or "",
        slot1Sku = (s1 and s1 > 0) and s1 or nil,
        slot2Sku = (s2 and s2 > 0) and s2 or nil,
        slot3Sku = (s3 and s3 > 0) and s3 or nil,
        diagnostic = (diag and diag ~= "") and diag or nil
    }
end

-- Controls RGB LEDs on the physical base.
-- slot: 0 = All, 1 = Player 1, 2 = Player 2, 3 = Playset / Hexagon
function Crabe.Portal.setLed(slot, r, g, b)
    if type(Crabe._portalSetLed) ~= "function" then
        return false
    end
    return Crabe._portalSetLed(slot or 0, r or 0, g or 0, b or 0)
end

-- Places a virtual figure on the portal (virtual base for all players).
-- slot: 1 (P1), 2 (P2), 3 (Hex)
-- sku: Character SKU ID (e.g. 1000200 for Anakin) or character name
function Crabe.Portal.placeVirtualFigure(slot, sku)
    if type(Crabe._portalEmulatePlace) ~= "function" then
        return false
    end

    slot = slot or 1
    local numSku = tonumber(sku)
    if not numSku and type(sku) == "string" and Crabe.VirtualReader and Crabe.VirtualReader.skuForName then
        numSku = tonumber(Crabe.VirtualReader.skuForName(sku))
    end
    if not numSku then
        numSku = 1000200 -- Default Anakin Skywalker
    end

    return Crabe._portalEmulatePlace(slot, numSku)
end

-- Removes a figure from a slot.
function Crabe.Portal.removeVirtualFigure(slot)
    if type(Crabe._portalEmulateRemove) ~= "function" then
        return false
    end
    return Crabe._portalEmulateRemove(slot or 1)
end

-- Polls tag arrival / departure events and dispatches to the game engine.
function Crabe.Portal.update()
    if type(Crabe._portalPollEvent) ~= "function" then
        return
    end

    while true do
        local eventType, slot, sku = Crabe._portalPollEvent()
        if not eventType or eventType == 0 then
            break
        end

        if eventType == 1 then -- Placed
            -- Auto-spawn character if placed on Player 1 slot
            if slot == 1 and sku and sku > 0 then
                if Game and type(Game.SetCharacter) == "function" then
                    pcall(function()
                        Game.SetCharacter(sku, "loadout", 1)
                    end)
                end
                -- Light up in Cyan
                Crabe.Portal.setLed(1, 0, 255, 255)
            elseif slot == 2 and sku and sku > 0 then
                if Game and type(Game.SetCharacter) == "function" then
                    pcall(function()
                        Game.SetCharacter(sku, "loadout", 2)
                    end)
                end
                -- Light up Player 2 in Magenta
                Crabe.Portal.setLed(2, 255, 0, 255)
            end
        elseif eventType == 2 then -- Removed
            -- Turn off LED on that slot
            Crabe.Portal.setLed(slot, 0, 0, 0)
        end
    end
end

-- Register automatic tick update
if Game and type(Game.onTick) == "function" then
    Game.onTick(function()
        Crabe.Portal.update()
    end)
elseif Crabe and Crabe.Events and type(Crabe.Events.on) == "function" then
    Crabe.Events.on("tick", function()
        Crabe.Portal.update()
    end)
end
