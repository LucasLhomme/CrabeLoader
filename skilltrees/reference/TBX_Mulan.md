# TBX_Mulan

`ProgressionTree = "IN3_TBX_Mulan"` -- source: `gamedb/core/in3_tbx_mulan.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| MULAN_BASEHEALTH | health | 150 |
| MULAN_BASEPARRY | MeleeParry | 0 |
| MULAN_BASERUNSPEED | runspeed | 6.5 |
| MULAN_LEDGE_OBB | ledgeMinY,ledgeMaxY,ledgeJumpMinY,ledgeJumpMaxY | 1.0,2.4,1.5,2.3 |
| MULAN_BASESUPERJUMP | BM_SuperJumper | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| MULAN_JUGGLE_COMBO | PRG_MUL_JuggleCombo | superStraightPunch | 1 | 2 | 0 | 0 | 0 |  | PRG_JuggleCombo_Screen | AV_PRG_JuggleCombo | PRG_MUL_JuggleCombo_desc |
| MULAN_SPECIAL_DMG2 | PRG_MUL_SuperDmg2 | specialDamage | 0.4 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_MUL_SuperDmg2_desc |
| MULAN_RANGED_DMG3 | PRG_MUL_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.4,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MUL_RangedDmg3_desc |
| MULAN_SPECIAL_AOE2 | PRG_MUL_SpecialAOE2 | specialAOE | 2 | 5 | 0 | 0 | 0 |  | PRG_SuperMoveAOE2_Screen | AV_PRG_SuperMoveAOE2 | PRG_MUL_SpecialAOE2_desc |
| MULAN_ATHLETIC_JUMP | PRG_MUL_AthleticJump | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 2 | 1 | 0 | 0 |  | PRG_MUL_AthleticJump_Screen | AV_PRG_SuperJumpUnlock | PRG_MUL_AthleticJump_desc |
| MULAN_HEALTH1 | PRG_MUL_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_MUL_Health1_desc |
| MULAN_RANGED_DMG1 | PRG_MUL_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.15,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MUL_RangedDmg1_desc |
| MULAN_POWERFILL1 | PRG_PowerFill1 | powerFill | 95 | 5 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| MULAN_SPECIAL_DMG3 | PRG_MUL_SuperDmg3 | specialDamage | 0.7 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_MUL_SuperDmg3_desc |
| MULAN_SPECIAL_DMG1 | PRG_MUL_SuperDmg1 | specialDamage | 0.4 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_MUL_SuperDmg1_desc |
| MULAN_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Special |  |
| MULAN_POWERBAR1 | PRG_PowerBar1 | powerBar | 2 | 3 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar1_desc |
| MULAN_HEALTH2 | PRG_MUL_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_MUL_Health2_desc |
| MULAN_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| MULAN_PAUSECOMBO2 | PRG_MUL_PauseCombo2 | PauseComboYY_YY | 1 | 6 | 1 | 0 | 0 |  | PRG_MUL_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_MUL_PauseCombo2_desc |
| MULAN_RANGED_FLURRY1 | PRG_MUL_RangedFlurry1 | DashAttack | 1 | 1 | 0 | 0 | 0 |  | PRG_DashAttack_Screen | AV_PRG_DashAttack | PRG_MUL_RangedFlurry1_desc |
| MULAN_PAUSECOMBO1 | PRG_MUL_PauseCombo1 | PauseComboY_YY | 1 | 4 | 1 | 0 | 0 |  | PRG_MUL_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_MUL_PauseCombo1_desc |
| MULAN_MELEE_DMG1 | PRG_MUL_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MUL_MeleeDmg1_desc |
| MULAN_MELEE_DMG2 | PRG_MUL_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MUL_MeleeDmg2_desc |
| MULAN_MELEE_DMG3 | PRG_MUL_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_MUL_MeleeDmg3_desc |
| MULAN_PARRY_COUNTER | PRG_MUL_ParryCounter | repelAttack | 1 | 5 | 0 | 0 | 0 |  | PRG_ParryCounter_Screen | AV_PRG_ParryCounter | PRG_MUL_ParryCounter_desc |
| MULAN_RANGED_FLURRY2 | PRG_MUL_RangedFlurry2 | DashAttack | 2 | 5 | 0 | 0 | 0 |  | PRG_DashAttackFlurry | AV_PRG_DashAttackFlurry | PRG_MUL_RangedFlurry2_desc |
| MULAN_RANGED_DMG2 | PRG_MUL_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.25,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MUL_RangedDmg2_desc |
| MULAN_SPECIAL_AOE1 | PRG_MUL_SpecialAOE1 | specialAOE | 1 | 2 | 0 | 0 | 0 |  | PRG_SuperMoveAOE1_Screen | AV_PRG_SuperMoveAOE1 | PRG_MUL_SpecialAOE1_desc |
| MULAN_POWERBAR2 | PRG_PowerBar2 | powerBar | 2 | 5 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar2_desc |
| MULAN_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Mulan |  |
| MULAN_POWERBAR3 | PRG_PowerBar3 | powerBar | 2 | 6 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar3_desc |
| MULAN_HEALTH3 | PRG_MUL_Health3 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_MUL_Health3_desc |
| MULAN_SPECIAL_UNLOCKED | PRG_MUL_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,3.0,120.0 | 1 | 1 | 0 | 0 |  | PRG_MUL_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_MUL_SpecialUnlocked_desc |
| MULAN_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| MULAN_PARRY | PRG_MUL_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_MUL_Parry_desc |
| MULAN_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
