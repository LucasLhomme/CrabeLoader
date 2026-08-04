# REB_Sabine

`ProgressionTree = "IN3_REB_Sabine"` -- source: `gamedb/core/in3_reb_sabine.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| SABINE_RANGEDCOMBO_YRT | RangedComboYRT | 1 |
| SABINE_CHARGEATTACK | blockBreakerCombo | 1 |
| SABINE_SPECIAL_NOTARGET_SHOOT_DISTANCE | NoTargetShootDistance | 8 |
| SABINE_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| SABINE_BASE_FIRERATE | fireRate | 4 |
| SABINE_BASE_FINISHER | finisherCooldown | 10 |
| SABINE_BASE_HEALTH | health | 150 |
| SABINE_LEDGE_OBB | ledgeMinY,ledgeMaxY,ledgeJumpMinY,ledgeJumpMaxY | 1.0,2.4,1.5,2.3 |
| SABINE_RANGEDCOMBO_AERIAL_YRT | AerialRangedComboYRT | 1 |
| SABINE_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.6 |
| SABINE_RANGEDCOMBO_YYRT | RangedComboYYRT | 1 |
| SABINE_BASE_CLIP | clipSize | 10 |
| SABINE_BASE_PARRY | MeleeParry | 0 |
| SABINE_LAUNCHER | superStraightPunch | 1 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| SABINE_MELEE_DMG2 | PRG_SAB_MeleeDmg2 | meleeHit,meleeDamage | 1,0.2 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SAB_MeleeDmg2_desc |
| SABINE_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| SABINE_POWERFILL1 | PRG_PowerFill1 | powerFill | 95 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| SABINE_SPECIAL_DMG1 | PRG_SAB_SuperDmg1 | specialDamage | 0.25 | 1 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_SAB_SuperDmg1_desc |
| SABINE_PARRY | PRG_SAB_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_SAB_Parry_Screen | AV_PRG_RepelUnlock | PRG_SAB_Parry_desc |
| SABINE_RANGED_DMG2 | PRG_SAB_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.075,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SAB_RangedDmg2_desc |
| SABINE_SPECIAL_UNLOCKED | PRG_SAB_SpecialUnlocked | specialUnlocked,powerBar,powerFill,maxRemoteBombs | 1.0,2.0,120.0,2.0 | 1 | 1 | 0 | 1 |  | PRG_SAB_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_SAB_SpecialUnlocked_desc |
| SABINE_MELEE_DMG3 | PRG_SAB_MeleeDmg3 | meleeHit,meleeDamage | 1,0.35 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SAB_MeleeDmg3_desc |
| SABINE_AERIALPAUSECOMBO | PRG_SAB_AerialPauseCombo | AerialPauseComboY_YY | 1 | 3 | 1 | 0 | 0 |  | PRG_SAB_AerialPauseCombo_Screen | AV_PRG_PauseComboAerial | PRG_SAB_AerialPauseCombo_desc |
| SABINE_ATTACK_FINISHER2 | PRG_SAB_ForceFinisher2 | finisherCooldown | 20 | 4 | 0 | 0 | 0 |  | PRG_ComboFinisherUpgrade1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_SAB_ForceFinisher2_desc |
| SABINE_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| SABINE_POWERBAR1 | PRG_PowerBar1 | powerBar | 1 | 2 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar1_desc |
| SABINE_POWERBAR2 | PRG_PowerBar2 | powerBar | 1 | 3 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar2_desc |
| SABINE_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Sabine |  |
| SABINE_HEALTH3 | PRG_SAB_Health2 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_SAB_Health2_desc |
| SABINE_POWERBAR3 | PRG_PowerBar3 | powerBar | 1 | 5 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar3_desc |
| SABINE_MELEE_DMG1 | PRG_SAB_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SAB_MeleeDmg1_desc |
| SABINE_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| SABINE_SPECIAL_DMG2 | PRG_SAB_SuperDmg2 | specialDamage | 0.4 | 3 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_SAB_SuperDmg2_desc |
| SABINE_CLIP1 | PRG_SAB_BlasterClip1 | clipSize | 14 | 1 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_SAB_BlasterClip1_desc |
| SABINE_FINISHER_DMG3 | PRG_SAB_FinisherDmg3 | finisherDamage | 0.35 | 5 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_SAB_FinisherDmg3_desc |
| SABINE_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| SABINE_PAUSECOMBO1 | PRG_SAB_PauseCombo1 | PauseComboY_YY | 1 | 2 | 0 | 0 | 0 |  | PRG_SAB_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_SAB_PauseCombo1_desc |
| SABINE_RANGED_DMG1 | PRG_SAB_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.05,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SAB_RangedDmg1_desc |
| SABINE_ATTACK_FINISHER1 | PRG_SAB_ForceFinisher1 | finisherCombo,finisherCooldown | 1.0,25.0 | 2 | 1 | 0 | 1 |  | PRG_SAB_ForceFinisher1_Screen | AV_PRG_ComboFinisherUnlock | PRG_SAB_ForceFinisher1_desc |
| SABINE_SPECIAL_LEVEL2 | PRG_SAB_SpecialBombs2 | maxRemoteBombs | 1 | 5 | 0 | 0 | 1 | 1 | PRG_SuperMoveUpgrade2_Screen | AV_PRG_SuperMoveUpgrade2 | PRG_SAB_SpecialBombs2_desc |
| SABINE_RANGED_CRIT1 | PRG_SAB_RangedCrit1 | rangedCrit | 1 | 2 | 0 | 0 | 0 | 1 | PRG_RangedUpgrade1_Screen | AV_PRG_RangedUpgrade1 | PRG_SAB_RangedCrit1_desc |
| SABINE_SPECIAL_DMG3 | PRG_SAB_SuperDmg3 | specialDamage | 0.6 | 5 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_SAB_SuperDmg3_desc |
| SABINE_SPECIAL_LEVEL1 | PRG_SAB_SpecialBombs1 | maxRemoteBombs | 1 | 3 | 0 | 0 | 1 | 1 | PRG_SuperMoveUpgrade1_Screen | AV_PRG_SuperMoveUpgrade1 | PRG_SAB_SpecialBombs1_desc |
| SABINE_RANGED_DMG3 | PRG_SAB_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.125,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SAB_RangedDmg3_desc |
| SABINE_RANGED_CRIT2 | PRG_SAB_RangedCrit2 | rangedCrit | 1 | 4 | 0 | 0 | 0 | 1 | PRG_RangedUpgrade2_Screen | AV_PRG_RangedUpgrade2 | PRG_SAB_RangedCrit2_desc |
| SABINE_HEALTH1 | PRG_SAB_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_SAB_Health1_desc |
| SABINE_ATHLETIC_JUMP | PRG_SAB_AthleticJump | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 2 | 0 | 0 | 0 |  | PRG_SAB_AthleticJump_Screen | AV_PRG_SuperJumpUnlock | PRG_SAB_AthleticJump_desc |
| SABINE_FINISHER_DMG2 | PRG_SAB_FinisherDmg2 | finisherDamage | 0.2 | 3 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_SAB_FinisherDmg2_desc |
| SABINE_FIRERATE2 | PRG_SAB_FireRate2 | fireRate | 5.5 | 4 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_SAB_FireRate2_desc |
| SABINE_PAUSECOMBO2 | PRG_SAB_PauseCombo2 | PauseComboYY_YY | 1 | 5 | 1 | 0 | 0 |  | PRG_SAB_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_SAB_PauseCombo2_desc |
| SABINE_FIRERATE1 | PRG_SAB_FireRate1 | fireRate | 4.75 | 2 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_SAB_FireRate1_desc |
| SABINE_HEALTH2 | PRG_SAB_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_SAB_Health2_desc |
