# MU_Randall

`ProgressionTree = "IN1_MU_Randall"` -- source: `gamedb/core/in1_mu_randall.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| RANDY_BASERUNSPEED | runspeed | 7 |
| RANDY_BASEHEALTH | health | 150 |
| RANDY_BASESUPERJUMP | BM_SuperJumper | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| RANDY_MELEE_DMG4 | PRG_RND_MeleeDmg4 | meleeDamage | 0.2 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RND_MeleeDmg4_desc |
| RANDY_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| RANDY_RANGED_DMG3 | PRG_RND_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.2,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RND_RangedDmg3_desc |
| RANDY_MELEE_DMG2 | PRG_RND_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RND_MeleeDmg2_desc |
| RANDY_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| RANDY_RANGED_DMG1 | PRG_RND_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RND_RangedDmg1_desc |
| RANDY_HEALTH1 | PRG_RND_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RND_Health1_desc |
| RANDY_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| RANDY_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Randall |  |
| RANDY_RUNSPEED2 | PRG_RND_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_RND_RunSpeed2_desc |
| RANDY_RUNSPEED1 | PRG_RND_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_RND_RunSpeed1_desc |
| RANDY_RANGED_DMG4 | PRG_RND_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.25,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RND_RangedDmg4_desc |
| RANDY_HEIGHTENED_SENSE | PRG_RND_HeightenedSense | heightenedSense,BM_HeightenedSenses | 1.0,1.0 | 6 | 1 | 0 | 1 |  | PRG_HeightenedSense_Screen | AV_PRG_SpideySenseUnlock | PRG_RND_HeightenedSense_desc |
| RANDY_SUPERJUMP1 | PRG_RND_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 4 | 1 | 0 | 1 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_RND_SuperJump1_desc |
| RANDY_RANGED_DMG2 | PRG_RND_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.3 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RND_RangedDmg2_desc |
| RANDY_RANGED_DMG5 | PRG_RND_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.3,0.5 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RND_RangedDmg5_desc |
| RANDY_HEALTH2 | PRG_RND_Health2 | health | 40 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RND_Health2_desc |
| RANDY_MELEE_DMG5 | PRG_RND_MeleeDmg5 | meleeDamage | 0.25 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RND_MeleeDmg5_desc |
| RANDY_GROUNDPOUND2 | PRG_RND_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 3 | 1 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_RND_GroundPound2_desc |
| RANDY_MELEE_DMG3 | PRG_RND_MeleeDmg3 | meleeDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RND_MeleeDmg3_desc |
| RANDY_MELEE_DMG1 | PRG_RND_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RND_MeleeDmg1_desc |
| RANDY_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| RANDY_HEALTH3 | PRG_RND_Health3 | health | 70 | 6 | 1 | 0 | 1 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RND_Health3_desc |
