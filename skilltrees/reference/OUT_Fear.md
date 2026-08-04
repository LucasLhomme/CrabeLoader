# OUT_Fear

`ProgressionTree = "IN3_OUT_Fear"` -- source: `gamedb/core/in3_out_fear.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| FEAR_BLOCKBREAKER | blockBreakerCombo | 1 |
| FEAR_BASE_HEALTH | health | 150 |
| FEAR_BASERUNSPEED | runspeed | 10 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| FEAR_SPECIAL_DMG3 | PRG_FER_SuperDmg3 | specialDamage | 0.7 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_FER_SuperDmg3_desc |
| FEAR_HEALTH2 | PRG_FER_Health2 | health | 60 | 4 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_FER_Health2_desc |
| FEAR_RANGED_CHARGE1 | PRG_FER_RangedCharge1 | throwChargeTime | 1.5 | 3 | 0 | 0 | 0 |  | PRG_RangedCharge1_Screen | AV_PRG_RangedCharge1 | PRG_FER_RangedCharge1_desc |
| FEAR_SPECIAL_AOE1 | PRG_FER_SpecialAOE1 | specialAreaUpgrade | 1 | 3 | 0 | 0 | 0 |  | PRG_SuperMoveAOE1_Screen | AV_PRG_SuperMoveAOE1 | PRG_FER_SpecialAOE1_desc |
| FEAR_RANGED_DMG3 | PRG_FER_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.25,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_FER_RangedDmg3_desc |
| FEAR_RANGED_DMG2 | PRG_FER_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_FER_RangedDmg2_desc |
| FEAR_RANGED_DMG1 | PRG_FER_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_FER_RangedDmg1_desc |
| FEAR_POWERFILL1 | PRG_PowerFill1 | powerFill | 45 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| FEAR_SPECIAL_UNLOCKED | PRG_FER_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerAOE,specialAreaUpgrade | 1.0,1.0,75.0,0.0,0.0 | 1 | 1 | 0 | 1 |  | PRG_FER_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_FER_SpecialUnlocked_desc |
| FEAR_GRID1 | PRG_OUT_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| FEAR_RANGED_CHARGE2 | PRG_FER_RangedCharge2 | throwChargeTime | 0.75 | 6 | 0 | 0 | 0 |  | PRG_RangedCharge2_Screen | AV_PRG_RangedCharge2 | PRG_FER_RangedCharge2_desc |
| FEAR_RANGED_UTILITY | PRG_FER_RangedUpgrade1 | throwUpgrade | 1 | 3 | 0 | 0 | 0 |  | PRG_RangedUtility1_Screen | AV_PRG_RangedUtility1 | PRG_FER_RangedUpgrade1_desc |
| FEAR_GROUNDPOUND2 | PRG_FER_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 1 | 0 | 0 |  | PRG_FER_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_FER_GroundPound2_desc |
| FEAR_SEND_ATTACK | PRG_FER_SendAttack | superStraightPunch | 1 | 4 | 1 | 0 | 0 |  | PRG_FER_SendAttack_Screen | AV_PRG_SuperPunchUnlock | PRG_FER_SendAttack_desc |
| FEAR_MELEE_DMG3 | PRG_FER_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_FER_MeleeDmg3_desc |
| FEAR_PARRY | PRG_FER_Parry | repel | 1 | 1 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_FER_Parry_desc |
| FEAR_RANGED_CHARGE_UNLOCK | PRG_FER_RangedCharge_Unlock | chargedThrow,throwChargeTime,throwHeal | 1,2.0,5.0 | 1 | 1 | 0 | 0 |  | PRG_FER_RangedCharge_Screen | AV_PRG_RangedChargeUp | PRG_FER_RangedCharge_Unlock_desc |
| FEAR_HEALTH3 | PRG_FER_Health3 | health | 60 | 6 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_FER_Health3_desc |
| FEAR_HEALTH1 | PRG_FER_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_FER_Health1_desc |
| FEAR_MELEE_DMG1 | PRG_FER_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_FER_MeleeDmg1_desc |
| FEAR_GRID3 | PRG_OUT_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| FEAR_POWERBAR1 | PRG_PowerBar1 | powerBar | 2 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar1_desc |
| FEAR_GRID2 | PRG_OUT_EmotionGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Emotion |  |
| FEAR_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Fear |  |
| FEAR_MELEE_DMG2 | PRG_FER_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_FER_MeleeDmg2_desc |
| FEAR_SPECIAL_AOE2 | PRG_FER_SpecialAOE2 | specialAreaUpgrade | 2 | 6 | 0 | 0 | 0 |  | PRG_SuperMoveAOE2_Screen | AV_PRG_SuperMoveAOE2 | PRG_FER_SpecialAOE2_desc |
| FEAR_GRID4 | PRG_OUT_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| FEAR_SPECIAL_DMG1 | PRG_FER_SuperDmg1 | specialDamage | 0.4 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_FER_SuperDmg1_desc |
| FEAR_SPECIAL_DMG2 | PRG_FER_SuperDmg2 | specialDamage | 0.4 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_FER_SuperDmg2_desc |
