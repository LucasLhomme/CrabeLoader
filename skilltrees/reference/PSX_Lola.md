# PSX_Lola

`ProgressionTree = "IN3_PSX_Lola"` -- source: `gamedb/core/in3_psx_lola.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| LOLA_BASERUNSPEED | runspeed | 6.5 |
| LOLA_BASEPARRY | MeleeParry | 0 |
| LOLA_BASERANGED_DMG | rangedDamage | 1 |
| LOLA_BASEHEALTH | health | 150 |
| LOLA_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.6 |
| LOLA_BASESUPERJUMP | BM_SuperJumper | 0 |
| LOLA_BASECLIP | clipSize | 8 |
| LOLA_LEDGE_OBB | ledgeMinY,ledgeMaxY,ledgeJumpMinY,ledgeJumpMaxY | 1.0,2.4,1.5,2.3 |
| LOLA_BASEFIRERATE | fireRate | 3 |
| LOLA_BASESPECIAL_DMG | specialDamage | 1 |
| LOLA_BASEMELEE_DMG | meleeDamage | 1 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| LOLA_SPECIAL_DMG1 | PRG_LLA_SuperDmg1 | specialDamage | 0.2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_LLA_SuperDmg1_desc |
| LOLA_CLIP1 | PRG_LLA_ClipSize1 | clipSize | 10 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_LLA_ClipSize1_desc |
| LOLA_ATHLETIC_JUMP | PRG_LLA_AthleticJump | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 2 | 0 | 0 | 0 |  | PSX_PRG_LLA_AthleticJump_Screen | AV_PRG_SuperJumpUnlock | PRG_LLA_AthleticJump_desc |
| LOLA_POWERBAR2 | PRG_PowerBar2 | powerBar | 2 | 5 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar2_desc |
| LOLA_PAUSECOMBO2 | PRG_LLA_PauseCombo2 | PauseComboYY_YY | 1 | 6 | 0 | 0 | 0 |  | PRG_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_LLA_PauseCombo2_desc |
| LOLA_POWERBAR1 | PRG_PowerBar1 | powerBar | 2 | 3 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar1_desc |
| LOLA_FIRERATE1 | PRG_LLA_FireRate1 | fireRate | 3.5 | 3 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_LLA_FireRate1_desc |
| LOLA_SPECIAL_COMBO | PRG_LLA_SpecialCombo | specialCombo | 1 | 4 | 1 | 0 | 0 |  | PSX_PRG_LLA_SpecialCombo_Screen | AV_PRG_SuperCombo | PRG_LLA_SpecialCombo_desc |
| LOLA_JUGGLE_COMBO | PRG_LLA_JuggleCombo | superStraightPunch | 1 | 5 | 0 | 0 | 0 |  | PSX_PRG_LLA_JuggleCombo_Screen | AV_PRG_JuggleCombo | PRG_LLA_JuggleCombo_desc |
| LOLA_PAUSECOMBO1 | PRG_LLA_PauseCombo1 | PauseComboY_YY | 1 | 3 | 0 | 0 | 0 |  | PRG_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_LLA_PauseCombo1_desc |
| LOLA_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Lola |  |
| LOLA_MORALE_BOOST | PRG_LLA_MoraleBoost | moraleBoost | 1 | 5 | 0 | 0 | 0 |  | PRG_MoraleBoost_Screen | AV_PRG_MoraleBoost | PRG_LLA_MoraleBoost_desc |
| LOLA_SPECIAL_UNLOCKED | PRG_LLA_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,2.0,120.0,8.0 | 1 | 1 | 0 | 0 |  | PSX_PRG_LLA_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_LLA_SpecialUnlocked_desc |
| LOLA_MELEE_DMG2 | PRG_LLA_MeleeDmg2 | meleeHit,meleeDamage | 1,0.2 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LLA_MeleeDmg2_desc |
| LOLA_PARRY | PRG_LLA_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_LLA_Parry_desc |
| LOLA_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| LOLA_SURVIVAL_MODE | PRG_LLA_SurvivalMode | survivalMode,survivalModeTime,survivalModeStartHealthPercent,survivalDamage | 1.0,12.00,0.15,1.25 | 6 | 0 | 0 | 0 |  | PRG_SurvivalMode_Screen | AV_PRG_SurvivalMode | PRG_LLA_SurvivalMode_desc |
| LOLA_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Special |  |
| LOLA_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| LOLA_FIRERATE2 | PRG_LLA_FireRate2 | fireRate | 4 | 5 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_LLA_FireRate2_desc |
| LOLA_HEALTH1 | PRG_LLA_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_LLA_Health1_desc |
| LOLA_MELEE_DMG3 | PRG_LLA_MeleeDmg3 | meleeHit,meleeDamage | 1,0.35 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LLA_MeleeDmg3_desc |
| LOLA_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| LOLA_RANGED_DMG2 | PRG_LLA_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.075,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LLA_RangedDmg2_desc |
| LOLA_CLIP2 | PRG_LLA_ClipSize2 | clipSize | 14 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_LLA_ClipSize2_desc |
| LOLA_RANGED_DMG3 | PRG_LLA_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.125,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LLA_RangedDmg3_desc |
| LOLA_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| LOLA_SPECIAL_DMG2 | PRG_LLA_SuperDmg2 | specialDamage | 0.3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_LLA_SuperDmg2_desc |
| LOLA_RANGED_DMG1 | PRG_LLA_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.05,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LLA_RangedDmg1_desc |
| LOLA_PARRY_COUNTER | PRG_LLA_ParryCounter | repelAttack | 1 | 5 | 1 | 0 | 0 |  | PSX_PRG_LLA_ParryCounter_Screen | AV_PRG_ParryCounter | PRG_LLA_ParryCounter_desc |
| LOLA_COMBO_FINISHER | PRG_LLA_ComboFinisher | finisherCombo,finisherCooldown | 1.0,20.0 | 3 | 1 | 0 | 1 |  | PSX_PRG_LLA_ComboFinisher_Screen | AV_PRG_ComboFinisherUnlock | PRG_LLA_ComboFinisher_desc |
| LOLA_HEALTH3 | PRG_LLA_Health2 | health | 60 | 6 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_LLA_Health2_desc |
| LOLA_SPECIAL_DMG3 | PRG_LLA_SuperDmg3 | specialDamage | 0.5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_LLA_SuperDmg3_desc |
| LOLA_MELEE_DMG1 | PRG_LLA_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LLA_MeleeDmg1_desc |
| LOLA_HEALTH2 | PRG_LLA_Health2 | health | 60 | 4 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_LLA_Health2_desc |
