# TBX_Minnie

`ProgressionTree = "IN3_TBX_Minnie"` -- source: `gamedb/core/in3_tbx_minnie.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| MINNIE_BASE_HEALTH | health | 150 |
| MINNIE_BASERICOCHET | rangedRicochet,ricochetCount | 0,0.0 |
| MINNIE_BASESUPERJUMP | BM_SuperJumper | 0 |
| MINNIE_BASESPECIAL | specialUpgrade | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| MINNIE_SPECIAL_DURATION2 | PRG_MIN_SpecialDuration2 | powerDurationIGP,powerDurationNPC | 6.0,20.0 | 5 | 0 | 0 | 0 |  | PRG_SuperMoveDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_MIN_SpecialDuration2_desc |
| MINNIE_RANGED_DMG2 | PRG_MIN_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.3,0.15,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MIN_RangedDmg2_desc |
| MINNIE_SPECIAL_UNLOCKED | PRG_MIN_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDurationIGP,powerDurationNPC | 1.0,1.0,120.0,4.0,10.0 | 1 | 1 | 0 | 1 |  | PRG_MIN_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_MIN_SpecialUnlocked_desc |
| MINNIE_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Jasmine |  |
| MINNIE_RANGED_DMG3 | PRG_MIN_RangedDmg3 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.5,0.25,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MIN_RangedDmg3_desc |
| MINNIE_GROUNDPOUND2 | PRG_MIN_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 1 | 1 | 0 | 0 |  | PRG_MIN_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_MIN_GroundPound2_desc |
| MINNIE_RANGED_DMG1 | PRG_MIN_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.2,0.1,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MIN_RangedDmg1_desc |
| MINNIE_PURSERICOCHET1 | PRG_MIN_RangedRicochet1 | rangedRicochet,ricochetCount | 1,2.0 | 3 | 0 | 0 | 1 |  | PRG_RangedUpgrade1_Screen | AV_PRG_RangedUpgrade1 | PRG_MIN_RangedRicochet1_desc |
| MINNIE_PURSERICOCHET2 | PRG_MIN_RangedRicochet2 | ricochetCount | 4 | 6 | 0 | 0 | 1 |  | PRG_RangedUpgrade2_Screen | AV_PRG_RangedUpgrade2 | PRG_MIN_RangedRicochet2_desc |
| MINNIE_ATHLETIC_JUMP | PRG_MIN_AthleticJump | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,44,10,12 | 2 | 0 | 0 | 0 |  | PRG_MIN_AthleticJump_Screen | AV_PRG_SuperJumpUnlock | PRG_MIN_AthleticJump_desc |
| MINNIE_CHARGESPEED2 | PRG_MIN_ChargeTime2 | rangedChargeTime | 1 | 4 | 0 | 0 | 0 |  | PRG_RangedCharge2_Screen | AV_PRG_RangedCharge2 | PRG_MIN_ChargeTime2_desc |
| MINNIE_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| MINNIE_CHARGESPEED1 | PRG_MIN_ChargeTime1 | rangedChargeTime | 1.5 | 3 | 0 | 0 | 0 |  | PRG_RangedCharge1_Screen | AV_PRG_RangedCharge1 | PRG_MIN_ChargeTime1_desc |
| MINNIE_CHARGED_UNLOCK | PRG_MIN_ChargeUnlocked | allowRangedCharge,rangedChargeTime | 1,2.0 | 2 | 1 | 0 | 1 |  | PRG_MIN_ChargeUnlocked_Screen | AV_PRG_RangedChargeUp | PRG_MIN_ChargeUnlocked_desc |
| MINNIE_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| MINNIE_MELEE_DMG2 | PRG_MIN_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MIN_MeleeDmg2_desc |
| MINNIE_MELEE_DMG1 | PRG_MIN_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MIN_MeleeDmg1_desc |
| MINNIE_SEND_ATTACK | PRG_MIN_SuperStraightPunch | superStraightPunch | 1 | 3 | 1 | 0 | 0 |  | PRG_MIN_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_MIN_SuperStraightPunch_desc |
| MINNIE_SPECIAL_AOE2 | PRG_MIN_SpecialAOE2 | specialAOE | 3 | 6 | 0 | 0 | 0 |  | PRG_SuperMoveAOE2_Screen | AV_PRG_SuperMoveAOE2 | PRG_MIN_SpecialAOE2_desc |
| MINNIE_IMP_SPECIAL | PRG_MIN_ImpSpecial | specialUpgrade,specialAOE | 1.0,1.0 | 2 | 0 | 1 | 1 |  | PRG_SuperMeterChargeUpgrade1_Screen | AV_PRG_SuperMeterChargeUpgrade1 | PRG_MIN_ImpSpecial_desc |
| MINNIE_PAUSECOMBO1 | PRG_MIN_PauseCombo1 | PauseComboY_YY | 1 | 4 | 0 | 0 | 0 |  | PRG_MIN_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_MIN_PauseCombo1_desc |
| MINNIE_HEALTH1 | PRG_MIN_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MIN_Health1_desc |
| MINNIE_MELEE_DMG3 | PRG_MIN_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MIN_MeleeDmg3_desc |
| MINNIE_HEALTH3 | PRG_MIN_Health3 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MIN_Health3_desc |
| MINNIE_POWERFILL1 | PRG_PowerFill1 | powerFill | 75 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| MINNIE_SPECIAL_AOE1 | PRG_MIN_SpecialAOE1 | specialAOE | 2 | 4 | 0 | 0 | 0 |  | PRG_SuperMoveAOE1_Screen | AV_PRG_SuperMoveAOE1 | PRG_MIN_SpecialAOE1_desc |
| MINNIE_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| MINNIE_SPECIAL_DURATION1 | PRG_MIN_SpecialDuration1 | powerDurationIGP,powerDurationNPC | 5.0,15.0 | 3 | 0 | 0 | 0 |  | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_MIN_SpecialDuration1_desc |
| MINNIE_HEALTH2 | PRG_MIN_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MIN_Health2_desc |
| MINNIE_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| MINNIE_HEALTH4 | PRG_MIN_Health4 | health | 60 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MIN_Health4_desc |
