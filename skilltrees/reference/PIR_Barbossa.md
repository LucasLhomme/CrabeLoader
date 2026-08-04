# PIR_Barbossa

`ProgressionTree = "IN1_PIR_Barbossa"` -- source: `gamedb/core/in1_pir_barbossa.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| BARBOSSA_BASEHEALTH | health | 150 |
| BARBOSSA_BASECLIP | sharedAmmoMultiplier | 4 |
| BARBOSSA_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.4 |
| BARBOSSA_BASERUNSPEED | runspeed | 7 |
| BARBOSSA_BASEFIRERATE | sharedFireRateMultiplier | 1 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| BARBOSSA_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| BARBOSSA_RANGED_DMG3 | PRG_BRB_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.05,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BRB_RangedDmg3_desc |
| BARBOSSA_GROUNDPOUND2 | PRG_BRB_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 2 | 1 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_BRB_GroundPound2_desc |
| BARBOSSA_MELEE_DMG3 | PRG_BRB_MeleeDmg3 | meleeDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BRB_MeleeDmg3_desc |
| BARBOSSA_MELEE_DMG2 | PRG_BRB_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BRB_MeleeDmg2_desc |
| BARBOSSA_FIRERATE2 | PRG_BRB_FireRate2 | sharedFireRateMultiplier | 2 | 5 | 0 | 0 | 1 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_BRB_FireRate2_desc |
| BARBOSSA_FIRERATE1 | PRG_BRB_FireRate1 | sharedFireRateMultiplier | 1.5 | 3 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_BRB_FireRate1_desc |
| BARBOSSA_MELEE_DMG5 | PRG_BRB_MeleeDmg5 | meleeDamage | 0.25 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BRB_MeleeDmg5_desc |
| BARBOSSA_HEALTH3 | PRG_BRB_Health3 | health | 25 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BRB_Health3_desc |
| BARBOSSA_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| BARBOSSA_RANGED_DMG5 | PRG_BRB_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.15,0.5 | 6 | 1 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BRB_RangedDmg5_desc |
| BARBOSSA_CLIP1 | PRG_BRB_ClipSize1 | sharedAmmoMultiplier | 6 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_BRB_ClipSize1_desc |
| BARBOSSA_RANGED_DMG2 | PRG_BRB_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.05,0.3 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BRB_RangedDmg2_desc |
| BARBOSSA_HEALTH2 | PRG_BRB_Health2 | health | 25 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BRB_Health2_desc |
| BARBOSSA_RANGED_DMG1 | PRG_BRB_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.05,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BRB_RangedDmg1_desc |
| BARBOSSA_RUNSPEED1 | PRG_BRB_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_BRB_RunSpeed1_desc |
| BARBOSSA_CLIP2 | PRG_BRB_ClipSize2 | sharedAmmoMultiplier | 8 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_BRB_ClipSize2_desc |
| BARBOSSA_MELEE_DMG4 | PRG_BRB_MeleeDmg4 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BRB_MeleeDmg4_desc |
| BARBOSSA_HEALTH1 | PRG_BRB_Health1 | health | 25 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BRB_Health1_desc |
| BARBOSSA_RANGED_DMG4 | PRG_BRB_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.1,0.4 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BRB_RangedDmg4_desc |
| BARBOSSA_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| BARBOSSA_MELEE_DMG1 | PRG_BRB_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BRB_MeleeDmg1_desc |
| BARBOSSA_RUNSPEED2 | PRG_BRB_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 1 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_BRB_RunSpeed2_desc |
| BARBOSSA_RICOCHET_BLOCK | PRG_BRB_RicochetBlock | blockRicochet | 1 | 5 | 1 | 0 | 1 |  | PRG_RicochetBlock_Screen | AV_PRG_RicochetBlock | PRG_BRB_RicochetBlock_desc |
| BARBOSSA_HEALTH4 | PRG_BRB_Health4 | health | 25 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BRB_Health4_desc |
| BARBOSSA_HEALTH5 | PRG_BRB_Health5 | health | 50 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BRB_Health5_desc |
| BARBOSSA_BLOCKBREAKER_COMBO | PRG_BRB_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 1 |  | PRG_BlockBreaker_Screen | AV_PRG_BlockBreaker | PRG_BRB_BlockBreakerCombo_desc |
| BARBOSSA_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| BARBOSSA_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Barbossa |  |
