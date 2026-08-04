# AVG_Loki

`ProgressionTree = "IN2_AVG_Loki"` -- source: `gamedb/core/in2_avg_loki.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| LOKI_BASE_FLIGHT | BM_Flier | 0 |
| LOKI_BASE_RANGEDAMAGE | rangedDamage | 1 |
| LOKI_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.6 |
| LOKI_BASE_GROUNDPOUND | groundpound | 0 |
| LOKI_BASE_POWERDURATION | powerDuration | 15 |
| LOKI_BASE_RUNSPEED | runspeed | 6.5 |
| LOKI_BASE_HEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| LOKI_RUNSPEED1 | PRG_LK_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_LK_RunSpeed1_desc |
| LOKI_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| LOKI_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| LOKI_MELEE_DMG2 | PRG_LK_MeleeDmg2 | meleeDamage | 0.45 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LK_MeleeDmg2_desc |
| LOKI_RANGED_DMG3 | PRG_LK_RangedDmg3 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.125,0.25,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LK_RangedDmg3_desc |
| LOKI_HEALTH3 | PRG_LK_Health3 | health | 30 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_LK_Health3_desc |
| LOKI_FLIGHT1 | PRG_LK_Flight | BM_Flier,flight,flightSpeed,maxFlightSpeed,hoverSpeed | 0.0,1.0,24.00,31.00,3.00 | 5 | 1 |  | 0 |  | PRG_LK_Flight_Screen | AV_PRG_FlightUnlock | PRG_LK_Flight_desc |
| LOKI_GROUNDPOUND2 | PRG_LK_GroundPound2 | groundpound,groundpoundDamage | 1.00,2.0 | 4 | 0 | 0 | 0 |  | PRG_LK_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_LK_GroundPound2_desc |
| LOKI_DECOY_ATTACK | PRG_LK_DecoyAttack | decoyAttack | 1 | 6 | 1 | 0 | 1 |  | PRG_LK_DecoyAttack_Screen | AV_PRG_SuperDecoyAttack | PRG_LK_DecoyAttack_desc |
| LOKI_POWERDURATION2 | PRG_LK_PowerDuration2 | powerDuration | 25 | 6 | 0 | 0 | 0 |  | PRG_SuperMoveDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_LK_PowerDuration2_desc |
| LOKI_POWERDURATION1 | PRG_LK_PowerDuration1 | powerDuration | 20 | 4 | 0 | 0 | 0 |  | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_LK_PowerDuration1_desc |
| LOKI_SPECIAL_UNLOCKED | PRG_LK_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDuration | 1.0,1.0,90.0,15 | 2 | 1 | 0 | 1 |  | PRG_LK_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_LK_SpecialUnlocked_desc |
| LOKI_POWERFILL1 | PRG_PowerFill1 | powerFill | 75 | 4 | 0 |  |  |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| LOKI_FIRERATE1 | PRG_LK_FireRate1 | staffFireRate | 4 | 4 | 0 |  |  |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_LK_FireRate1_desc |
| LOKI_RANGED_DMG1 | PRG_LK_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.05,0.1,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LK_RangedDmg1_desc |
| LOKI_RANGED_DMG2 | PRG_LK_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.075,0.15,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LK_RangedDmg2_desc |
| LOKI_SUPER_STRAIGHT_PUNCH | PRG_LK_SuperStraightPunch | superStraightPunch | 1 | 5 | 0 | 0 | 1 |  | PRG_LK_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_LK_SuperStraightPunch_desc |
| LOKI_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| LOKI_CHARGED_CHARGETIME2 | PRG_LK_ChargedChargeTime2 | pulseChargeTime | 0.75 | 6 | 0 |  |  |  | PRG_ChargeSpeed2_Screen | AV_PRG_RangedCharge2 | PRG_LK_ChargedChargeTime2_desc |
| LOKI_CHARGED_UNLOCK | PRG_LK_ChargedUnlocked | allowPulseCharge, pulseChargeTime | 1, 1.5 | 3 | 1 | 0 | 1 |  | PRG_LK_ChargedUnlocked_Screen | AV_PRG_RangedChargeUp | PRG_LK_ChargedUnlocked_desc |
| LOKI_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Loki |  |
| LOKI_CHARGED_CHARGETIME1 | PRG_LK_ChargedChargeTime1 | pulseChargeTime | 1.2 | 5 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_LK_ChargedChargeTime1_desc |
| LOKI_MELEE_DMG1 | PRG_LK_MeleeDmg1 | meleeDamage | 0.25 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LK_MeleeDmg1_desc |
| LOKI_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| LOKI_HEALTH2 | PRG_LK_Health2 | health | 30 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_LK_Health2_desc |
| LOKI_BLOCKBREAKER_COMBO | PRG_LK_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 0 |  | PRG_LK_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_LK_BlockBreakerCombo_desc |
| LOKI_HEALTH1 | PRG_LK_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_LK_Health1_desc |
| LOKI_GROUNDCOMBO4 | PRG_LK_GroundCombo4 | groundCombo4 | 1 | 4 | 0 | 0 | 0 |  | PRG_LK_GroundCombo4_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_LK_GroundCombo4_desc |
| LOKI_HEALTH4 | PRG_LK_Health4 | health | 60 | 6 | 0 |  |  | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_LK_Health4_desc |
