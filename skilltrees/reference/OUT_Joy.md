# OUT_Joy

`ProgressionTree = "IN3_OUT_Joy"` -- source: `gamedb/core/in3_out_joy.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| JOY_LEDGE_OBB | ledgeMinY,ledgeMaxY,ledgeJumpMinY,ledgeJumpMaxY | 1.0,2.4,1.5,2.3 |
| JOY_BLOCKBREAKER | blockBreakerCombo | 1 |
| JOY_FLOAT_SPEED | floatspeed,floatgravity | 5.0,2.0 |
| JOY_BASE_HEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| JOY_GRID4 | PRG_OUT_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| JOY_SPECIAL_DMG2 | PRG_JOY_SuperDmg2 | specialDamage | 0.4 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_JOY_SuperDmg2_desc |
| JOY_GROUNDPOUND2 | PRG_JOY_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 1 | 0 | 0 |  | PRG_JOY_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_JOY_GroundPound2_desc |
| JOY_SPECIAL_DMG1 | PRG_JOY_SuperDmg1 | specialDamage | 0.4 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_JOY_SuperDmg1_desc |
| JOY_PARRY | PRG_JOY_Parry | repel | 1 | 1 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_JOY_Parry_desc |
| JOY_GRID2 | PRG_OUT_EmotionGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Emotion |  |
| JOY_HEALTHREGEN2 | PRG_JOY_HealthRegen2 | healthRegen | 5 | 6 | 0 | 0 | 0 |  | AV_PRG_RegenUpgrade1_Screen | AV_PRG_RegenUpgrade1 | PRG_JOY_HealthRegen2_desc |
| JOY_SEND_ATTACK | PRG_JOY_SendAttack | superStraightPunch | 1 | 3 | 1 | 0 | 0 |  | PRG_JOY_SendAttack_Screen | AV_PRG_SuperPunchUnlock | PRG_JOY_SendAttack_desc |
| JOY_RANGED_DMG3 | PRG_JOY_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.25,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JOY_RangedDmg3_desc |
| JOY_HEALTH2 | PRG_JOY_Health2 | health | 60 | 4 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_JOY_Health2_desc |
| JOY_SPECIAL_AOE1 | PRG_JOY_SpecialAOE1 | specialAreaUpgrade | 1 | 2 | 0 | 0 | 0 |  | PRG_SuperMoveAOE1_Screen | AV_PRG_SuperMoveAOE1 | PRG_JOY_SpecialAOE1_desc |
| JOY_RANGED_DMG2 | PRG_JOY_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JOY_RangedDmg2_desc |
| JOY_GRID1 | PRG_OUT_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| JOY_GRID3 | PRG_OUT_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| JOY_RANGED_HEAL2 | PRG_JOY_RangedHeal2 | throwHeal | 15 | 5 | 0 | 0 | 0 |  | PRG_RangedHeal2_Screen | AV_PRG_RangedHeal2 | PRG_JOY_RangedHeal2_desc |
| JOY_RANGED_CHARGE1 | PRG_JOY_RangedCharge1 | throwChargeTime | 1.5 | 2 | 0 | 0 | 0 |  | PRG_RangedCharge1_Screen | AV_PRG_RangedCharge1 | PRG_JOY_RangedCharge1_desc |
| JOY_POWERFILL1 | PRG_PowerFill1 | powerFill | 45 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| JOY_RANGED_DMG1 | PRG_JOY_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JOY_RangedDmg1_desc |
| JOY_MELEE_DMG1 | PRG_JOY_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JOY_MeleeDmg1_desc |
| JOY_RANGED_HEAL1 | PRG_JOY_RangedHeal1 | throwHeal | 10 | 2 | 0 | 0 | 0 |  | PRG_RangedHeal1_Screen | AV_PRG_RangedHeal1 | PRG_JOY_RangedHeal1_desc |
| JOY_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Joy |  |
| JOY_RANGED_CHARGE_UNLOCK | PRG_JOY_RangedCharge_Unlock | chargedThrow,throwChargeTime,throwHeal | 1,2.0,5.0 | 1 | 1 | 0 | 0 |  | PRG_JOY_RangedCharge_Screen | AV_PRG_RangedChargeUp | PRG_JOY_RangedCharge_Unlock_desc |
| JOY_HEALTHREGEN1 | PRG_JOY_HealthRegen1 | healthRegen | 2 | 3 | 0 | 0 | 1 |  | AV_PRG_RegenUnlock_Screen | AV_PRG_RegenUnlock | PRG_JOY_HealthRegen1_desc |
| JOY_SPECIAL_AOE2 | PRG_JOY_SpecialAOE2 | specialAreaUpgrade | 2 | 6 | 0 | 0 | 0 |  | PRG_SuperMoveAOE2_Screen | AV_PRG_SuperMoveAOE2 | PRG_JOY_SpecialAOE2_desc |
| JOY_MELEE_DMG3 | PRG_JOY_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JOY_MeleeDmg3_desc |
| JOY_MELEE_DMG2 | PRG_JOY_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JOY_MeleeDmg2_desc |
| JOY_SPECIAL_DMG3 | PRG_JOY_SuperDmg3 | specialDamage | 0.7 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_JOY_SuperDmg3_desc |
| JOY_RANGED_CHARGE2 | PRG_JOY_RangedCharge2 | throwChargeTime | 0.75 | 5 | 0 | 0 | 0 |  | PRG_RangedCharge2_Screen | AV_PRG_RangedCharge2 | PRG_JOY_RangedCharge2_desc |
| JOY_POWERBAR1 | PRG_PowerBar1 | powerBar | 2 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar1_desc |
| JOY_SPECIAL_UNLOCKED | PRG_JOY_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerAOE,specialAreaUpgrade | 1.0,1.0,75.0,0.0,1.0,0.0 | 1 | 1 | 0 | 1 |  | PRG_JOY_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_JOY_SpecialUnlocked_desc |
| JOY_HEALTH1 | PRG_JOY_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_JOY_Health1_desc |
