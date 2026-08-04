# MAL_Maleficent

`ProgressionTree = "IN2_MAL_Maleficent"` -- source: `gamedb/core/in2_mal_maleficent.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| MALEFICENT_BASERUNSPEED | runspeed | 6.5 |
| MALEFICENT_BASEHEALTH | health | 150 |
| MALEFICENT_FLIGHT0 | BM_Flier,flightSpeed,maxFlightSpeed,hoverSpeed | 0.0,24.00,31.00,10.00 |
| MALEFICENT_LEDGE_OBB | ledgeMinY,ledgeMaxY,ledgeJumpMinY,ledgeJumpMaxY | 1.0,2.4,1.5,2.3 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| MALEFICENT_SUPER_DMG3 | PRG_MLF_SuperDmg3 | specialDamage | 0.5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_MLF_SuperDmg3_desc |
| MALEFICENT_RANGED_DMG1 | PRG_MLF_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.2,0.1,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MLF_RangedDmg1_desc |
| MALEFICENT_SUPER_DMG2 | PRG_MLF_SuperDmg2 | specialDamage | 0.3 | 5 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_MLF_SuperDmg2_desc |
| MALEFICENT_HEALTH4 | PRG_MLF_Health4 | health | 60 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MLF_Health4_desc |
| MALEFICENT_FLIGHT1 | PRG_MLF_FlightSpeed1 | BM_Flier,flightSpeed,maxFlightSpeed,hoverSpeed | 1.0,24.00,31.00,10.00 | 3 | 1 | 0 | 0 |  | PRG_MLF_FlightSpeed1_Screen | AV_PRG_FlightUnlock | PRG_MLF_FlightSpeed1_desc |
| MALEFICENT_SUPER_STRAIGHT_PUNCH | PRG_MLF_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 0 |  | PRG_MLF_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_MLF_SuperStraightPunch_desc |
| MALEFICENT_RUNSPEED2 | PRG_MLF_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_MLF_RunSpeed2_desc |
| MALEFICENT_MELEE_DMG1 | PRG_MLF_MeleeDmg1 | meleeDamage | 0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MLF_MeleeDmg1_desc |
| MALEFICENT_SUPER_DMG1 | PRG_MLF_SuperDmg1 | specialDamage | 0.2 | 3 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_MLF_SuperDmg1_desc |
| MALEFICENT_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| MALEFICENT_HEALTH1 | PRG_MLF_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MLF_Health1_desc |
| MALEFICENT_BLOCKBREAKER_COMBO | PRG_MLF_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 1 |  | PRG_MLF_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_MLF_BlockBreakerCombo_desc |
| MALEFICENT_CHARGEDSHOT | PRG_MLF_ChargedShot | allowStaffCharge,staffCharge | 1,1.5 | 2 | 0 | 0 | 0 |  | PRG_MLF_ChargedShot_Screen | AV_PRG_RangedChargeUp | PRG_MLF_ChargedShot_desc |
| MALEFICENT_SPECIAL_UNLOCKED | PRG_MLF_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,150.0 | 2 | 1 | 0 | 1 |  | PRG_MLF_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_MLF_SpecialUnlocked_desc |
| MALEFICENT_RANGED_DMG3 | PRG_MLF_RangedDmg3 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.5,0.25,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MLF_RangedDmg3_desc |
| MALEFICENT_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| MALEFICENT_CHARGESPEED1 | PRG_MLF_ChargeSpeed1 | staffCharge | 1.2 | 4 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_MLF_ChargeSpeed1_desc |
| MALEFICENT_RANGED_DMG2 | PRG_MLF_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.3,0.15,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MLF_RangedDmg2_desc |
| MALEFICENT_HEALAURA_BLOCK | PRG_MLF_HealAuraBlock | blockHealthRegen,blockHealthDelay,blockHealthAura | 10.0,1.0,1.0 | 5 | 0 | 0 | 1 |  | PRG_MLF_HealAuraBlock_Screen | AV_PRG_HealAuraBlock | PRG_MLF_HealAuraBlock_desc |
| MALEFICENT_CHARGESPEED2 | PRG_MLF_ChargeSpeed2 | staffCharge | 0.75 | 6 | 0 | 0 | 0 |  | PRG_ChargeSpeed2_Screen | AV_PRG_RangedCharge2 | PRG_MLF_ChargeSpeed2_desc |
| MALEFICENT_HEALTH3 | PRG_MLF_Health3 | health | 30 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MLF_Health3_desc |
| MALEFICENT_POWERFILL1 | PRG_PowerFill1 | powerFill | 115 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| MALEFICENT_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| MALEFICENT_RUNSPEED1 | PRG_MLF_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_MLF_RunSpeed1_desc |
| MALEFICENT_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| MALEFICENT_MELEE_DMG2 | PRG_MLF_MeleeDmg2 | meleeDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MLF_MeleeDmg2_desc |
| MALEFICENT_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Maleficent |  |
| MALEFICENT_GROUNDPOUND2 | PRG_MLF_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 0 | 0 | 0 |  | PRG_MLF_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_MLF_GroundPound2_desc |
| MALEFICENT_HEALTH2 | PRG_MLF_Health2 | health | 30 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MLF_Health2_desc |
| MALEFICENT_MELEE_DMG3 | PRG_MLF_MeleeDmg3 | meleeDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MLF_MeleeDmg3_desc |
