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

### 2.3. Running Faster
`Game.SetActorSpeedScale` scales how fast one actor moves, without touching the game clock, so enemies keep their pace. `1.0` is normal and `20.0` the ceiling; the engine rebuilds the value when a world loads, so set it again afterwards.

```lua
Game.SetActorSpeedScale(p1Handle, 3.0)
print(Game.ActorSpeedScale(p1Handle))
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

## 4. Sparks, Level Progression & Skill Points

### 4.1. Sparks
Sparks are the game's currency. Adding and removing go through the inventory; the game ignores a removal larger than the balance instead of clamping it to zero, so cap the amount yourself:

```lua
local balance = Game.GetSparks()
Game.AddToInventory("Items.money", 5000)
Game.RemoveFromInventory("Items.money", math.min(1000, balance))
```

### 4.2. Level
```lua
print(Game.GetAvatarLevel())    -- level of the current hero, 0 to 20
Game.LevelUpAvatar()            -- one level up
Game.SetAvatarLevel(12)         -- absolute level, 0 to 20: down as well as up
```

`Game.SetAvatarLevel` calls the engine's own level-up code, so the hero's data is saved the way a normal level-up saves it. Experience is not touched: a hero set below the level its experience pays for levels back up at the next experience gained. It returns the level the game reports afterwards.

### 4.3. Skill points
Skill points are derived, never stored: points earned by the level (a table in the game's `avatarlevels.lua`, 75 in total at level 20) minus the cost of the skills bought. There is no setter, so the ways to have more are a higher level and a refund:

```lua
print(Game.GetSkillPoints())    -- unspent points
Game.RespecAvatar()             -- refunds every bought skill, free; the game allows two per figure
```

Lowering a level below what the bought skills cost leaves the count negative; call `Game.RespecAvatar()` afterwards.

To change the 75-point total itself, patch the table when the game loads it. The patch has to be a file read at startup (see [skilltrees.md](skilltrees.md)): `mods/<your mod>/skilltrees/SkillPointsEarned.patch` containing

```lua
local points = Data.SkillPointsEarned
points[#points] = points[#points] + 25
```

gives a level 20 hero 100 points. It applies to every hero and takes effect the next time the game starts: a chunk patch registered later from Lua, with `Crabe.Hooks.patchChunk`, is too late because the table is read once at startup.

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

---

## 6. Loading Worlds Without Losing a Play Set Save

`Game.LoadLevel(zone)` behaves differently depending on where it is called from. Inside a loaded world the game saves the current world and then switches zone, so Play Set progress is kept. From the front end (or from a world of another Play Set) the engine starts the zone as a master zone without loading any save, which looks like a reset: story cutscenes replay and unlocked levels are locked again.

To enter a Play Set zone with its save, resume the save first, then move to the zone:

```lua
if Game.CanContinuePlayset("InsideOut") then
    Game.ContinuePlayset("InsideOut")
    -- once the hero is standing in the Play Set (Game.GetAvatarHandle() answers, 3 to 4 seconds after load):
    Game.LoadLevel("OUT_L2_S1")
end
```

`Game.ContinuePlayset(playset, zone)` skips the character screen and resumes the save where it was saved: the engine picks that zone itself and ignores the one given. The Play Set of a zone is `Game.ZoneString("PlaysetProperties", zone)`. CrabeMenu's world travel follows exactly this sequence.
