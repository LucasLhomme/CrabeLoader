# AVG_Thor

`ProgressionTree = "IN2_AVG_Thor"` -- source: `gamedb/core/in2_avg_thor.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| THOR_FLIGHT1 | BM_Flier,flight,flightSpeed,maxFlightSpeed,hoverSpeed | 1.0,1.0,24.00,31.00,10.00 |
| THOR_BASERUNSPEED | runspeed | 7 |
| THOR_BASEHEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| THOR_RANGED_DMG2 | PRG_THR_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_THR_RangedDmg2_desc |
| THOR_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| THOR_HEALTH4 | PRG_THR_Health4 | health | 25 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_THR_Health4_desc |
| THOR_MJOLNIR_LIGHTNING | PRG_THR_MjolnirLightning | mjolnirLightning,mjolnirUpgrade | 0.125,3.0 | 2 | 1 | 0 | 1 |  | PRG_THR_MjolnirLightning_Screen | AV_PRG_RangedChargeUp | PRG_THR_MjolnirLightning_desc |
| THOR_RANGED_DMG1 | PRG_THR_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_THR_RangedDmg1_desc |
| THOR_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| THOR_MELEE_DMG3 | PRG_THR_MeleeDmg3 | meleeDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_THR_MeleeDmg3_desc |
| THOR_FLIGHT2 | PRG_THR_FlightSpeed | flightSpeed,maxFlightSpeed,hoverSpeed | 31.00,38.00,12.00 | 3 | 0 | 0 | 0 |  | PRG_FlightSpeedUpgrade1_Screen | AV_PRG_FlightSpeedUpgrade1 | PRG_THR_FlightSpeed_desc |
| THOR_GROUNDPOUND2 | PRG_THR_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 0 | 0 | 0 |  | PRG_THR_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_THR_GroundPound2_desc |
| THOR_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| THOR_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Thor |  |
| THOR_GROUNDCOMBO5 | PRG_THR_GroundCombo5 | groundCombo5 | 1 | 4 | 1 | 0 | 1 |  | PRG_THR_GroundCombo5_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_THR_GroundCombo5_desc |
| THOR_SUPER_DMG3 | PRG_THR_SuperDmg3 | specialDamage | 0.75 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_THR_SuperDmg3_desc |
| THOR_SUPER_DMG2 | PRG_THR_SuperDmg2 | specialDamage | 0.45 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_THR_SuperDmg2_desc |
| THOR_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| THOR_RANGED_DMG3 | PRG_THR_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.25,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_THR_RangedDmg3_desc |
| THOR_MELEE_DMG2 | PRG_THR_MeleeDmg2 | meleeDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_THR_MeleeDmg2_desc |
| THOR_POWERDURATION2 | PRG_THR_PowerDuration2 | powerDuration | 15 | 5 | 0 | 0 | 0 |  | PRG_SuperMoveDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_THR_PowerDuration2_desc |
| THOR_SPECIAL_UNLOCKED | PRG_THR_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,150.0 | 2 | 1 | 0 | 1 |  | PRG_THR_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_THR_SpecialUnlocked_desc |
| THOR_SUPER_DMG1 | PRG_THR_SuperDmg1 | specialDamage | 0.3 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_THR_SuperDmg1_desc |
| THOR_POWERDURATION1 | PRG_THR_PowerDuration1 | powerDuration | 10 | 3 | 0 | 0 | 0 |  | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_THR_PowerDuration1_desc |
| THOR_POWERFILL1 | PRG_PowerFill1 | powerFill | 115 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| THOR_HEALTH3 | PRG_THR_Health3 | health | 25 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_THR_Health3_desc |
| THOR_MJOLNIR_BOOMERANG | PRG_THR_MjolnirBoomerang | mjolnirBoomerang | 1 | 6 | 0 | 0 | 0 |  | PRG_THR_MjolnirBoomerang_Screen | AV_PRG_RangedUpgrade1 | PRG_THR_MjolnirBoomerang_desc |
| THOR_BLOCKBREAKER_COMBO | PRG_THR_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 1 |  | PRG_THR_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_THR_BlockBreakerCombo_desc |
| THOR_MJOLNIR_UPGRADE1 | PRG_THR_MjolnirUpgrade | mjolnirUpgrade | 2 | 5 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_THR_MjolnirUpgrade_desc |
| THOR_HEALTH1 | PRG_THR_Health1 | health | 25 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_THR_Health1_desc |
| THOR_MELEE_DMG1 | PRG_THR_MeleeDmg1 | meleeDamage | 0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_THR_MeleeDmg1_desc |
| THOR_RUNSPEED1 | PRG_THR_RunSpeed1 | runspeed | 9 | 2 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_THR_RunSpeed1_desc |
| THOR_SUPER_PUNCH_UPGRADE | PRG_THR_SuperPunchUpgrade | superStraightPunch | 1 | 4 | 1 | 0 | 0 |  | PRG_THR_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_THR_SuperPunchUpgrade_desc |
| THOR_HEALTH2 | PRG_THR_Health2 | health | 25 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_THR_Health2_desc |
| THOR_HEALTH5 | PRG_THR_Health5 | health | 50 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_THR_Health5_desc |
