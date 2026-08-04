# BRV_Merida

`ProgressionTree = "IN2_BRV_Merida"` -- source: `gamedb/core/in2_brv_merida.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| MERIDA_BASERICOCHET | ricochetShot,ricochetCount | 1.0,1.0 |
| MERIDA_BASEHEALTH | health | 150 |
| MERIDA_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.5 |
| MERIDA_BASERUNSPEED | runspeed | 6.5 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| MERIDA_ALLOWCHARGE | PRG_MRD_ChargedShot1 | allowArrowCharge,arrowCharge | 1, 2 | 2 | 0 | 0 | 1 |  | PRG_RangedChargeUp_Screen | AV_PRG_RangedChargeUp | PRG_MRD_ChargedShot1_desc |
| MERIDA_SUPER_DMG1 | PRG_MRD_SuperDmg1 | specialDamage | 0.35 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_MRD_SuperDmg1_desc |
| MERIDA_RUNSPEED2 | PRG_MRD_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_MRD_RunSpeed2_desc |
| MERIDA_MELEE_DMG3 | PRG_MRD_MeleeDmg3 | meleeDamage | 0.35 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MRD_MeleeDmg3_desc |
| MERIDA_RICOCHETSHOT3 | PRG_MRD_RicochetShot3 | ricochetCount | 5 | 4 | 0 | 0 | 0 |  | PRG_MRD_RicochetShot1_Screen | AV_PRG_RangedUpgrade3 | PRG_MRD_RicochetShot3_desc |
| MERIDA_SUPER_DMG3 | PRG_MRD_SuperDmg3 | specialDamage | 1 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_MRD_SuperDmg3_desc |
| MERIDA_RUNSPEED1 | PRG_MRD_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_MRD_RunSpeed1_desc |
| MERIDA_SUPER_DMG2 | PRG_MRD_SuperDmg2 | specialDamage | 0.65 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_MRD_SuperDmg2_desc |
| MERIDA_HEALTH1 | PRG_MRD_Health1 | health | 50 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MRD_Health1_desc |
| MERIDA_FREEZESHOT | PRG_MRD_FreezeShot1 | arrowFreezeAOE | 1 | 6 | 1 | 0 | 1 |  | PRG_MRD_FreezeShot1_Screen | AV_PRG_RangedAOEBlast | PRG_MRD_FreezeShot1_desc |
| MERIDA_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| MERIDA_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| MERIDA_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| MERIDA_MELEE_DMG1 | PRG_MRD_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MRD_MeleeDmg1_desc |
| MERIDA_RANGED_DMG2 | PRG_MRD_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.075,0.25,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MRD_RangedDmg2_desc |
| MERIDA_RANGED_DMG3 | PRG_MRD_RangedDmg3 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.125,0.35,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MRD_RangedDmg3_desc |
| MERIDA_GROUNDCOMBO4 | PRG_MRD_GroundCombo4 | groundCombo4 | 1 | 4 | 1 | 0 | 1 |  | PRG_MRD_GroundCombo4_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_MRD_GroundCombo4_desc |
| MERIDA_SPECIAL_UNLOCKED | PRG_MRD_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 2 | 1 | 0 | 1 |  | PRG_MRD_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_MRD_SpecialUnlocked_desc |
| MERIDA_CHARGESPEED1 | PRG_MRD_ChargeSpeed1 | arrowCharge | 1.5 | 4 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_MRD_ChargeSpeed1_desc |
| MERIDA_HEALTH2 | PRG_MRD_Health2 | health | 100 | 2 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MRD_Health2_desc |
| MERIDA_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| MERIDA_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Merida |  |
| MERIDA_SUPER_STRAIGHT_PUNCH | PRG_MRD_SuperStraightPunch | superStraightPunch | 1 | 4 | 0 | 0 | 0 |  | PRG_MRD_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_MRD_SuperStraightPunch_desc |
| MERIDA_MELEE_DMG2 | PRG_MRD_MeleeDmg2 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MRD_MeleeDmg2_desc |
| MERIDA_BLOCKBREAKER_COMBO | PRG_MRD_BlockBreakerCombo | blockBreakerCombo | 1 | 3 | 0 | 0 | 1 |  | PRG_MRD_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_MRD_BlockBreakerCombo_desc |
| MERIDA_RICOCHETSHOT2 | PRG_MRD_RicochetShot2 | ricochetCount | 3 | 2 | 0 | 0 | 0 |  | PRG_MRD_RicochetShot1_Screen | AV_PRG_RangedUpgrade2 | PRG_MRD_RicochetShot2_desc |
| MERIDA_RICOCHETSHOT1 | PRG_MRD_RicochetShot1 | ricochetCount | 2 | 1 | 1 | 0 | 0 |  | PRG_MRD_RicochetShot1_Screen | AV_PRG_RangedUpgrade1 | PRG_MRD_RicochetShot1_desc |
| MERIDA_CHARGESPEED2 | PRG_MRD_ChargeSpeed2 | arrowCharge | 0.75 | 5 | 0 | 0 | 0 |  | PRG_ChargeSpeed2_Screen | AV_PRG_RangedCharge2 | PRG_MRD_ChargeSpeed2_desc |
| MERIDA_RANGED_DMG1 | PRG_MRD_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.05,0.15,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MRD_RangedDmg1_desc |
| MERIDA_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| MERIDA_GROUNDPOUND2 | PRG_MRD_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_MRD_GroundPound2_desc |
