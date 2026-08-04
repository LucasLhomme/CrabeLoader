# TB_MickeyMouseInfinite

`ProgressionTree = "IN1_TB_MickeyMouse"` -- source: `gamedb/core/in1_tb_mickeymouse.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| MICKEY_BASERUNSPEED | runspeed | 7 |
| MICKEY_BASESUPERJUMP | BM_SuperJumper | 0 |
| MICKEY_BASEHEALTH | health | 150 |
| MICKEY_BASEHEALTHREGEN | healthRegen | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| MICKEY_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| MICKEY_RUNSPEED1 | PRG_MKY_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_MKY_RunSpeed1_desc |
| MICKEY_SUPERJUMP1 | PRG_MKY_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 4 | 1 | 0 | 1 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_MKY_SuperJump1_desc |
| MICKEY_HEALTH4 | PRG_MKY_Health4 | health | 60 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MKY_Health4_desc |
| MICKEY_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| MICKEY_MELEE_DMG1 | PRG_MKY_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MKY_MeleeDmg1_desc |
| MICKEY_MELEE_DMG4 | PRG_MKY_MeleeDmg4 | meleeDamage | 0.2 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MKY_MeleeDmg4_desc |
| MICKEY_HEALTH1 | PRG_MKY_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MKY_Health1_desc |
| MICKEY_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| MICKEY_HEALTH2 | PRG_MKY_Health2 | health | 30 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MKY_Health2_desc |
| MICKEY_MELEE_DMG2 | PRG_MKY_MeleeDmg2 | meleeDamage | 0.1 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MKY_MeleeDmg2_desc |
| MICKEY_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| MICKEY_RANGED_DMG5 | PRG_MKY_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.3,0.5 | 6 | 1 | 0 | 1 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MKY_RangedDmg5_desc |
| MICKEY_HEALTHREGEN2 | PRG_MKY_HealthRegen2 | healthRegen | 5 | 6 | 0 | 0 | 1 |  | PRG_HealthRegen2_Screen | AV_PRG_RegenUpgrade1 | PRG_MKY_HealthRegen2_desc |
| MICKEY_GROUNDPOUND2 | PRG_MKY_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 2 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_MKY_GroundPound2_desc |
| MICKEY_RANGED_DMG3 | PRG_MKY_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.2,0.3 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MKY_RangedDmg3_desc |
| MICKEY_RANGED_DMG2 | PRG_MKY_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MKY_RangedDmg2_desc |
| MICKEY_RANGED_DMG1 | PRG_MKY_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MKY_RangedDmg1_desc |
| MICKEY_HEALTH3 | PRG_MKY_Health3 | health | 30 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MKY_Health3_desc |
| MICKEY_MELEE_DMG5 | PRG_MKY_MeleeDmg5 | meleeDamage | 0.25 | 6 | 1 | 0 | 1 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MKY_MeleeDmg5_desc |
| MICKEY_MELEE_DMG3 | PRG_MKY_MeleeDmg3 | meleeDamage | 0.15 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MKY_MeleeDmg3_desc |
| MICKEY_HEALTHREGEN1 | PRG_MKY_HealthRegen1 | healthRegen | 2 | 5 | 1 | 0 | 0 |  | PRG_HealthRegen1_Screen | AV_PRG_RegenUnlock | PRG_MKY_HealthRegen1_desc |
| MICKEY_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_SorcererMickey |  |
| MICKEY_RANGED_DMG4 | PRG_MKY_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.25,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MKY_RangedDmg4_desc |
| MICKEY_RUNSPEED2 | PRG_MKY_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_MKY_RunSpeed2_desc |
