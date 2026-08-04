# WR_Ralph

`ProgressionTree = "IN1_WR_Ralph"` -- source: `gamedb/core/in1_wr_ralph.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| RALPH_BASERUNSPEED | runspeed | 7 |
| RALPH_BASESTRENGTH | BM_SuperDuperStrong | 0 |
| RALPH_BASESUPERJUMP | BM_SuperJumper | 0 |
| RALPH_BASEHEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| RALPH_RANGED_DMG5 | PRG_RPH_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.3,0.5 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RPH_RangedDmg5_desc |
| RALPH_RANGED_DMG2 | PRG_RPH_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RPH_RangedDmg2_desc |
| RALPH_MELEE_DMG3 | PRG_RPH_MeleeDmg3 | meleeDamage | 0.15 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RPH_MeleeDmg3_desc |
| RALPH_RANGED_DMG4 | PRG_RPH_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.25,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RPH_RangedDmg4_desc |
| RALPH_HEALTH3 | PRG_RPH_Health3 | health | 25 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RPH_Health3_desc |
| RALPH_STRENGTH | PRG_RPH_Strength | BM_SuperDuperStrong | 1 | 3 | 1 | 0 | 1 |  | PRG_StrengthUpgrade_Screen | AV_PRG_PickupThrowUpgrade1 | PRG_RPH_Strength_desc |
| RALPH_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| RALPH_MELEE_DMG5 | PRG_RPH_MeleeDmg5 | meleeDamage | 0.25 | 6 | 0 | 0 | 1 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RPH_MeleeDmg5_desc |
| RALPH_SUPERJUMP2 | PRG_RPH_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,44,10,12 | 4 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_RPH_SuperJump2_desc |
| RALPH_HEALTH1 | PRG_RPH_Health1 | health | 25 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RPH_Health1_desc |
| RALPH_RUNSPEED2 | PRG_RPH_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_RPH_RunSpeed2_desc |
| RALPH_RUNSPEED1 | PRG_RPH_RunSpeed1 | runspeed | 8 | 1 | 1 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_RPH_RunSpeed1_desc |
| RALPH_HEALTH5 | PRG_RPH_Health5 | health | 50 | 6 | 0 | 0 | 1 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RPH_Health5_desc |
| RALPH_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| RALPH_GROUNDPOUND2 | PRG_RPH_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 2 | 1 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_RPH_GroundPound2_desc |
| RALPH_SUPERJUMP3 | PRG_RPH_SuperJump3 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,64,11.3,10 | 6 | 0 | 0 | 1 |  | PRG_SuperJumpUpgrade2_Screen | AV_PRG_SuperJumpUpgrade2 | PRG_RPH_SuperJump3_desc |
| RALPH_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Ralph |  |
| RALPH_MELEE_DMG1 | PRG_RPH_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RPH_MeleeDmg1_desc |
| RALPH_SUPERJUMP1 | PRG_RPH_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 2 | 1 | 0 | 0 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_RPH_SuperJump1_desc |
| RALPH_RANGED_DMG3 | PRG_RPH_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.2,0.3 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RPH_RangedDmg3_desc |
| RALPH_RANGED_DMG1 | PRG_RPH_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RPH_RangedDmg1_desc |
| RALPH_MELEE_DMG2 | PRG_RPH_MeleeDmg2 | meleeDamage | 0.1 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RPH_MeleeDmg2_desc |
| RALPH_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| RALPH_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| RALPH_HEALTH2 | PRG_RPH_Health2 | health | 25 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RPH_Health2_desc |
| RALPH_MELEE_DMG4 | PRG_RPH_MeleeDmg4 | meleeDamage | 0.2 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RPH_MeleeDmg4_desc |
| RALPH_HEALTH4 | PRG_RPH_Health4 | health | 25 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RPH_Health4_desc |
