# AVG_Falcon

`ProgressionTree = "IN2_AVG_Falcon"` -- source: `gamedb/core/in2_avg_falcon.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| FALCON_BASE_RUNSPEED | runspeed | 7 |
| FALCON_BASE_FLIGHT | BM_Flier,flight,flightSpeed,maxFlightSpeed,hoverSpeed | 1.0,1.0,24.00,31.00,10.00 |
| FALCON_BASE_HEALTH | health | 150 |
| FALCON_BASE_GROUNDPOUND | groundpound | 0 |
| FALCON_BASE_SPREADFIRE | falconSpreadFire | 0 |
| FALCON_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.6 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| FALCON_SPECIAL_DMG2 | PRG_FAL_SpecialDmg2 | specialDamage | 0.45 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_FAL_SpecialDmg2_desc |
| FALCON_RANGED_DMG3 | PRG_FAL_RangedDmg3 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.25,0.125,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_FAL_RangedDmg3_desc |
| FALCON_RANGED_CHARGETIME2 | PRG_FAL_SpreadFireCharge2 | falconSpreadFireChargeTime | 1.25 | 5 | 0 | 0 | 0 |  | PRG_ChargeSpeed2_Screen | AV_PRG_RangedCharge2 | PRG_FAL_SpreadFireCharge2_desc |
| FALCON_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| FALCON_RANGED_PROJECTILE2 | PRG_FAL_SpreadProjectiles2 | falconSpreadFire | 2 | 5 | 0 | 0 | 1 |  | PRG_FAL_SpreadFireUnlocked_Screen | AV_PRG_RangedUpgrade2 | PRG_FAL_SpreadProjectiles2_desc |
| FALCON_SPECIAL_DMG1 | PRG_FAL_SpecialDmg1 | specialDamage | 0.3 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_FAL_SpecialDmg1_desc |
| FALCON_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| FALCON_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| FALCON_SPECIAL_DMG3 | PRG_FAL_SpecialDmg3 | specialDamage | 0.75 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_FAL_SpecialDmg3_desc |
| FALCON_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| FALCON_SPECIAL_UNLOCK | PRG_FAL_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 2 | 1 | 0 | 1 |  | PRG_FAL_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_FAL_SpecialUnlocked_desc |
| FALCON_GROUNDPOUND2 | PRG_FAL_GroundPound2 | groundpound,groundpoundDamage | 1.00,2.0 | 3 | 1 | 0 | 0 |  | PRG_FAL_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_FAL_GroundPound2_desc |
| FALCON_MELEE_DMG2 | PRG_FAL_MeleeDmg2 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_FAL_MeleeDmg2_desc |
| FALCON_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Falcon |  |
| FALCON_RUNSPEED1 | PRG_FAL_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_FAL_RunSpeed1_desc |
| FALCON_FLIGHTSPEED1 | PRG_FAL_FlightSpeed1 | flightSpeed,maxFlightSpeed,hoverSpeed | 31.00,38.00,12.00 | 1 | 0 | 0 | 0 |  | PRG_FlightSpeedUpgrade1_Screen | AV_PRG_FlightSpeedUpgrade1 | PRG_FAL_FlightSpeed1_desc |
| FALCON_CHARGEDATTACK | PRG_FAL_SpreadFireUnlocked | falconSpreadFireUnlock,falconSpreadFireChargeTime | 1,2.25 | 2 | 1 | 0 | 0 |  | PRG_FAL_SpreadFireUnlocked_Screen | AV_PRG_RangedChargeUp | PRG_FAL_SpreadFireUnlocked_desc |
| FALCON_GROUNDCOMBO5 | PRG_FAL_GroundCombo5 | groundCombo5 | 1 | 4 | 0 | 0 | 1 |  | PRG_FAL_GroundCombo5_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_FAL_GroundCombo5_desc |
| FALCON_HEALTH2 | PRG_FAL_Health2 | health | 30 | 2 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_FAL_Health2_desc |
| FALCON_RUNSPEED2 | PRG_FAL_RunSpeed2 | runspeed | 9 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_FAL_RunSpeed2_desc |
| FALCON_HEALTH1 | PRG_FAL_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_FAL_Health1_desc |
| FALCON_RANGED_DMG2 | PRG_FAL_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.15,0.075,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_FAL_RangedDmg2_desc |
| FALCON_BLOCKBREAKER_COMBO | PRG_FAL_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 0 |  | PRG_FAL_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_FAL_BlockBreakerCombo_desc |
| FALCON_MELEE_DMG1 | PRG_FAL_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_FAL_MeleeDmg1_desc |
| FALCON_FLIGHTSPEED2 | PRG_FAL_FlightSpeed2 | flightSpeed,maxFlightSpeed,hoverSpeed | 38.00,45.00,14.00 | 3 | 0 | 0 | 1 |  | PRG_FlightSpeedUpgrade2_Screen | AV_PRG_FlightSpeedUpgrade2 | PRG_FAL_FlightSpeed2_desc |
| FALCON_RANGED_PROJECTILE1 | PRG_FAL_SpreadProjectiles1 | falconSpreadFire | 1 | 3 | 0 | 0 | 0 |  | PRG_FAL_SpreadFireUnlocked_Screen | AV_PRG_RangedUpgrade1 | PRG_FAL_SpreadProjectiles1_desc |
| FALCON_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| FALCON_RANGED_DMG1 | PRG_FAL_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.1,0.05,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_FAL_RangedDmg1_desc |
| FALCON_HEALTH4 | PRG_FAL_Health4 | health | 60 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_FAL_Health4_desc |
| FALCON_HEALTH3 | PRG_FAL_Health3 | health | 30 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_FAL_Health3_desc |
| FALCON_RANGED_CHARGETIME1 | PRG_FAL_SpreadFireCharge1 | falconSpreadFireChargeTime | 1.75 | 3 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_FAL_SpreadFireCharge1_desc |
| FALCON_SUPER_STRAIGHT_PUNCH | PRG_FAL_SuperStraightPunch | superStraightPunch | 1 | 4 | 0 | 0 | 0 |  | PRG_FAL_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_FAL_SuperStraightPunch_desc |
| FALCON_MELEE_DMG3 | PRG_FAL_MeleeDmg3 | meleeDamage | 0.35 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_FAL_MeleeDmg3_desc |
