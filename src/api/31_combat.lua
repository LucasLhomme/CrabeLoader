-- CrabeLoader
-- File description:
-- Area damage: hurting every actor around a point with the engine's own damage code.
-- Needs a loader that carries Crabe._damageRadius; the engine decides who is hurt, and by how much.
-- Reads no actor list and spawns nothing -- that is src/api/14_world.lua and src/api/13_spawn.lua.
--
-- Authors: @LucasLhomme

Game = Game or {}

-- Deals `damage` of an engine damage type to every actor within `radius` of x, y, z (y being
-- height), the way the engine's DamageRadiusExceptActor script native does, with no source and
-- no owner. damageType is "damageExplosive" (the default), "damageNormal", "damageSpecial" or a
-- four-character damage code. The actor behind `exceptHandle` (Game.GetAvatarHandle for a
-- player) is spared. Victims keep their own health, armour and team rules: a big enough number
-- kills anything that can be killed. True when the engine ran the call.
function Game.DamageRadius(x, y, z, radius, damage, damageType, exceptHandle)
    if type(Crabe._damageRadius) ~= "function" then
        error("Game.DamageRadius: this loader has no area damage native", 2)
    end
    if type(x) ~= "number" or type(y) ~= "number" or type(z) ~= "number" then
        error("Game.DamageRadius: expected x, y, z as numbers", 2)
    end
    if type(radius) ~= "number" or radius <= 0 then
        error("Game.DamageRadius: radius must be a positive number", 2)
    end
    if type(damage) ~= "number" or damage <= 0 then
        error("Game.DamageRadius: damage must be a positive number", 2)
    end
    return Crabe._damageRadius(x, y, z, radius, damage, damageType or "damageExplosive", exceptHandle or 0) == true
end
