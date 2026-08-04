# AV_Dash

`ProgressionTree = "IN1_Inc_Dash"` -- source: `gamedb/core/in1_inc_dash.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| DASH_BASEHEALTH | health | 150 |
| DASH_BASERUNSPEED | runspeed | 7 |
| DASH_BASERANGEDDAMAGE | rangedDamage,sharedRangedDamage | 1,1 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| DASH_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| DASH_BLOCKBREAKER_COMBO | PRG_DSH_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 1 |  | PRG_BlockBreaker_Screen | AV_PRG_BlockBreaker | PRG_DSH_BlockBreakerCombo_desc |
| DASH_MELEE_DMG2 | PRG_DSH_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DSH_MeleeDmg2_desc |
| DASH_RANGED_DMG5 | PRG_DSH_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.3,0.50 | 6 | 0 | 0 | 1 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DSH_RangedDmg5_desc |
| DASH_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Dash |  |
| DASH_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| DASH_MELEE_DMG4 | PRG_DSH_MeleeDmg4 | meleeDamage | 0.2 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DSH_MeleeDmg4_desc |
| DASH_MELEE_DMG1 | PRG_DSH_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DSH_MeleeDmg1_desc |
| DASH_RUNSPEED2 | PRG_DSH_RunSpeed2 | runspeed | 9 | 3 | 1 | 0 | 1 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_DSH_RunSpeed2_desc |
| DASH_RUNSPEED1 | PRG_DSH_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_DSH_RunSpeed1_desc |
| DASH_RANGED_DMG4 | PRG_DSH_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.25,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DSH_RangedDmg4_desc |
| DASH_RANGED_DMG1 | PRG_DSH_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DSH_RangedDmg1_desc |
| DASH_RANGED_DMG2 | PRG_DSH_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.3 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DSH_RangedDmg2_desc |
| DASH_HEALTH2 | PRG_DSH_Health2 | health | 40 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_DSH_Health2_desc |
| DASH_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| DASH_RANGED_DMG3 | PRG_DSH_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.2,0.3 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DSH_RangedDmg3_desc |
| DASH_SUPER_STRAIGHT_PUNCH | PRG_DSH_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 0 |  | PRG_SuperPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_DSH_SuperStraightPunch_desc |
| DASH_HEALTH3 | PRG_DSH_Health3 | health | 70 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_DSH_Health3_desc |
| DASH_GROUNDPOUND2 | PRG_DSH_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 3 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_DSH_GroundPound2_desc |
| DASH_HEALTH1 | PRG_DSH_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_DSH_Health1_desc |
| DASH_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| DASH_MELEE_DMG3 | PRG_DSH_MeleeDmg3 | meleeDamage | 0.15 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DSH_MeleeDmg3_desc |
| DASH_HEIGHTENED_SENSE | PRG_DSH_HeightenedSense | heightenedSense,BM_HeightenedSenses | 1.0,1.0 | 6 | 1 | 0 | 1 |  | PRG_HeightenedSense_Screen | AV_PRG_SpideySenseUnlock | PRG_DSH_HeightenedSense_desc |
| DASH_MELEE_DMG5 | PRG_DSH_MeleeDmg5 | meleeDamage | 0.25 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DSH_MeleeDmg5_desc |
