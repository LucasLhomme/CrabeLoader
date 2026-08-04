# PSX_Emmitt

`ProgressionTree = "IN3_PSX_Emmitt"` -- source: `gamedb/core/in3_psx_emmitt.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| EMMITT_BASEHEALTH | health | 150 |
| EMMITT_BASESUPERJUMP | BM_SuperJumper | 0 |
| EMMITT_BASERANGED_DMG | rangedDamage | 1 |
| EMMITT_BASESPECIAL_DMG | specialDamage | 1 |
| EMMITT_BASERUNSPEED | runspeed | 6.5 |
| EMMITT_RAPIDFIRE_TURRET | specialFireRate,jumpThresholdLeft,jumpThresholdRight,maxJumpThreshold,shakeDuration,shakeAmplitude,shakeSpeed,burstSize,burstPause,rotationTime,firePauseTime | 10.0,25.0,25.0,60.0,0.05,0.07,2.0,6,0.3,0.4,0.3 |
| EMMITT_BASEPARRY | MeleeParry | 0 |
| EMMITT_BASECLIP | clipSize | 12 |
| EMMITT_BASEMELEE_DMG | meleeDamage | 1 |
| EMMITT_BASEFIRERATE | fireRate | 6 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| EMMITT_POWERBAR2 | PRG_PowerBar2 | powerBar | 2 | 4 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar2_desc |
| EMMITT_FIRERATE1 | PRG_EMT_FireRate1 | fireRate | 10.5 | 2 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_EMT_FireRate1_desc |
| EMMITT_HEALTH3 | PRG_EMT_Health3 | health | 60 | 4 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_EMT_Health3_desc |
| EMMITT_FIRERATE2 | PRG_EMT_FireRate2 | fireRate | 15 | 5 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_EMT_FireRate2_desc |
| EMMITT_PARRY | PRG_EMT_Parry | MeleeParry | 1 | 3 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_EMT_Parry_desc |
| EMMITT_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| EMMITT_POWERBAR1 | PRG_PowerBar1 | powerBar | 2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar1_desc |
| EMMITT_HEALTH2 | PRG_EMT_Health2 | health | 60 | 2 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_EMT_Health2_desc |
| EMMITT_SPECIAL_DMG2 | PRG_EMT_SuperDmg2 | specialDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_EMT_SuperDmg2_desc |
| EMMITT_CLIP2 | PRG_EMT_ClipSize2 | clipSize | 24 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_EMT_ClipSize2_desc |
| EMMITT_FINISHER_STUN3 | PRG_EMT_FinisherStun3 | powerStunNPC,powerStunIGP,finisherCooldown | 2.0,1.0,15.0 | 5 | 0 | 0 | 1 | 1 | PRG_FinisherDuration_A_Screen | AV_PRG_FinisherDuration_A | PRG_EMT_FinisherStun3_desc |
| EMMITT_CLIP1 | PRG_EMT_ClipSize1 | clipSize | 16 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_EMT_ClipSize1_desc |
| EMMITT_POWERFILL1 | PRG_PowerFill1 | powerFill | 30 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| EMMITT_SPECIAL_DMG1 | PRG_EMT_SuperDmg1 | specialDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_EMT_SuperDmg1_desc |
| EMMITT_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| EMMITT_MELEE_DMG2 | PRG_EMT_MeleeDmg2 | meleeHit,meleeDamage | 1,0.2 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_EMT_MeleeDmg2_desc |
| EMMITT_COMBO_FINISHER | PRG_EMT_ComboFinisher | finisherCombo,finisherCooldown,powerStunNPC,powerStunIGP | 1.0,20.0,10.0,5.0 | 2 | 1 | 0 | 1 |  | PSX_PRG_EMT_ComboFinisher_Screen | AV_PRG_ComboFinisherUnlock | PRG_EMT_ComboFinisher_desc |
| EMMITT_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Special |  |
| EMMITT_RANGED_DMG2 | PRG_EMT_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.075,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_EMT_RangedDmg2_desc |
| EMMITT_RANGED_DMG1 | PRG_EMT_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.05,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_EMT_RangedDmg1_desc |
| EMMITT_SPECIAL_DMG3 | PRG_EMT_SuperDmg3 | specialDamage | 0.25 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_EMT_SuperDmg3_desc |
| EMMITT_HEALTH1 | PRG_EMT_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_EMT_Health1_desc |
| EMMITT_ATHLETIC_JUMP | PRG_EMT_AthleticJump | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 2 | 1 | 0 | 0 |  | PSX_PRG_EMT_AthleticJump_Screen | AV_PRG_SuperJumpUnlock | PRG_EMT_AthleticJump_desc |
| EMMITT_SPECIAL_UNLOCKED | PRG_EMT_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,3.0,60.0 | 1 | 1 | 0 | 1 |  | PSX_PRG_EMT_SpecialUnlocked | AV_PRG_SuperMoveUnlock | PRG_EMT_SpecialUnlocked_desc |
| EMMITT_CONFIDENCE_BOOST | PRG_EMT_ConfidenceBoost | confidenceBoost,confidenceBoostFill | 1.0,10.0 | 3 | 0 | 0 | 0 |  | PRG_ConfidenceBoost_Screen | AV_PRG_ConfidenceBoost | PRG_EMT_ConfidenceBoost_desc |
| EMMITT_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Emmitt |  |
| EMMITT_FINISHER_STUN2 | PRG_EMT_FinisherStun2 | powerStunNPC,powerStunIGP | 2.0,1.0 | 4 | 0 | 0 | 1 | 1 | PRG_FinisherDuration_A_Screen | AV_PRG_FinisherDuration_A | PRG_EMT_FinisherStun2_desc |
| EMMITT_MELEE_DMG3 | PRG_EMT_MeleeDmg3 | meleeHit,meleeDamage | 1,0.35 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_EMT_MeleeDmg3_desc |
| EMMITT_RANGED_DMG3 | PRG_EMT_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.125,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_EMT_RangedDmg3_desc |
| EMMITT_PARRY_COUNTER | PRG_EMT_ParryCounter | repelAttack | 1 | 5 | 0 | 0 | 0 |  | PSX_PRG_EMT_ParryCounter_Screen | AV_PRG_ParryCounter | PRG_EMT_ParryCounter_desc |
| EMMITT_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| EMMITT_MELEE_DMG1 | PRG_EMT_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_EMT_MeleeDmg1_desc |
| EMMITT_HEALTH4 | PRG_EMT_Health4 | health | 60 | 6 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_EMT_Health4_desc |
| EMMITT_PAUSECOMBO | PRG_EMT_PauseCombo1 | PauseComboY_YY | 1 | 2 | 0 | 0 | 0 |  | PSX_PRG_EMT_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_EMT_PauseCombo1_desc |
| EMMITT_FINISHER_STUN1 | PRG_EMT_FinisherStun1 | powerStunNPC,powerStunIGP | 2.0,1.0 | 2 | 0 | 0 | 1 | 1 | PRG_FinisherDuration_A_Screen | AV_PRG_FinisherDuration_A | PRG_EMT_FinisherStun1_desc |
| EMMITT_AERIALPAUSECOMBO | PRG_EMT_AerialPauseCombo | AerialPauseComboY_YY | 1 | 4 | 1 | 0 | 0 |  | PSX_PRG_EMT_AerialPauseCombo_Screen | AV_PRG_PauseComboAerial | PRG_EMT_AerialPauseCombo_desc |
| EMMITT_POWERBAR3 | PRG_PowerBar3 | powerBar | 2 | 6 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar3_desc |
