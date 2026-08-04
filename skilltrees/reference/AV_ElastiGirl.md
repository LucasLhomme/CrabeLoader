# AV_ElastiGirl

`ProgressionTree = "IN1_Inc_Helen"` -- source: `gamedb/core/in1_inc_helen.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| HELEN_BASEHEALTH | health | 150 |
| HELEN_BASESUPERJUMP | BM_SuperJumper | 0 |
| HELEN_BASERUNSPEED | runspeed | 7 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| HELEN_SUPER_STRAIGHT_PUNCH | PRG_HLN_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 0 |  | PRG_SuperPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_HLN_SuperStraightPunch_desc |
| HELEN_RANGED_DMG1 | PRG_HLN_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.05,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HLN_RangedDmg1_desc |
| HELEN_HEALTH2 | PRG_HLN_Health2 | health | 30 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_HLN_Health2_desc |
| HELEN_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Helen |  |
| HELEN_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| HELEN_RUNSPEED2 | PRG_HLN_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_HLN_RunSpeed2_desc |
| HELEN_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| HELEN_HEIGHTENED_SENSE | PRG_HLN_HeightenedSense | heightenedSense,BM_HeightenedSenses | 1.0,1.0 | 5 | 1 | 0 | 0 |  | PRG_HeightenedSense_Screen | AV_PRG_SpideySenseUnlock | PRG_HLN_HeightenedSense_desc |
| HELEN_SUPERJUMP1 | PRG_HLN_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 2 | 0 | 0 | 1 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_HLN_SuperJump1_desc |
| HELEN_RANGED_DMG5 | PRG_HLN_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.15,0.50 | 6 | 1 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HLN_RangedDmg5_desc |
| HELEN_MELEE_DMG3 | PRG_HLN_MeleeDmg3 | meleeDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HLN_MeleeDmg3_desc |
| HELEN_MELEE_DMG1 | PRG_HLN_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HLN_MeleeDmg1_desc |
| HELEN_MELEE_DMG2 | PRG_HLN_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HLN_MeleeDmg2_desc |
| HELEN_GROUNDPOUND2 | PRG_HLN_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 2 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_HLN_GroundPound2_desc |
| HELEN_RANGED_DMG2 | PRG_HLN_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.075,0.3 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HLN_RangedDmg2_desc |
| HELEN_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| HELEN_MELEE_DMG5 | PRG_HLN_MeleeDmg5 | meleeDamage | 0.25 | 6 | 0 | 0 | 1 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HLN_MeleeDmg5_desc |
| HELEN_HEALTH3 | PRG_HLN_Health3 | health | 30 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_HLN_Health3_desc |
| HELEN_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| HELEN_BLOCKBREAKER_COMBO | PRG_HLN_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 1 |  | PRG_BlockBreaker_Screen | AV_PRG_BlockBreaker | PRG_HLN_BlockBreakerCombo_desc |
| HELEN_MELEE_DMG4 | PRG_HLN_MeleeDmg4 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HLN_MeleeDmg4_desc |
| HELEN_RANGED_DMG4 | PRG_HLN_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.125,0.4 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HLN_RangedDmg4_desc |
| HELEN_HEALTH1 | PRG_HLN_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_HLN_Health1_desc |
| HELEN_RUNSPEED1 | PRG_HLN_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_HLN_RunSpeed1_desc |
| HELEN_RANGED_DMG3 | PRG_HLN_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.1,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HLN_RangedDmg3_desc |
| HELEN_HEALTH4 | PRG_HLN_Health4 | health | 60 | 6 | 0 | 0 | 1 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_HLN_Health4_desc |
