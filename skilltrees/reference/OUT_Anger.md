# OUT_Anger

`ProgressionTree = "IN3_OUT_Anger"` -- source: `gamedb/core/in3_out_anger.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| ANGER_BASE_HEALTH | health | 150 |
| ANGER_BLOCKBREAKER | blockBreakerCombo | 1 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ANGER_RANGED_CHARGE2 | PRG_AGR_RangedCharge2 | throwChargeTime | 0.75 | 5 | 0 | 0 | 0 |  | PRG_RangedCharge2_Screen | AV_PRG_RangedCharge2 | PRG_AGR_RangedCharge2_desc |
| ANGER_RANGED_AOE2 | PRG_AGR_RangedAOE2 | throwAOE | 3.5 | 5 | 0 | 0 | 0 |  | PRG_RangedAOEBlast2_Screen | AV_PRG_RangedAOEBlast2 | PRG_AGR_RangedAOE2_desc |
| ANGER_SPECIAL_DMG2 | PRG_AGR_SuperDmg2 | specialDamage | 0.4 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_AGR_SuperDmg2_desc |
| ANGER_RANGED_DMG2 | PRG_AGR_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_AGR_RangedDmg2_desc |
| ANGER_RANGED_AOE1 | PRG_AGR_RangedAOE1 | throwAOE | 2 | 3 | 0 | 0 | 0 |  | PRG_RangedAOEBlast_Screen | AV_PRG_RangedAOEBlast | PRG_AGR_RangedAOE1_desc |
| ANGER_SPECIAL_UNLOCKED | PRG_AGR_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerAOE,specialAreaUpgrade | 1.0,1.0,75.0,0.0,0.0 | 1 | 1 | 0 | 1 |  | PRG_AGR_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_AGR_SpecialUnlocked_desc |
| ANGER_RANGED_UTILITY | PRG_AGR_RangedUpgrade1 | throwUpgrade | 1 | 5 | 0 | 0 | 0 |  | PRG_RangedUtility1_Screen | AV_PRG_RangedUtility1 | PRG_AGR_RangedUpgrade1_desc |
| ANGER_RANGED_DMG3 | PRG_AGR_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.25,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_AGR_RangedDmg3_desc |
| ANGER_MELEE_DMG2 | PRG_AGR_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_AGR_MeleeDmg2_desc |
| ANGER_SPECIAL_AOE1 | PRG_AGR_SpecialAOE1 | specialAreaUpgrade | 1 | 3 | 0 | 0 | 0 |  | PRG_SuperMoveAOE1_Screen | AV_PRG_SuperMoveAOE1 | PRG_AGR_SpecialAOE1_desc |
| ANGER_RANGED_DMG1 | PRG_AGR_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_AGR_RangedDmg1_desc |
| ANGER_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Anger |  |
| ANGER_GRID1 | PRG_OUT_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| ANGER_GRID2 | PRG_OUT_EmotionGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Emotion |  |
| ANGER_RANGED_CHARGE1 | PRG_AGR_RangedCharge1 | throwChargeTime | 1.5 | 3 | 0 | 0 | 0 |  | PRG_RangedCharge1_Screen | AV_PRG_RangedCharge1 | PRG_AGR_RangedCharge1_desc |
| ANGER_RANGED_CHARGE_UNLOCK | PRG_AGR_RangedCharge_Unlock | chargedThrow,throwChargeTime | 1,2.0 | 2 | 1 | 0 | 0 |  | PRG_AGR_RangedCharge_Screen | AV_PRG_RangedChargeUp | PRG_AGR_RangedCharge_Unlock_desc |
| ANGER_POWERFILL1 | PRG_PowerFill1 | powerFill | 45 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| ANGER_HEALTH1 | PRG_AGR_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_AGR_Health1_desc |
| ANGER_GROUNDPOUND2 | PRG_AGR_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 1 | 0 | 0 |  | PRG_AGR_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_AGR_GroundPound2_desc |
| ANGER_HEALTH2 | PRG_AGR_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_AGR_Health2_desc |
| ANGER_GRID4 | PRG_OUT_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| ANGER_HEALTH3 | PRG_AGR_Health3 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_AGR_Health3_desc |
| ANGER_SEND_ATTACK | PRG_AGR_SendAttack | superStraightPunch | 1 | 3 | 1 | 0 | 0 |  | PRG_AGR_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_AGR_SendAttack_desc |
| ANGER_PARRY | PRG_AGR_Parry | repel | 1 | 2 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_AGR_Parry_desc |
| ANGER_MELEE_DMG3 | PRG_AGR_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_AGR_MeleeDmg3_desc |
| ANGER_SPECIAL_AOE2 | PRG_AGR_SpecialAOE2 | specialAreaUpgrade | 2 | 5 | 0 | 0 | 0 |  | PRG_SuperMoveAOE2_Screen | AV_PRG_SuperMoveAOE2 | PRG_AGR_SpecialAOE2_desc |
| ANGER_MELEE_DMG1 | PRG_AGR_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_AGR_MeleeDmg1_desc |
| ANGER_SPECIAL_DMG3 | PRG_AGR_SuperDmg3 | specialDamage | 0.7 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_AGR_SuperDmg3_desc |
| ANGER_GRID3 | PRG_OUT_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| ANGER_SPECIAL_DMG1 | PRG_AGR_SuperDmg1 | specialDamage | 0.4 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_AGR_SuperDmg1_desc |
| ANGER_POWERBAR1 | PRG_PowerBar1 | powerBar | 2 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar1_desc |
