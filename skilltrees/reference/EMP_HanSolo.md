# EMP_HanSolo

`ProgressionTree = "IN3_EMP_HanSolo"` -- source: `gamedb/core/in3_emp_hansolo.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| HANSOLO_BASE_HEALTH | health | 150 |
| HANSOLO_BASE_PARRY | MeleeParry | 0 |
| HANSOLO_LAUNCHER | straightSuperPunch | 1 |
| HANSOLO_RAPIDFIRE_TURRET | specialFireRate,jumpThresholdLeft,jumpThresholdRight,maxJumpThreshold,shakeDuration,shakeAmplitude,shakeSpeed,burstSize,burstPause,rotationTime,firePauseTime | 7.0,25.0,25.0,60.0,0.05,0.1,2.0,4,0.3,0.4,0.3 |
| HANSOLO_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| HANSOLO_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.4 |
| HANSOLO_BASE_CLIP | clipSize | 8 |
| HANSOLO_CHARGEATTACK | blockBreakerCombo | 1 |
| HANSOLO_BASE_FIRERATE | fireRate | 3 |
| HANSOLO_BASE_FINISHER | finisherCombo,finisherCooldown,powerStun | 0.0,10.0,8.0 |
| HANSOLO_BASE_RANGEDCOMBO | RangedComboYRT | 1 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| HANSOLO_HEALTH1 | PRG_HAN_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_HAN_Health1_desc |
| HANSOLO_MELEE_DMG2 | PRG_HAN_MeleeDmg2 | meleeHit,meleeDamage | 1,0.2 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HAN_MeleeDmg2_desc |
| HANSOLO_MELEE_DMG1 | PRG_HAN_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HAN_MeleeDmg1_desc |
| HANSOLO_ATTACK_FINISHER3 | PRG_HAN_AttackFinisher3 | finisherCooldown | 20 | 5 | 0 | 0 | 0 | 1 | PRG_ComboFinisherUpgrade2 | AV_PRG_ComboFinisherUgrade2 | PRG_HAN_AttackFinisher3_desc |
| HANSOLO_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| HANSOLO_SPECIAL_DMG1 | PRG_HAN_SuperDmg1 | specialDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_HAN_SuperDmg1_desc |
| HANSOLO_POWERBAR2 | PRG_PowerBar2 | powerBar | 2 | 4 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar2_desc |
| HANSOLO_POWERBAR1 | PRG_PowerBar1 | powerBar | 2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar1_desc |
| HANSOLO_HEALTH3 | PRG_HAN_Health2 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_HAN_Health2_desc |
| HANSOLO_RANGEDCOMBO2 | PRG_HAN_RangedCombo2 | RangedComboYYYRT | 1 | 6 | 0 | 0 | 0 |  | PRG_HAN_RangedCombo2_Screen | AV_PRG_RangedCombo2 | PRG_HAN_RangedCombo2_desc |
| HANSOLO_POWERFILL1 | PRG_PowerFill1 | powerFill | 30 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| HANSOLO_SPECIAL_UNLOCKED | PRG_HAN_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,3.0,60.0 | 1 | 1 | 0 | 1 |  | PRG_HAN_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_HAN_SpecialUnlocked_desc |
| HANSOLO_ATTACK_FINISHER1 | PRG_HAN_AttackFinisher1 | finisherCombo,finisherCooldown | 1.0,25 | 2 | 1 | 0 | 1 |  | PRG_HAN_AttackFinisher1_Screen | AV_PRG_ComboFinisherUnlock | PRG_HAN_AttackFinisher1_desc |
| HANSOLO_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| HANSOLO_PAUSECOMBO1 | PRG_HAN_PauseCombo1 | PauseComboY_YY | 1 | 4 | 1 | 0 | 0 |  | PRG_HAN_PauseCombo1_Screen | AV_PRG_RangedPauseCombo1 | PRG_HAN_PauseCombo1_desc |
| HANSOLO_SPECIAL_DMG3 | PRG_HAN_SuperDmg3 | specialDamage | 0.25 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_HAN_SuperDmg3_desc |
| HANSOLO_RANGED_DMG2 | PRG_HAN_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.075,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HAN_RangedDmg2_desc |
| HANSOLO_PARRY | PRG_HAN_Parry | MeleeParry | 1 | 3 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_HAN_Parry_desc |
| HANSOLO_RANGEDCOMBO1 | PRG_HAN_RangedCombo1 | RangedComboYYRT | 1 | 2 | 1 | 0 | 0 |  | PRG_HAN_RangedCombo1_Screen | AV_PRG_RangedCombo1 | PRG_HAN_RangedCombo1_desc |
| HANSOLO_RANGED_DMG1 | PRG_HAN_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.05,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HAN_RangedDmg1_desc |
| HANSOLO_ATTACK_FINISHER2 | PRG_HAN_AttackFinisher2 | powerStun | 4 | 4 | 0 | 0 | 0 | 1 | PRG_ComboFinisherUpgrade1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_HAN_AttackFinisher2_desc |
| HANSOLO_POWERBAR3 | PRG_PowerBar3 | powerBar | 2 | 6 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar3_desc |
| HANSOLO_RANGED_DMG3 | PRG_HAN_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.125,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HAN_RangedDmg3_desc |
| HANSOLO_HEALTH2 | PRG_HAN_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_HAN_Health2_desc |
| HANSOLO_FIRERATE2 | PRG_HAN_FireRate2 | fireRate | 4 | 5 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_HAN_FireRate2_desc |
| HANSOLO_SPECIAL_DMG2 | PRG_HAN_SuperDmg2 | specialDamage | 0.15 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_HAN_SuperDmg2_desc |
| HANSOLO_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| HANSOLO_FIRERATE1 | PRG_HAN_FireRate1 | fireRate | 3.5 | 2 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_HAN_FireRate1_desc |
| HANSOLO_ATTACK_FINISHER4 | PRG_HAN_AttackFinisher4 | powerStun | 4 | 6 | 0 | 0 | 0 | 1 | PRG_ComboFinisherUpgrade3 | AV_PRG_ComboFinisherUgrade3 | PRG_HAN_AttackFinisher4_desc |
| HANSOLO_ATHLETICJUMP | PRG_HAN_AthleticJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 2 | 0 | 0 | 0 |  | PRG_HAN_AthleticJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_HAN_AthleticJump1_desc |
| HANSOLO_CLIP1 | PRG_HAN_BlasterClip1 | clipSize | 10 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_HAN_BlasterClip1_desc |
| HANSOLO_CLIP2 | PRG_HAN_BlasterClip2 | clipSize | 14 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_HAN_BlasterClip2_desc |
| HANSOLO_MELEE_DMG3 | PRG_HAN_MeleeDmg3 | meleeHit,meleeDamage | 1,0.35 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HAN_MeleeDmg3_desc |
| HANSOLO_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_HanSolo |  |
| HANSOLO_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
