-- CrabeLoader
-- File description:
-- The engine's own figure registry (Gateway), still complete in the PC build: primitives only.
-- A tag added here is a figure the game sees as if a reader had reported it, under its own UID.
-- Reference: docs/guides/portal.md.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Gateway = Crabe.Gateway or {}

-- True when this build carries the bridge to the engine's registry.
-- Without it, every other Crabe.Gateway call fails and returns false or nil.
function Crabe.Gateway.isAvailable()
    return type(Crabe._gatewayVirtualAvatar) == "function" and Crabe._gatewayVirtualAvatar() ~= nil
end

-- UID of the virtual reader's tag for a character id.
-- The virtual reader uses the character id itself as the UID of the tags it adds.
function Crabe.Gateway.uidForId(id)
    return string.format("%014x", tonumber(id) or 0)
end

-- Character id of the virtual reader's current avatar tag.
-- Nil when the virtual reader holds no avatar or the bridge is absent.
function Crabe.Gateway.virtualAvatar()
    if type(Crabe._gatewayVirtualAvatar) ~= "function" then
        return nil
    end
    local id = Crabe._gatewayVirtualAvatar()
    return (id and id > 0) and id or nil
end

-- Adds a tag; antenna 0 is Player 1 and 1 is Player 2, and the game reacts before this returns.
-- options: experience, level, lastPlayed, actionFlags, owner, skillBits ({ n, ... }) and watch.
-- With watch, every write the game makes to this tag is emitted as "gatewayTagWrite".
function Crabe.Gateway.addTag(uid, characterId, antenna, options)
    if type(Crabe._gatewayAddTag) ~= "function" or type(uid) ~= "string" then
        return false
    end
    options = options or {}
    local bits = options.skillBits and Crabe.Portal.encodeSkillBits(options.skillBits) or nil
    return Crabe._gatewayAddTag(uid, characterId or 0, antenna or 0, options.watch == true,
        options.experience or 0, options.level or 0, options.lastPlayed or 0, options.actionFlags or 0,
        options.owner or 0, bits)
end

-- Removes a tag by UID, whether it was added here or by the virtual reader.
-- The game reacts before this returns.
function Crabe.Gateway.removeTag(uid)
    if type(Crabe._gatewayRemoveTag) ~= "function" or type(uid) ~= "string" then
        return false
    end
    return Crabe._gatewayRemoveTag(uid)
end

-- Drains the game's writes to watched tags and emits each as "gatewayTagWrite" (uid, field, value).
-- A progression value is decoded to its set skill bit numbers; every other value is a number.
-- Runs on every game tick.
function Crabe.Gateway.update()
    if type(Crabe._gatewayPollWrite) ~= "function" or not (Crabe.Events and Crabe.Events.emit) then
        return
    end
    while true do
        local uid, field, value = Crabe._gatewayPollWrite()
        if not uid then
            break
        end
        if field == "progression" then
            value = Crabe.Portal.decodeSkillBits(value or "")
        end
        Crabe.Events.emit("gatewayTagWrite", uid, field, value)
    end
end

if Game and type(Game.onTick) == "function" then
    Game.onTick(Crabe.Gateway.update)
elseif Crabe.Events and type(Crabe.Events.on) == "function" then
    Crabe.Events.on("tick", Crabe.Gateway.update)
end
