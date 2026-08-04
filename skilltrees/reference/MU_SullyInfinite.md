# MU_SullyInfinite

`ProgressionTree = "IN1_MU_Sullivan"` -- source: `gamedb/core/in1_mu_sullivan.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| SULLEY_BASEHEALTH | health | 150 |
| SULLEY_BASEMELEE_DMG | meleeDamage | 1 |
| SULLEY_BASERUNSPEED | runspeed | 7 |
| SULLEY_BASESUPERJUMP | BM_SuperJumper | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| SULLEY_RANGED_DMG3 | PRG_SUL_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.2,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SUL_RangedDmg3_desc |
| SULLEY_SUPERJUMP2 | PRG_SUL_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,44,10,12 | 5 | 0 | 0 | 1 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade2 | PRG_SUL_SuperJump2_desc |
| SULLEY_RANGED_DMG2 | PRG_SUL_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.3 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SUL_RangedDmg2_desc |
| SULLEY_HEALTH5 | PRG_SUL_Health5 | health | 50 | 6 | 0 | 0 | 1 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SUL_Health5_desc |
| SULLEY_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| SULLEY_HEALTH2 | PRG_SUL_Health2 | health | 25 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SUL_Health2_desc |
| SULLEY_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| SULLEY_RANGED_DMG5 | PRG_SUL_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.3,0.5 | 6 | 1 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SUL_RangedDmg5_desc |
| SULLEY_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| SULLEY_HEALTH1 | PRG_SUL_Health1 | health | 25 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SUL_Health1_desc |
| SULLEY_MELEE_DMG2 | PRG_SUL_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SUL_MeleeDmg2_desc |
| SULLEY_MELEE_DMG5 | PRG_SUL_MeleeDmg5 | meleeDamage | 0.25 | 6 | 1 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SUL_MeleeDmg5_desc |
| SULLEY_RUNSPEED1 | PRG_SUL_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_SUL_RunSpeed1_desc |
| SULLEY_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Sully |  |
| SULLEY_MELEE_DMG3 | PRG_SUL_MeleeDmg3 | meleeDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SUL_MeleeDmg3_desc |
| SULLEY_RANGED_DMG4 | PRG_SUL_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.25,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SUL_RangedDmg4_desc |
| SULLEY_MELEE_DMG4 | PRG_SUL_MeleeDmg4 | meleeDamage | 0.2 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SUL_MeleeDmg4_desc |
| SULLEY_RANGED_DMG1 | PRG_SUL_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SUL_RangedDmg1_desc |
| SULLEY_RUNSPEED2 | PRG_SUL_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_SUL_RunSpeed2_desc |
| SULLEY_GROUNDPOUND2 | PRG_SUL_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 3 | 1 | 0 | 1 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_SUL_GroundPound2_desc |
| SULLEY_MELEE_DMG1 | PRG_SUL_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SUL_MeleeDmg1_desc |
| SULLEY_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| SULLEY_SUPERJUMP1 | PRG_SUL_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 3 | 1 | 0 | 0 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_SUL_SuperJump1_desc |
| SULLEY_HEALTH4 | PRG_SUL_Health4 | health | 25 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SUL_Health4_desc |
| SULLEY_HEALTH3 | PRG_SUL_Health3 | health | 25 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SUL_Health3_desc |
