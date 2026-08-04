# TB_Tinkerbell

`ProgressionTree = "IN2_TB_Tinkerbell"` -- source: `gamedb/core/in2_tb_tinkerbell.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| TINKERBELL_BASERUNSPEED | runspeed | 6.5 |
| TINKERBELL_BASEFLIGHT | BM_Flier,flight,flightSpeed,maxFlightSpeed,hoverSpeed | 1.0,1.0,24.00,31.00,10.00 |
| TINKERBELL_LEDGE_OBB | ledgeMinY,ledgeMaxY,ledgeJumpMinY,ledgeJumpMaxY | 1.0,2.4,1.5,2.3 |
| TINKERBELL_BASERANGED_DMG | rangedDamage,sharedRangedDamage | 1.0,1.0 |
| TINKERBELL_BASEHEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| TINKERBELL_RUNSPEED2 | PRG_TNK_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_TNK_RunSpeed2_desc |
| TINKERBELL_RANGED_DMG2 | PRG_TNK_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.65,0.3,1 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_TNK_RangedDmg2_desc |
| TINKERBELL_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| TINKERBELL_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| TINKERBELL_SUPER_DMG3 | PRG_TNK_SuperDmg3 | specialDamage | 1 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_TNK_SuperDmg3_desc |
| TINKERBELL_POWERFILL1 | PRG_PowerFill1 | powerFill | 75 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| TINKERBELL_SEEKINGSHOT3 | PRG_TNK_SeekingShot3 | pixieDustShots | 5 | 6 | 0 | 0 | 0 |  | PRG_TNK_SeekingShot1_Screen | AV_PRG_RangedUpgrade3 | PRG_TNK_SeekingShot3_desc |
| TINKERBELL_GROUNDPOUND2 | PRG_TNK_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 0 | 0 | 0 |  | PRG_TNK_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_TNK_GroundPound2_desc |
| TINKERBELL_SUPERRICOCHET2 | PRG_TNK_SuperRicochet2 | specialRicochet | 5 | 4 | 0 | 0 | 0 |  | PRG_RicochetBlock_Screen | AV_PRG_SuperMoveUpgrade2 | PRG_TNK_SuperRicochet2_desc |
| TINKERBELL_SPECIAL_UNLOCKED | PRG_TNK_SpecialUnlocked | specialUnlocked,powerBar,powerFill,specialRicochet | 1.0,1.0,90.0,3.0 | 2 | 1 | 0 | 1 |  | PRG_TNK_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_TNK_SpecialUnlocked_desc |
| TINKERBELL_MELEE_DMG1 | PRG_TNK_MeleeDmg1 | meleeDamage | 0.3 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_TNK_MeleeDmg1_desc |
| TINKERBELL_HEALTH2 | PRG_TNK_Health2 | health | 100 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_TNK_Health2_desc |
| TINKERBELL_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Tinkerbell |  |
| TINKERBELL_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| TINKERBELL_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| TINKERBELL_MELEE_DMG2 | PRG_TNK_MeleeDmg2 | meleeDamage | 0.5 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_TNK_MeleeDmg2_desc |
| TINKERBELL_HEALTH1 | PRG_TNK_Health1 | health | 50 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_TNK_Health1_desc |
| TINKERBELL_RANGED_DMG1 | PRG_TNK_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.35,0.2,0.75 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_TNK_RangedDmg1_desc |
| TINKERBELL_SUPER_DMG1 | PRG_TNK_SuperDmg1 | specialDamage | 0.35 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_TNK_SuperDmg1_desc |
| TINKERBELL_SUPER_STRAIGHT_PUNCH | PRG_TNK_SuperStraightPunch | superStraightPunch | 1 | 4 | 0 | 0 | 0 |  | PRG_TNK_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_TNK_SuperStraightPunch_desc |
| TINKERBELL_SEEKINGSHOT1 | PRG_TNK_SeekingShot1 | pixieDustShots | 3 | 2 | 0 | 0 | 0 |  | PRG_TNK_SeekingShot1_Screen | AV_PRG_RangedUpgrade1 | PRG_TNK_SeekingShot1_desc |
| TINKERBELL_CHARGESPEED2 | PRG_TNK_ChargeSpeed2 | pixieDustCharge | 1.5 | 6 | 0 | 0 | 0 |  | PRG_RangedChargeUp_Screen | AV_PRG_RangedCharge2 | PRG_TNK_ChargeSpeed2_desc |
| TINKERBELL_CHARGESPEED1 | PRG_TNK_ChargeSpeed1 | pixieDustCharge | 1.75 | 4 | 0 | 0 | 0 |  | PRG_RangedChargeUp_Screen | AV_PRG_RangedCharge1 | PRG_TNK_ChargeSpeed1_desc |
| TINKERBELL_SUPERRICOCHET3 | PRG_TNK_SuperRicochet3 | specialRicochet | 6 | 6 | 0 | 0 | 0 |  | PRG_RicochetBlock_Screen | AV_PRG_SuperMoveUpgrade3 | PRG_TNK_SuperRicochet3_desc |
| TINKERBELL_SEEKINGSHOT2 | PRG_TNK_SeekingShot2 | pixieDustShots | 4 | 4 | 0 | 0 | 0 |  | PRG_TNK_SeekingShot1_Screen | AV_PRG_RangedUpgrade2 | PRG_TNK_SeekingShot2_desc |
| TINKERBELL_FLIGHT1 | PRG_TNK_FlightSpeed1 | flightSpeed,maxFlightSpeed,hoverSpeed | 31.00,38.00,12.00 | 2 | 1 | 0 | 0 |  | PRG_TNK_FlightSpeed1_Screen | AV_PRG_FlightSpeedUpgrade1 | PRG_TNK_FlightSpeed1_desc |
| TINKERBELL_RUNSPEED1 | PRG_TNK_RunSpeed1 | runspeed | 8 | 2 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_TNK_RunSpeed1_desc |
| TINKERBELL_BLOCKBREAKER_COMBO | PRG_TNK_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 0 |  | PRG_TNK_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_TNK_BlockBreakerCombo_desc |
| TINKERBELL_SUPERRICOCHET1 | PRG_TNK_SuperRicochet1 | specialRicochet | 4 | 2 | 0 | 0 | 0 |  | PRG_RicochetBlock_Screen | AV_PRG_SuperMoveUpgrade1 | PRG_TNK_SuperRicochet1_desc |
| TINKERBELL_SUPER_DMG2 | PRG_TNK_SuperDmg2 | specialDamage | 0.65 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_TNK_SuperDmg2_desc |
| TINKERBELL_HEALAURA_BLOCK | PRG_TNK_HealAuraBlock | blockHealthRegen,blockHealthDelay,blockHealthAura | 10,1.0,1.0 | 3 | 0 | 0 | 1 |  | PRG_TNK_HealAuraBlock_Screen | AV_PRG_HealAuraBlock | PRG_TNK_HealAuraBlock_desc |
| TINKERBELL_STRENGTH | PRG_TNK_Strength | BM_SuperDuperStrong | 1 | 5 | 1 | 0 | 0 |  | PRG_TNK_Strength_Screen | AV_PRG_PickupThrowUpgrade1 | PRG_TNK_Strength_desc |
| TINKERBELL_CHARGEDSHOT | PRG_TNK_ChargedShot1 | allowPixieDustCharge,pixieDustShots | 1.0,2 | 2 | 1 | 0 | 1 |  | PRG_TNK_ChargedShot1_Screen | AV_PRG_RangedChargeUp | PRG_TNK_ChargedShot1_desc |
