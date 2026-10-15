# Avatar Control, Stats & Area Damage

A developer guide to manipulating player avatars, managing health and invulnerability, tracking coordinates, teleporting, leveling up, and dealing area-of-effect damage using **CrabeLoader's Avatar and Combat APIs** (`Game.Avatar*` and `Game.DamageRadius`).

---

## 1. Overview & Architecture

In Disney Infinity 3.0:
* Every active player avatar is backed by an internal **kinematic actor handle** (an unsigned integer).
* Native script functions like `Agent_*` and `Actor_*` operate on this handle.
* Player positions, velocities, and health cannot be modified with raw memory writes because the Octane physics engine (`KinematicState`) immediately overwrites them.

CrabeLoader hooks the engine's internal kinematic setters to provide clean, robust avatar manipulation without physics desynchronization.

---

## 2. Resolving the Avatar Handle & Position

To interact with the player avatar, first obtain its active actor handle:

```lua
-- Get the actor handle for Player 1 (Host)
local p1Handle = Game.GetAvatarHandle()

-- Get the actor handle for Player 2
local p2Handle = Game.GetAvatarHandle(1)
```

### 2.1. Reading World Coordinates
```lua
local x, y, z = Game.ActorPosition(p1Handle)
if x then
    print(string.format("Player is at X=%.2f, Y=%.2f (height), Z=%.2f", x, y, z))
end
```

### 2.2. Teleporting the Avatar
Use `Game.PlaceActor` to warp the player to exact coordinates while preserving facing direction and physics state:

```lua
-- Teleport to coordinates (x, y, z)
Game.PlaceActor(p1Handle, 150.0, 20.0, -85.5)
```

---

## 3. Health, God Mode & Respawning

CrabeLoader provides complete control over avatar mortality (`src/api/11b_avatar_state.lua`):

### 3.1. Invulnerability (God Mode)
```lua
-- Make Player 1 invulnerable
Game.SetInvulnerable(true)

-- Turn off invulnerability
Game.SetInvulnerable(false)
```

### 3.2. Defeat & Respawn
```lua
-- Instantly defeats the avatar (triggers defeat animation & sound)
Game.KillAvatar()

-- Forces an immediate respawn at the nearest checkpoint
Game.RespawnPlayer()
```

---

## 4. Sparks, Level Progression & Economy

Disney Infinity uses **Sparks** as currency (Orange Sparks for XP, Blue Sparks for Toy Box toys, Green Sparks for health):

```lua
-- Query current spark balance
local sparks = Game.GetSparks()
print("Sparks:", sparks)

-- Add 5000 sparks
Game.AddSparks(5000)

-- Set avatar level directly (Level 1 to 20)
Game.SetLevel(20)

-- Query current level
local currentLvl = Game.GetLevel()
print("Current Level:", currentLvl)
```

---

## 5. Area-of-Effect Combat: `Game.DamageRadius`

CrabeLoader exposes the engine's native radial damage algorithm via `Game.DamageRadius` (`src/api/31_combat.lua`).

```lua
Game.DamageRadius(x, y, z, radius, damage, damageType, exceptHandle)
```

### 5.1. Parameters:
* **`x, y, z`** (*numbers*): Center coordinates of the explosion / attack.
* **`radius`** (*number*): Blast radius in game meters.
* **`damage`** (*number*): Amount of raw damage to inflict.
* **`damageType`** (*string*, optional):
  * `"damageExplosive"` (Default): High knockback and physics impulse.
  * `"damageNormal"`: Standard physical strike damage.
  * `"damageSpecial"`: Bypasses standard shields.
* **`exceptHandle`** (*number*, optional): Actor handle to spare (e.g. the player executing the move).

### 5.2. Example: Ground Slam / Force Push Attack
```lua
-- Trigger an explosive shockwave around the player without hurting the player
local handle = Game.GetAvatarHandle()
local px, py, pz = Game.ActorPosition(handle)

if px then
    -- Blast enemies within 15 meters for 500 damage, sparing the player
    Game.DamageRadius(px, py, pz, 15.0, 500, "damageExplosive", handle)
    print("Force Push shockwave executed!")
end
```
Victims calculate their own damage reactions, armor absorption, and debris physics automatically through the engine's combat rules.
