# TRN_Quorra

`ProgressionTree = "IN2_TRN_Quorra"` -- source: `gamedb/core/in2_trn_quorra.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| QUORRA_LEDGE_OBB | ledgeMinY,ledgeMaxY,ledgeJumpMinY,ledgeJumpMaxY | 1.0,2.4,1.5,2.3 |
| QUORRA_BASESUPERJUMP | BM_SuperJumper | 0 |
| QUORRA_BASEHEALTH | health | 150 |
| QUORRA_BASEMELEE_DMG | meleeDamage | 1 |
| QUORRA_BASEWEBSWING | BM_WebSwinger | 0 |
| QUORRA_BASERANGED_DMG | rangedDamage,sharedRangedDamage | 1.0,1.0 |
| QUORRA_BASERUNSPEED | runspeed | 7 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| QUORRA_RANGED_DMG2 | PRG_QRA_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_QRA_RangedDmg2_desc |
| QUORRA_RUNSPEED1 | PRG_QRA_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_QRA_RunSpeed1_desc |
| QUORRA_SUPER_STRAIGHT_PUNCH | PRG_QRA_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 0 |  | PRG_QRA_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_QRA_SuperStraightPunch_desc |
| QUORRA_SPECIAL_UNLOCKED | PRG_QRA_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDuration,poweredUpRunSpeed | 1.0,1.0,120.0,8.0,13.0 | 2 | 1 | 0 | 0 |  | PRG_QRA_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_QRA_SpecialUnlocked_desc |
| QUORRA_SUPERJUMP1 | PRG_QRA_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,44,10,12 | 3 | 0 | 0 | 0 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_QRA_SuperJump1_desc |
| QUORRA_MELEE_DMG2 | PRG_QRA_MeleeDmg2 | meleeDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_QRA_MeleeDmg2_desc |
| QUORRA_WEBSWING | PRG_QRA_WebSwing | webSwing,BM_WebSwinger | 1,1 | 5 | 1 | 0 | 1 |  | PRG_QRA_WebSwing_Screen | AV_PRG_WebSwingUnlock | PRG_QRA_WebSwing_desc |
| QUORRA_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| QUORRA_SPECIAL_DMG3 | PRG_QRA_SuperDmg3 | specialDamage | 0.25 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_QRA_SuperDmg3_desc |
| QUORRA_GROUNDPOUND2 | PRG_QRA_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 3 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_QRA_GroundPound2_desc |
| QUORRA_HEALTH2 | PRG_QRA_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_QRA_Health2_desc |
| QUORRA_MELEE_DMG1 | PRG_QRA_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_QRA_MeleeDmg1_desc |
| QUORRA_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| QUORRA_SPECIAL_DMG2 | PRG_QRA_SuperDmg2 | specialDamage | 0.15 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_QRA_SuperDmg2_desc |
| QUORRA_SPECIAL_DMG1 | PRG_QRA_SuperDmg1 | specialDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_QRA_SuperDmg1_desc |
| QUORRA_HEALTH3 | PRG_QRA_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_QRA_Health3_desc |
| QUORRA_HEALTH1 | PRG_QRA_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_QRA_Health1_desc |
| QUORRA_MELEE_DMG3 | PRG_QRA_MeleeDmg3 | meleeDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_QRA_MeleeDmg3_desc |
| QUORRA_LIGHTCABLE_UPGRADE | PRG_QRA_LightCablePull | lightCablePull | 1 | 4 | 0 | 0 | 1 |  | PRG_QRA_LightCablePull_Screen | AV_PRG_RangedUpgrade1 | PRG_QRA_LightCablePull_desc |
| QUORRA_GROUNDCOMBO5 | PRG_QRA_GroundCombo6 | groundCombo5 | 1 | 4 | 1 | 0 | 1 |  | PRG_QRA_GroundCombo6_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_QRA_GroundCombo6_desc |
| QUORRA_POWERDURATION1 | PRG_QRA_PowerDuration1 | powerDuration | 15 | 4 | 0 | 0 | 1 |  | PRG_PowerDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_QRA_PowerDuration1_desc |
| QUORRA_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| QUORRA_BLOCKBREAKER_COMBO | PRG_QRA_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 0 |  | PRG_QRA_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_QRA_BlockBreakerCombo_desc |
| QUORRA_RANGED_DMG1 | PRG_QRA_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_QRA_RangedDmg1_desc |
| QUORRA_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| QUORRA_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Quorra |  |
| QUORRA_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| QUORRA_POWERDURATION2 | PRG_QRA_PowerDuration2 | powerDuration | 20 | 6 | 0 | 0 | 0 |  | PRG_PowerDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_QRA_PowerDuration2_desc |
| QUORRA_RANGED_DMG3 | PRG_QRA_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.25,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_QRA_RangedDmg3_desc |
