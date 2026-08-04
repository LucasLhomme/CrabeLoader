# AV_Jessie

`ProgressionTree = "IN1_TS_Jessie"` -- source: `gamedb/core/in1_ts_jessie.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| JESSIE_BASEHEALTH | health | 150 |
| JESSIE_BASERUNSPEED | runspeed | 7 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| JESSIE_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| JESSIE_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| JESSIE_HEALTH1 | PRG_JES_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_JES_Health1_desc |
| JESSIE_RANGED_DMG1 | PRG_JES_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JES_RangedDmg1_desc |
| JESSIE_MELEE_DMG1 | PRG_JES_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JES_MeleeDmg1_desc |
| JESSIE_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| JESSIE_RANGED_DMG4 | PRG_JES_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.25,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JES_RangedDmg4_desc |
| JESSIE_MELEE_DMG2 | PRG_JES_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JES_MeleeDmg2_desc |
| JESSIE_GROUNDPOUND2 | PRG_JES_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 3 | 1 | 0 | 1 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_JES_GroundPound2_desc |
| JESSIE_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| JESSIE_MELEE_DMG3 | PRG_JES_MeleeDmg3 | meleeDamage | 0.15 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JES_MeleeDmg3_desc |
| JESSIE_MELEE_DMG5 | PRG_JES_MeleeDmg5 | meleeDamage | 0.25 | 6 | 1 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JES_MeleeDmg5_desc |
| JESSIE_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Jessie |  |
| JESSIE_RUNSPEED2 | PRG_JES_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 1 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_JES_RunSpeed2_desc |
| JESSIE_MELEE_DMG4 | PRG_JES_MeleeDmg4 | meleeDamage | 0.2 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JES_MeleeDmg4_desc |
| JESSIE_RANGED_DMG2 | PRG_JES_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.3 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JES_RangedDmg2_desc |
| JESSIE_RANGED_DMG3 | PRG_JES_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.2,0.3 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JES_RangedDmg3_desc |
| JESSIE_RUNSPEED1 | PRG_JES_RunSpeed1 | runspeed | 8 | 1 | 1 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_JES_RunSpeed1_desc |
| JESSIE_RANGED_DMG5 | PRG_JES_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.3,0.5 | 6 | 1 | 0 | 1 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JES_RangedDmg5_desc |
| JESSIE_HEALTH3 | PRG_JES_Health3 | health | 70 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_JES_Health3_desc |
| JESSIE_HEALTH2 | PRG_JES_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_JES_Health2_desc |
