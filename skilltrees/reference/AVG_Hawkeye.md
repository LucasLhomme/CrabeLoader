# AVG_Hawkeye

`ProgressionTree = "IN2_AVG_Hawkeye"` -- source: `gamedb/core/in2_avg_hawkeye.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| HAWKEYE_BASECHARGETIME | arrowChargeTime | 2.5 |
| HAWKEYE_BASEFIRERATE | arrowFireRate | 1.75 |
| HAWKEYE_BASERUNSPEED | runspeed | 6.5 |
| HAWKEYE_BASEHEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| HAWKEYE_HEALTH1 | PRG_HKY_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_HKY_Health1_desc |
| HAWKEYE_GROUNDCOMBO5 | PRG_HKY_GroundCombo5 | groundCombo5 | 1 | 4 | 0 | 0 | 1 |  | PRG_HKY_GroundCombo5_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_HKY_GroundCombo5_desc |
| HAWKEYE_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| HAWKEYE_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Hawkeye |  |
| HAWKEYE_GROUNDPOUND2 | PRG_HKY_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 3 | 0 | 0 | 0 |  | PRG_HKY_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_HKY_GroundPound2_desc |
| HAWKEYE_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| HAWKEYE_HEALTH3 | PRG_HKY_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_HKY_Health3_desc |
| HAWKEYE_SUPER_DMG3 | PRG_HKY_SuperDmg3 | specialDamage | 0.75 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_HKY_SuperDmg3_desc |
| HAWKEYE_ALLOWCHARGE | PRG_HKY_ChargedShot1 | allowArrowCharge, arrowChargeTime | 1,2.00 | 2 | 0 | 0 | 0 |  | PRG_HKY_ChargedShot1_Screen | AV_PRG_RangedChargeUp | PRG_HKY_ChargedShot1_desc |
| HAWKEYE_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| HAWKEYE_SUPER_DMG2 | PRG_HKY_SuperDmg2 | specialDamage | 0.45 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_HKY_SuperDmg2_desc |
| HAWKEYE_SPREADSHOT | PRG_HKY_SpreadShot | arrowSpread,multiShotDamage | 1,0.5 | 4 | 1 | 0 | 1 |  | PRG_HKY_SpreadShot_Screen | AV_PRG_RangedAOEBlast | PRG_HKY_SpreadShot_desc |
| HAWKEYE_EXPLODEARROW | PRG_HKY_ExplodeArrow | arrowExplode | 1 | 6 | 1 | 0 | 1 |  | PRG_HKY_ExplodeArrow_Screen | AV_PRG_RangedUpgrade1 | PRG_HKY_ExplodeArrow_desc |
| HAWKEYE_RANGED_DMG2 | PRG_HKY_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HKY_RangedDmg2_desc |
| HAWKEYE_SPECIAL_UNLOCKED | PRG_HKY_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 2 | 1 | 0 | 1 |  | PRG_HKY_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_HKY_SpecialUnlocked_desc |
| HAWKEYE_MELEE_DMG1 | PRG_HKY_MeleeDmg1 | meleeDamage | 0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HKY_MeleeDmg1_desc |
| HAWKEYE_RANGED_DMG3 | PRG_HKY_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.25,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HKY_RangedDmg3_desc |
| HAWKEYE_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| HAWKEYE_BLOCKBREAKER_COMBO | PRG_HKY_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 0 |  | PRG_HKY_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_HKY_BlockBreakerCombo_desc |
| HAWKEYE_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| HAWKEYE_RANGED_DMG1 | PRG_HKY_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HKY_RangedDmg1_desc |
| HAWKEYE_FIRERATE1 | PRG_HKY_FireRate1 | arrowFireRate | 2 | 4 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_HKY_FireRate1_desc |
| HAWKEYE_SUPER_DMG1 | PRG_HKY_SuperDmg1 | specialDamage | 0.3 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_HKY_SuperDmg1_desc |
| HAWKEYE_ARROWCHARGETIME | PRG_HKY_ChargedShot2 | arrowChargeTime | 1.5 | 5 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_HKY_ChargedShot2_desc |
| HAWKEYE_HEALTH2 | PRG_HKY_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_HKY_Health2_desc |
| HAWKEYE_MELEE_DMG2 | PRG_HKY_MeleeDmg2 | meleeDamage | 0.45 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HKY_MeleeDmg2_desc |
| HAWKEYE_RUNSPEED1 | PRG_HKY_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_HKY_RunSpeed1_desc |
| HAWKEYE_SUPER_STRAIGHT_PUNCH | PRG_HKY_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 0 |  | PRG_HKY_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_HKY_SuperStraightPunch_desc |
