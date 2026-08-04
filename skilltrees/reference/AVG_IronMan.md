# AVG_IronMan

`ProgressionTree = "IN2_AVG_IronMan"` -- source: `gamedb/core/in2_avg_ironman.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| IRONMAN_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.4 |
| IRONMAN_BASEHEALTH | health | 150 |
| IRONMAN_BASERUNSPEED | runspeed | 6.5 |
| IRONMAN_BASECLIP | pulseClipSize | 6 |
| IRONMAN_BASESHIELD | shield,shieldRegenDelay,shieldRegen | 0.0,10.0,5.0 |
| IRONMAN_BASEPULSECHARGETIME | pulseChargeTime | 1 |
| IRONMAN_BASEFIRERATE | pulseFireRate | 2.5 |
| IRONMAN_FLIGHT1 | BM_Flier,flightSpeed,maxFlightSpeed,hoverSpeed | 1.0,24.00,31.00,10.00 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| IRONMAN_MAXSHIELD2 | PRG_IMN_MaxShield1 | shield | 15 | 3 | 0 | 0 | 0 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_IMN_MaxShield1_desc |
| IRONMAN_RANGED_DMG1 | PRG_IMN_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.05,0.1,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_IMN_RangedDmg1_desc |
| IRONMAN_SUPER_DMG1 | PRG_IMN_SuperDmg1 | specialDamage | 0.2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_IMN_SuperDmg1_desc |
| IRONMAN_SUPER_DMG2 | PRG_IMN_SuperDmg2 | specialDamage | 0.3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_IMN_SuperDmg2_desc |
| IRONMAN_GROUNDPOUND2 | PRG_IMN_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_IMN_GroundPound2_desc |
| IRONMAN_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| IRONMAN_MELEE_DMG1 | PRG_IMN_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_IMN_MeleeDmg1_desc |
| IRONMAN_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_IronMan |  |
| IRONMAN_HEALTH2 | PRG_IMN_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_IMN_Health2_desc |
| IRONMAN_SUPER_STRAIGHT_PUNCH | PRG_IMN_SuperStraightPunch | superStraightPunch | 1 | 4 | 0 | 0 | 0 |  | PRG_IMN_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_IMN_SuperStraightPunch_desc |
| IRONMAN_MAXSHIELD4 | PRG_IMN_MaxShield2 | shield | 15 | 6 | 0 | 0 | 0 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_IMN_MaxShield3_desc |
| IRONMAN_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| IRONMAN_SPECIAL_UNLOCKED | PRG_IMN_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 2 | 1 | 0 | 1 |  | PRG_IMN_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_IMN_SpecialUnlocked_desc |
| IRONMAN_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| IRONMAN_BLOCKBREAKER_COMBO | PRG_IMN_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 0 |  | PRG_IMN_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_IMN_BlockBreakerCombo_desc |
| IRONMAN_CHARGEDBOLT1 | PRG_IMN_ChargedBolt1 | allowPulseCharge,pulseChargeTime | 1,1.5 | 2 | 1 | 0 | 1 |  | PRG_IMN_ChargedBolt1_Screen | AV_PRG_RangedChargeUp | PRG_IMN_ChargedBolt1_desc |
| IRONMAN_GROUNDCOMBO6 | PRG_IMN_GroundCombo6 | groundCombo6 | 1 | 4 | 1 | 0 | 1 |  | PRG_IMN_GroundCombo6_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_IMN_GroundCombo6_desc |
| IRONMAN_RUNSPEED1 | PRG_IMN_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_IMN_RunSpeed1_desc |
| IRONMAN_DUALPULSE | PRG_IMN_DualPulse | pulseDual,multiShotDamage | 1,0.625 | 3 | 0 | 0 | 0 |  | PRG_IMN_DualPulse_Screen | AV_PRG_DualBlastUpgrade | PRG_IMN_DualPulse_desc |
| IRONMAN_MELEE_DMG3 | PRG_IMN_MeleeDmg3 | meleeDamage | 0.35 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_IMN_MeleeDmg3_desc |
| IRONMAN_MELEE_DMG2 | PRG_IMN_MeleeDmg2 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_IMN_MeleeDmg2_desc |
| IRONMAN_CHARGEDBOLT2 | PRG_IMN_ChargedBolt2 | pulseChargeTime | 0.75 | 4 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_IMN_ChargedBolt2_desc |
| IRONMAN_SUPER_DMG3 | PRG_IMN_SuperDmg3 | specialDamage | 0.5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_IMN_SuperDmg3_desc |
| IRONMAN_FIRERATE1 | PRG_IMN_FireRate1 | pulseFireRate | 3 | 3 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_IMN_FireRate1_desc |
| IRONMAN_SHIELDREGEN1 | PRG_IMN_ShieldRegen1 | shieldRegenDelay,shieldRegen | 6,10.0 | 5 | 0 | 0 | 0 |  | PRG_ShieldRegen1_Screen | AV_PRG_ShieldRegenUpgrade1 | PRG_IMN_ShieldRegen1_desc |
| IRONMAN_FLIGHT2 | PRG_IMN_FlightSpeed1 | flightSpeed,maxFlightSpeed,hoverSpeed | 31.00,38.00,12.00 | 3 | 0 | 0 | 0 |  | PRG_FlightSpeedUpgrade1_Screen | AV_PRG_FlightSpeedUpgrade1 | PRG_IMN_FlightSpeed1_desc |
| IRONMAN_RANGED_DMG2 | PRG_IMN_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.075,0.15,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_IMN_RangedDmg2_desc |
| IRONMAN_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| IRONMAN_HEALTH1 | PRG_IMN_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_IMN_Health1_desc |
| IRONMAN_HEALTH3 | PRG_IMN_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_IMN_Health3_desc |
| IRONMAN_MAXSHIELD3 | PRG_IMN_MaxShield2 | shield | 15 | 5 | 0 | 0 | 0 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_IMN_MaxShield2_desc |
| IRONMAN_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| IRONMAN_MAXSHIELD1 | PRG_IMN_BaseShield | shield | 15 | 2 | 0 | 0 | 1 | 1 | PRG_MaxShield1_Screen | AV_PRG_ShieldsUnlock | PRG_IMN_BaseShield_desc |
| IRONMAN_RANGED_DMG3 | PRG_IMN_RangedDmg3 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.125,0.25,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_IMN_RangedDmg3_desc |
