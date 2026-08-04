# MU_Mike

`ProgressionTree = "IN1_MU_Mike"` -- source: `gamedb/core/in1_mu_mike.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| MIKE_BASESUPERJUMP | BM_SuperJumper | 0 |
| MIKE_BASERUNSPEED | runspeed | 7 |
| MIKE_BASEHEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| MIKE_MELEE_DMG4 | PRG_MIK_MeleeDmg4 | meleeDamage | 0.2 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MIK_MeleeDmg4_desc |
| MIKE_GROUNDPOUND2 | PRG_MIK_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 3 | 1 | 0 | 1 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_MIK_GroundPound2_desc |
| MIKE_RANGED_DMG3 | PRG_MIK_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.2,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MIK_RangedDmg3_desc |
| MIKE_HEALTH3 | PRG_MIK_Health3 | health | 70 | 6 | 1 | 0 | 1 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MIK_Health3_desc |
| MIKE_SUPERJUMP1 | PRG_MIK_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 4 | 1 | 0 | 1 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_MIK_SuperJump1_desc |
| MIKE_MELEE_DMG5 | PRG_MIK_MeleeDmg5 | meleeDamage | 0.25 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MIK_MeleeDmg5_desc |
| MIKE_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| MIKE_RANGED_DMG1 | PRG_MIK_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MIK_RangedDmg1_desc |
| MIKE_MELEE_DMG1 | PRG_MIK_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MIK_MeleeDmg1_desc |
| MIKE_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| MIKE_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Mike |  |
| MIKE_MELEE_DMG2 | PRG_MIK_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MIK_MeleeDmg2_desc |
| MIKE_RUNSPEED2 | PRG_MIK_RunSpeed2 | runspeed | 8.5 | 3 | 1 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_MIK_RunSpeed2_desc |
| MIKE_RUNSPEED1 | PRG_MIK_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_MIK_RunSpeed1_desc |
| MIKE_RANGED_DMG5 | PRG_MIK_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.3,0.5 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MIK_RangedDmg5_desc |
| MIKE_RANGED_DMG4 | PRG_MIK_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.25,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MIK_RangedDmg4_desc |
| MIKE_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| MIKE_MELEE_DMG3 | PRG_MIK_MeleeDmg3 | meleeDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MIK_MeleeDmg3_desc |
| MIKE_RANGED_DMG2 | PRG_MIK_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.3 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MIK_RangedDmg2_desc |
| MIKE_HEALTH2 | PRG_MIK_Health2 | health | 40 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MIK_Health2_desc |
| MIKE_HEALTH1 | PRG_MIK_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MIK_Health1_desc |
| MIKE_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
