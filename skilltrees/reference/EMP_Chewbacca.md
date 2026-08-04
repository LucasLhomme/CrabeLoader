# EMP_Chewbacca

`ProgressionTree = "IN3_EMP_Chewbacca"` -- source: `gamedb/core/in3_emp_chewbacca.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| CHEWBACCA_BASEPARRY | MeleeParry | 0 |
| CHEWBACCA_BASERUNSPEED | runspeed | 8 |
| CHEWBACCA_BLOCKBREAKER | blockBreakerCombo | 1 |
| CHEWBACCA_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.4 |
| CHEWBACCA_BASE_FIRERATE | fireRate | 2.75 |
| CHEWBACCA_BASE_CLIP | clipSize | 10 |
| CHEWBACCA_BASEHEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| CHEWBACCA_COMBO_FINISHER2 | PRG_CHW_ForceFinisher2 | finisherCooldown | 15 | 5 | 0 | 0 | 0 |  | PRG_ComboFinisherUpgrade1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_CHW_ForceFinisher2_desc |
| CHEWBACCA_HEALTH1 | PRG_CHW_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_CHW_Health1_desc |
| CHEWBACCA_PAUSECOMBO1 | PRG_CHW_PauseCombo1 | PauseComboY_YY | 1 | 3 | 0 | 0 | 0 |  | PRG_CHW_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_CHW_PauseCombo1_desc |
| CHEWBACCA_CHARGEDSHOT2 | PRG_CHW_ChargedShot2 | rangedChargeTime | 0.75 | 5 | 0 | 0 | 0 |  | PRG_RangedCharge1_Screen | AV_PRG_RangedCharge1 | PRG_CHW_ChargedShot2_desc |
| CHEWBACCA_PARRY | PRG_CHW_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_CHW_Parry_desc |
| CHEWBACCA_SPECIAL_UNLOCKED | PRG_CHW_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,150.0 | 1 | 1 | 0 | 1 |  | PRG_CHW_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_CHW_SpecialUnlocked_desc |
| CHEWBACCA_FIRERATE1 | PRG_LUK_FireRate1 | fireRate | 3.25 | 2 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_LUK_FireRate1_desc |
| CHEWBACCA_RANGED_DMG1 | PRG_CHW_RangedDmg1 | blasterShot,rangedDamage,chargedRangedDamage,sharedRangedDamage | 1,0.05,0.15,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_CHW_RangedDmg1_desc |
| CHEWBACCA_SPECIAL_DMG1 | PRG_CHW_SuperDmg1 | specialDamage | 0.2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_CHW_SuperDmg1_desc |
| CHEWBACCA_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| CHEWBACCA_POWERFILL1 | PRG_PowerFill1 | powerFill | 120 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| CHEWBACCA_COMBO_FINISHER1 | PRG_CHW_ForceFinisher1 | finisherCombo,finisherCooldown | 1.0,20.0 | 3 | 1 | 0 | 0 |  | PRG_CHW_ForceFinisher1_Screen | AV_PRG_ComboFinisherUnlock | PRG_CHW_ForceFinisher1_desc |
| CHEWBACCA_PARRY_COUNTER | PRG_CHW_ParryCounter | repelAttack | 1 | 4 | 1 | 0 | 0 |  | PRG_CHW_ParryCounter_Screen | AV_PRG_ParryCounter | PRG_CHW_ParryCounter_desc |
| CHEWBACCA_HEALTH3 | PRG_CHW_Health3 | health | 70 | 4 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_CHW_Health3_desc |
| CHEWBACCA_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| CHEWBACCA_MELEE_DMG2 | PRG_CHW_MeleeDmg2 | meleeHit,meleeDamage | 1,0.2 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_CHW_MeleeDmg2_desc |
| CHEWBACCA_CLIP2 | PRG_LUK_BlasterClip2 | clipSize | 16 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_LUK_BlasterClip2_desc |
| CHEWBACCA_CHARGEDSHOT1 | PRG_CHW_ChargedShot1 | allowRangedCharge,rangedChargeTime | 1.0,1.5 | 2 | 1 | 0 | 0 |  | PRG_CHW_ChargedShot1_Screen | AV_PRG_RangedChargeUp | PRG_CHW_ChargedShot1_desc |
| CHEWBACCA_RANGED_DMG3 | PRG_CHW_RangedDmg3 | blasterShot,rangedDamage,chargedRangedDamage,sharedRangedDamage | 1,0.125,0.35,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_CHW_RangedDmg3_desc |
| CHEWBACCA_POWERFILL2 | PRG_PowerFill2 | powerFill | 95 | 5 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate2_Screen | AV_PRG_SuperMeterFillRate2 | PRG_PowerFill2_desc |
| CHEWBACCA_CLIP1 | PRG_LUK_BlasterClip1 | clipSize | 12 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_LUK_BlasterClip1_desc |
| CHEWBACCA_RANGED_DMG2 | PRG_CHW_RangedDmg2 | blasterShot,rangedDamage,chargedRangedDamage,sharedRangedDamage | 1,0.075,0.25,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_CHW_RangedDmg2_desc |
| CHEWBACCA_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| CHEWBACCA_FIRERATE2 | PRG_LUK_FireRate2 | fireRate | 3.75 | 4 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_LUK_FireRate2_desc |
| CHEWBACCA_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| CHEWBACCA_GAPCLOSER | PRG_CHW_GapCloser | gapCloser | 1 | 3 | 0 | 0 | 0 |  | PRG_MeleeGapCloser_Screen | AV_PRG_MeleeGapCloser | PRG_CHW_GapCloser_desc |
| CHEWBACCA_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Chewbacca |  |
| CHEWBACCA_HEALTH2 | PRG_CHW_Health2 | health | 40 | 4 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_CHW_Health2_desc |
| CHEWBACCA_MELEE_DMG1 | PRG_CHW_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_CHW_MeleeDmg1_desc |
| CHEWBACCA_MELEE_DMG3 | PRG_CHW_MeleeDmg3 | meleeHit,meleeDamage | 1,0.35 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_CHW_MeleeDmg3_desc |
| CHEWBACCA_SPECIAL_DMG3 | PRG_CHW_SuperDmg3 | specialDamage | 0.5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_CHW_SuperDmg3_desc |
| CHEWBACCA_SPECIAL_DMG2 | PRG_CHW_SuperDmg2 | specialDamage | 0.3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_CHW_SuperDmg2_desc |
| CHEWBACCA_SPECIAL_AOE | PRG_CHW_SpecialAOE | specialAoE | 1 | 4 | 0 | 0 | 1 |  | PRG_SuperMoveAOE1_Screen | AV_PRG_SuperMoveAOE1 | PRG_CHW_SpecialAOE_desc |
| CHEWBACCA_PAUSECOMBO2 | PRG_CHW_PauseCombo2 | PauseComboYY_YY | 1 | 5 | 0 | 0 | 0 |  | PRG_CHW_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_CHW_PauseCombo2_desc |
