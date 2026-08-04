# AV_MrIncredibleInfinite

`ProgressionTree = "IN1_Inc_Bob"` -- source: `gamedb/core/in1_inc_bob.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| BOB_BASEMELEE_DMG | meleeDamage | 1 |
| BOB_BASESUPERJUMP | BM_SuperJumper | 0 |
| BOB_BASEHEALTH | health | 150 |
| BOB_BASERANGED_DMG | rangedDamage,sharedRangedDamage | 1.0,1.0 |
| BOB_BASERUNSPEED | runspeed | 7 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| BOB_SUPER_STRAIGHT_PUNCH | PRG_BOB_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 1 |  | PRG_SuperPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_BOB_SuperStraightPunch_desc |
| BOB_RANGED_DMG1 | PRG_BOB_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BOB_RangedDmg1_desc |
| BOB_MELEE_DMG4 | PRG_BOB_MeleeDmg4 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BOB_MeleeDmg4_desc |
| BOB_HEIGHTENED_SENSE | PRG_BOB_HeightenedSense | heightenedSense,BM_HeightenedSenses | 1.0,1.0 | 6 | 1 | 0 | 0 |  | PRG_HeightenedSense_Screen | AV_PRG_SpideySenseUnlock | PRG_BOB_HeightenedSense_desc |
| BOB_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| BOB_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| BOB_MELEE_DMG5 | PRG_BOB_MeleeDmg5 | meleeDamage | 0.25 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BOB_MeleeDmg5_desc |
| BOB_RUNSPEED2 | PRG_BOB_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_BOB_RunSpeed2_desc |
| BOB_SUPERJUMP3 | PRG_BOB_SuperJump3 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,64,11.3,10 | 5 | 0 | 0 | 1 |  | PRG_SuperJumpUpgrade2_Screen | AV_PRG_SuperJumpUpgrade2 | PRG_BOB_SuperJump3_desc |
| BOB_RUNSPEED1 | PRG_BOB_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_BOB_RunSpeed1_desc |
| BOB_SUPERJUMP2 | PRG_BOB_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,44,10,12 | 3 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_BOB_SuperJump2_desc |
| BOB_RICOCHET_BLOCK | PRG_BOB_RicochetBlock | blockRicochet | 1 | 4 | 0 | 0 | 1 |  | PRG_RicochetBlock_Screen | AV_PRG_RicochetBlock | PRG_BOB_RicochetBlock_desc |
| BOB_MELEE_DMG3 | PRG_BOB_MeleeDmg3 | meleeDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BOB_MeleeDmg3_desc |
| BOB_HEALTH5 | PRG_BOB_Health5 | health | 50 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BOB_Health5_desc |
| BOB_MELEE_DMG1 | PRG_BOB_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BOB_MeleeDmg1_desc |
| BOB_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| BOB_BLOCKBREAKER_COMBO | PRG_BOB_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 0 |  | PRG_BlockBreaker_Screen | AV_PRG_BlockBreaker | PRG_BOB_BlockBreakerCombo_desc |
| BOB_HEALTH1 | PRG_BOB_Health1 | health | 25 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BOB_Health1_desc |
| BOB_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_MrIncredible |  |
| BOB_RANGED_DMG5 | PRG_BOB_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.3,0.5 | 6 | 1 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BOB_RangedDmg5_desc |
| BOB_GROUNDPOUND2 | PRG_BOB_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 2 | 1 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_BOB_GroundPound2_desc |
| BOB_RANGED_DMG2 | PRG_BOB_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.3 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BOB_RangedDmg2_desc |
| BOB_SUPERJUMP1 | PRG_BOB_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 1 | 0 | 0 | 0 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_BOB_SuperJump1_desc |
| BOB_MELEE_DMG2 | PRG_BOB_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BOB_MeleeDmg2_desc |
| BOB_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| BOB_HEALTH4 | PRG_BOB_Health4 | health | 25 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BOB_Health4_desc |
| BOB_RANGED_DMG3 | PRG_BOB_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.2,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BOB_RangedDmg3_desc |
| BOB_HEALTH2 | PRG_BOB_Health2 | health | 25 | 2 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BOB_Health2_desc |
| BOB_RANGED_DMG4 | PRG_BOB_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.25,0.4 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BOB_RangedDmg4_desc |
| BOB_HEALTH3 | PRG_BOB_Health3 | health | 25 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BOB_Health3_desc |
