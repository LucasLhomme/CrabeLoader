# TBX_ClassicMickey

`ProgressionTree = "IN3_TBX_ClassicMickey"` -- source: `gamedb/core/in3_tbx_classicmickey.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| CLASSICMICKEY_BASE_FORCEPULL | forcePull | 0 |
| CLASSICMICKEY_BASEPARRY | MeleeParry | 0 |
| CLASSICMICKEY_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| CLASSICMICKEY_LAUNCHER | superStraightPunch | 1 |
| CLASSICMICKEY_BASE_HEALTH | health | 150 |
| CLASSICMICKEY_BASE_HEALTHREGEN | healthRegen | 0 |
| CLASSICMICKEY_FINISHER_COOLDOWN | finisherCooldown | 20 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| CLASSICMICKEY_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| CLASSICMICKEY_HEALTH2 | PRG_CMK_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_CMK_Health2_desc |
| CLASSICMICKEY_MELEE_DMG3 | PRG_CMK_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_CMK_MeleeDmg3_desc |
| CLASSICMICKEY_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| CLASSICMICKEY_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| CLASSICMICKEY_SPECIAL_UPG_DONALD | PRG_CMK_ImpSpecial_Donald | specialUpgradeDonald | 1 | 4 | 0 | 1 | 1 |  | PRG_SuperBlockhead_Screen | AV_PRG_SuperBlockhead | PRG_CMK_ImpSpecial_Donald_desc |
| CLASSICMICKEY_PARRY | PRG_CMK_Parry | repel | 1 | 2 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_CMK_Parry_desc |
| CLASSICMICKEY_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_ClassicMickey |  |
| CLASSICMICKEY_POWERFILL1 | PRG_PowerFill1 | powerFill | 60 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| CLASSICMICKEY_SPECIAL_UPG_OSWALD | PRG_CMK_ImpSpecial_Oswald | specialUpgradeOswald | 1 | 6 | 0 | 1 | 1 |  | PRG_SuperBlockhead_Screen | AV_PRG_SuperBlockhead | PRG_CMK_ImpSpecial_Oswald_desc |
| CLASSICMICKEY_CHARGEDTHROW | PRG_CMK_ChargedThrow | allowThrowCharge | 1.7 | 2 | 0 | 0 | 0 |  | PRG_RangedChargeUp_Screen | AV_PRG_RangedChargeUp | PRG_CMK_ChargedThrow_desc |
| CLASSICMICKEY_CHARGESPEED1 | PRG_CMK_ChargeSpeed1 | throwCharge | 1.3 | 4 | 0 | 0 | 0 |  | PRG_RangedCharge1_Screen | AV_PRG_RangedCharge1 | PRG_CMK_ChargeSpeed1_desc |
| CLASSICMICKEY_CHARGESPEED2 | PRG_CMK_ChargeSpeed2 | throwCharge | 0.75 | 6 | 0 | 0 | 0 |  | PRG_RangedCharge2_Screen | AV_PRG_RangedCharge2 | PRG_CMK_ChargeSpeed2_desc |
| CLASSICMICKEY_MELEE_DMG2 | PRG_CMK_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_CMK_MeleeDmg2_desc |
| CLASSICMICKEY_HEALTH3 | PRG_CMK_Health3 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_CMK_Health3_desc |
| CLASSICMICKEY_MELEE_DMG1 | PRG_CMK_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_CMK_MeleeDmg1_desc |
| CLASSICMICKEY_RANGED_DMG1 | PRG_CMK_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.2,0.1,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_CMK_RangedDmg1_desc |
| CLASSICMICKEY_SPECIAL_DMG1 | PRG_CMK_SuperDmg1 | ewokDamageMultiplier,blckHdSpecialDamage | 0.1,1 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_CMK_SuperDmg1_desc |
| CLASSICMICKEY_SPECIAL_UPG_GOOFY | PRG_CMK_ImpSpecial_Goofy | specialUpgradeGoofy | 1 | 4 | 0 | 1 | 1 |  | PRG_SuperBlockhead_Screen | AV_PRG_SuperBlockhead | PRG_CMK_ImpSpecial_Goofy_desc |
| CLASSICMICKEY_PAUSECOMBO2 | PRG_CMK_PauseCombo2 | PauseComboYY_YY | 1 | 3 | 1 | 0 | 0 |  | PRG_CMK_PauseCombo2_Screen | AV_PRG_PauseCombo1 | PRG_CMK_PauseCombo2_desc |
| CLASSICMICKEY_SPECIAL_DURATION2 | PRG_CMK_PowerDuration2 | powerDuration | 25 | 6 | 0 | 0 | 0 |  | PRG_SuperMoveDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_CMK_PowerDuration2_desc |
| CLASSICMICKEY_RANGED_DMG3 | PRG_CMK_RangedDmg3 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.5,0.25,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_CMK_RangedDmg3_desc |
| CLASSICMICKEY_RANGED_DMG2 | PRG_CMK_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.3,0.15,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_CMK_RangedDmg2_desc |
| CLASSICMICKEY_COMBO_FINISHER1 | PRG_CMK_ForceFinisher1 | finisherCombo,finisherCooldown | 1.0,20.0 | 4 | 1 | 0 | 1 |  | PRG_CMK_ForceFinisher1_Screen | AV_PRG_ComboFinisherUnlock | PRG_CMK_ForceFinisher1_desc |
| CLASSICMICKEY_COMBO_FINISHER2 | PRG_CMK_ForceFinisher2 | finisherCooldown | 15 | 6 | 0 | 0 | 1 |  | PRG_ComboFinisherUpgrade1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_CMK_ForceFinisher2_desc |
| CLASSICMICKEY_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| CLASSICMICKEY_SPECIAL_UNLOCKED | PRG_CMK_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDuration,blckHdSpecialDamage | 1.0,1.0,95.0,15.0,0 | 1 | 1 | 0 | 1 |  | PRG_CMK_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_CMK_SpecialUnlocked_desc |
| CLASSICMICKEY_SPECIAL_DMG2 | PRG_CMK_SuperDmg2 | ewokDamageMultiplier,blckHdSpecialDamage | 0.15,1 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_CMK_SuperDmg2_desc |
| CLASSICMICKEY_HEALTH1 | PRG_CMK_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_CMK_Health1_desc |
| CLASSICMICKEY_FORCE_PULL | PRG_CMK_ForcePull1 | forcePull | 1 | 2 | 0 | 0 | 0 |  | PRG_ForcePull_Screen | AV_PRG_ForcePull | PRG_CMK_ForcePull1_desc |
| CLASSICMICKEY_SPECIAL_DURATION1 | PRG_CMK_PowerDuration1 | powerDuration | 20 | 2 | 0 | 0 | 0 |  | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_CMK_PowerDuration1_desc |
| CLASSICMICKEY_ATHLETIC_JUMP | PRG_CMK_AthleticJump | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,44,10,12 | 2 | 0 | 0 | 0 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_CMK_AthleticJump_desc |
| CLASSICMICKEY_PAUSECOMBO1 | PRG_CMK_PauseCombo1 | PauseComboY_YY | 1 | 2 | 1 | 0 | 0 |  | PRG_CMK_PauseCombo1_Screen | AV_PRG_RangedPauseCombo1 | PRG_CMK_PauseCombo1_desc |
| CLASSICMICKEY_SPECIAL_DMG3 | PRG_CMK_SuperDmg3 | ewokDamageMultiplier,blckHdSpecialDamage | 0.25,1 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_CMK_SuperDmg3_desc |
