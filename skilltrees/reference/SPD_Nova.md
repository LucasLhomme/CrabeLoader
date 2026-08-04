# SPD_Nova

`ProgressionTree = "IN2_SPD_Nova"` -- source: `gamedb/core/in2_spd_nova.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| NOVA_BASEFLIGHT | BM_Flier,flightSpeed,maxFlightSpeed,hoverSpeed | 1.0,24.00,31.00,10.00 |
| NOVA_BASECLIP | blastClipSize | 6 |
| NOVA_BASECHARGETIME | pulseBeamCharge | 1 |
| NOVA_BASEHEALTH | health | 150 |
| NOVA_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.4 |
| NOVA_BASERUNSPEED | runspeed | 6.5 |
| NOVA_BASEFIRERATE | blastFireRate | 2.5 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| NOVA_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| NOVA_FLIGHT2 | PRG_NVA_FlightSpeed2 | flightSpeed,maxFlightSpeed,hoverSpeed | 38.00,45.00,14.00 | 3 | 0 | 0 | 0 |  | PRG_NVA_FlightSpeed2_Screen | AV_PRG_FlightSpeedUpgrade2 | PRG_NVA_FlightSpeed2_desc |
| NOVA_FLIGHT1 | PRG_NVA_FlightSpeed1 | flightSpeed,maxFlightSpeed,hoverSpeed | 31.00,38.00,12.00 | 1 | 0 | 0 | 0 |  | PRG_NVA_FlightSpeed1_Screen | AV_PRG_FlightSpeedUpgrade1 | PRG_NVA_FlightSpeed1_desc |
| NOVA_RUNSPEED1 | PRG_NVA_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_NVA_RunSpeed1_desc |
| NOVA_SPECIAL_HEAL3 | PRG_NVA_SpecialHeal3 | specialHeal | 0.5 | 6 | 0 | 0 | 1 |  | PRG_NVA_SpecialHeal3_Screen | AV_PRG_SuperMoveUpgrade3 | PRG_NVA_SpecialHeal3_desc |
| NOVA_HEALTH3 | PRG_NVA_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_NVA_Health3_desc |
| NOVA_MELEE_DMG1 | PRG_NVA_MeleeDmg1 | meleeDamage | 0.3 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_NVA_MeleeDmg1_desc |
| NOVA_SPECIAL_UNLOCKED | PRG_NVA_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 2 | 1 | 0 | 1 |  | PRG_NVA_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_NVA_SpecialUnlocked_desc |
| NOVA_CHARGESPEED2 | PRG_NVA_ChargeSpeed2 | pulseBeamCharge | 0.75 | 6 | 0 | 0 | 0 |  | PRG_ChargeSpeed2_Screen | AV_PRG_RangedCharge2 | PRG_NVA_ChargeSpeed2_desc |
| NOVA_SUPER_STRAIGHT_PUNCH | PRG_NVA_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 1 |  | PRG_NVA_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_NVA_SuperStraightPunch_desc |
| NOVA_MELEE_DMG2 | PRG_NVA_MeleeDmg2 | meleeDamage | 0.4 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_NVA_MeleeDmg2_desc |
| NOVA_HEALTH1 | PRG_NVA_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_NVA_Health1_desc |
| NOVA_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| NOVA_RANGED_DMG1 | PRG_NVA_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.05,0.1,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_NVA_RangedDmg1_desc |
| NOVA_BLOCKBREAKER_COMBO | PRG_NVA_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 0 |  | PRG_NVA_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_NVA_BlockBreakerCombo_desc |
| NOVA_CHARGESPEED1 | PRG_NVA_ChargeSpeed1 | pulseBeamCharge | 1.2 | 4 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_NVA_ChargeSpeed1_desc |
| NOVA_FIRERATE1 | PRG_NVA_FireRate1 | blastFireRate | 3 | 3 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_NVA_FireRate1_desc |
| NOVA_SUPER_DMG2 | PRG_NVA_SuperDmg2 | specialDamage | 0.3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_NVA_SuperDmg2_desc |
| NOVA_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| NOVA_SUPER_DMG1 | PRG_NVA_SuperDmg1 | specialDamage | 0.2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_NVA_SuperDmg1_desc |
| NOVA_SPECIAL_HEAL2 | PRG_NVA_SpecialHeal2 | specialHeal | 0.3 | 5 | 0 | 0 | 0 |  | PRG_NVA_SpecialHeal2_Screen | AV_PRG_SuperMoveUpgrade2 | PRG_NVA_SpecialHeal2_desc |
| NOVA_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| NOVA_SPECIAL_HEAL1 | PRG_NVA_SpecialHeal1 | specialHeal | 0.2 | 3 | 0 | 0 | 0 |  | PRG_NVA_SpecialHeal1_Screen | AV_PRG_SuperMoveUpgrade1 | PRG_NVA_SpecialHeal1_desc |
| NOVA_RANGED_DMG3 | PRG_NVA_RangedDmg3 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.125,0.25,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_NVA_RangedDmg3_desc |
| NOVA_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Nova |  |
| NOVA_SUPER_DMG3 | PRG_NVA_SuperDmg3 | specialDamage | 0.5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_NVA_SuperDmg3_desc |
| NOVA_DUALBLAST | PRG_NVA_DualBlast | blastDual,multiShotDamage | 1,0.625 | 3 | 1 | 0 | 1 |  | PRG_NVA_DualBlast_Screen | AV_PRG_DualBlastUpgrade | PRG_NVA_DualBlast_desc |
| NOVA_HEALTH2 | PRG_NVA_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_NVA_Health2_desc |
| NOVA_RANGED_DMG2 | PRG_NVA_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.075,0.15,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_NVA_RangedDmg2_desc |
| NOVA_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| NOVA_CHARGEDBLAST | PRG_NVA_ChargedBlast | allowBlastCharge,pulseBeamCharge | 1,1.5 | 2 | 0 | 0 | 0 |  | PRG_NVA_ChargedBlast_Screen | AV_PRG_RangedChargeUp | PRG_NVA_ChargedBlast_desc |
| NOVA_GROUNDPOUND2 | PRG_NVA_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_NVA_GroundPound2_desc |
