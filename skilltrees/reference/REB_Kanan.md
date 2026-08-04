# REB_Kanan

`ProgressionTree = "IN3_REB_Kanan"` -- source: `gamedb/core/in3_reb_kanan.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| KANAN_BASE_CLIP | clipSize | 8 |
| KANAN_RANGEDCOMBO_AERIAL_YRT | AerialRangedComboYRT | 1 |
| KANAN_BASE_BLOCK_RICOCHET_LIGHTSABER | lightsaberBlockRicochetChance, lightsaberPerfectBlockRicochetChance | 100,10 |
| KANAN_BASE_FORCEPUSH | forcePush | 1 |
| KANAN_BASE_FINISHER | finisherCooldown | 20 |
| KANAN_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| KANAN_CHARGEATTACK | blockBreakerCombo | 1 |
| KANAN_BASE_FIRERATE | fireRate | 3 |
| KANAN_RANGEDCOMBO_YRT | RangedComboYRT | 1 |
| KANAN_BASE_DEFLECT | DeflectBlasterBoltsFromBlock,DeflectBlasterBoltsFromParry,DeflectBlasterBoltsFromIdle | 1.0,0.0,0.0 |
| KANAN_BASE_HEALTH | health | 150 |
| KANAN_BASE_PARRY | MeleeParry | 0 |
| KANAN_BASE_RICOCHET_LIGHTSABER | lightsaberRicochetChance,lightsaberPerfectRicochetChance | 100,0 |
| KANAN_RANGEDCOMBO_YYRT | RangedComboYYRT | 1 |
| KANAN_LAUNCHER | superStraightPunch | 1 |
| KANAN_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.6 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| KANAN_FORCE_SPECIAL1 | PRG_KNN_ForceSpecial | specialUnlocked,powerBar,powerFill,powerDuration | 1.0,1.0,120.0,5.0 | 1 | 1 | 0 | 1 |  | PRG_KNN_ForceSpecial_Screen | AV_PRG_SuperMoveUnlock | PRG_KNN_ForceSpecial_desc |
| KANAN_FINISHER_DMG2 | PRG_KNN_FinisherDmg2 | finisherDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_KNN_FinisherDmg2_desc |
| KANAN_GRID3 | PRG_LUK_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| KANAN_PAUSECOMBO2 | PRG_KNN_PauseCombo2 | PauseComboYY_YY | 1 | 5 | 1 | 0 | 0 |  | PRG_KNN_PauseCombo2_Screen | AV_PRG_MeleeComboFinisher | PRG_KNN_PauseCombo2_desc |
| KANAN_FORCE_JUMP | PRG_KNN_ForceJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,12,0.4,52,12,14 | 2 | 0 | 0 | 0 |  | PRG_KNN_ForceJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_KNN_ForceJump1_desc |
| KANAN_MELEE_DMG3 | PRG_KNN_MeleeDmg3 | meleeHit,meleeDamage | 1,0.35 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_KNN_MeleeDmg3_desc |
| KANAN_HEALTH3 | PRG_KNN_Health2 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_KNN_Health2_desc |
| KANAN_TIMED_DEFLECT | PRG_KNN_TimedDeflect | DeflectBlasterBoltsFromParry | 1 | 3 | 1 | 0 | 0 |  | PRG_KNN_TimedDeflect_Screen | AV_PRG_DeflectTimed | PRG_KNN_TimedDeflect_desc |
| KANAN_SPECIAL_DMG2 | PRG_KNN_SuperDmg2 | specialDamage | 0.4 | 3 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_KNN_SuperDmg2_desc |
| KANAN_MELEE_DMG1 | PRG_KNN_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_KNN_MeleeDmg1_desc |
| KANAN_SPECIAL_DMG3 | PRG_KNN_SuperDmg3 | specialDamage | 0.7 | 5 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_KNN_SuperDmg3_desc |
| KANAN_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Kanan |  |
| KANAN_GRID2 | PRG_LUK_ForceGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| KANAN_GRID1 | PRG_LUK_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| KANAN_SPECIAL_DURATION1 | PRG_KNN_PowerDuration1 | powerDuration | 8 | 3 | 0 | 0 | 0 |  | PRG_PowerDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_KNN_PowerDuration1_desc |
| KANAN_FIRERATE1 | PRG_KNN_FireRate1 | fireRate | 3.5 | 1 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_KNN_FireRate1_desc |
| KANAN_SPECIAL_DMG1 | PRG_KNN_SuperDmg1 | specialDamage | 0.4 | 1 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_KNN_SuperDmg1_desc |
| KANAN_ATTACK_FINISHER1 | PRG_KNN_AttackFinisher1 | finisherCombo,finisherCooldown | 1.0,25.0 | 2 | 0 | 0 | 1 |  | PRG_SuperPunch_Screen | AV_PRG_ForcePull | PRG_KNN_AttackFinisher1_desc |
| KANAN_CLIP2 | PRG_KNN_BlasterClip2 | clipSize | 14 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_KNN_BlasterClip2_desc |
| KANAN_AUTO_DEFLECT | PRG_KNN_AutoDeflect | DeflectBlasterBoltsFromIdle | 1 | 5 | 0 | 0 | 1 |  | PRG_KNN_AutoDeflect_Screen | AV_PRG_DeflectAuto | PRG_KNN_AutoDeflect_desc |
| KANAN_SPECIAL_DURATION2 | PRG_KNN_PowerDuration2 | powerDuration | 12 | 5 | 0 | 0 | 0 |  | PRG_PowerDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_KNN_PowerDuration2_desc |
| KANAN_RANGED_DMG3 | PRG_KNN_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.125,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_KNN_RangedDmg3_desc |
| KANAN_RANGED_DMG1 | PRG_KNN_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.05,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_KNN_RangedDmg1_desc |
| KANAN_HEALTH2 | PRG_KNN_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_KNN_Health2_desc |
| KANAN_FINISHER_DMG3 | PRG_KNN_FinisherDmg3 | finisherDamage | 0.35 | 6 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_KNN_FinisherDmg3_desc |
| KANAN_GRID4 | PRG_LUK_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| KANAN_PAUSECOMBO1 | PRG_KNN_PauseCombo1 | PauseComboY_YY | 1 | 2 | 0 | 0 | 0 |  | PRG_KNN_PauseCombo1_Screen | AV_PRG_MeleeComboFinisher | PRG_KNN_PauseCombo1_desc |
| KANAN_PARRY | PRG_KNN_Parry | MeleeParry | 1 | 1 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_KNN_Parry_desc |
| KANAN_AERIALPAUSECOMBO | PRG_ANI_AerialPauseCombo | AerialPauseComboY_YY | 1 | 3 | 1 | 0 | 0 |  | PRG_KNN_PauseCombo3_Screen | AV_PRG_MeleeComboFinisher | PRG_KNN_PauseCombo3_desc |
| KANAN_FINISHER_DMG1 | PRG_KNN_FinisherDmg1 | finisherDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_KNN_FinisherDmg1_desc |
| KANAN_RANGED_DMG2 | PRG_KNN_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.075,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_KNN_RangedDmg2_desc |
| KANAN_FIRERATE2 | PRG_KNN_FireRate2 | fireRate | 4 | 4 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_KNN_FireRate2_desc |
| KANAN_ATTACK_FINISHER2 | PRG_KNN_AttackFinisher2 | finisherCooldown | 20 | 4 | 0 | 0 | 0 |  | PRG_SuperPunch_Screen | AV_PRG_ForcePull | PRG_KNN_AttackFinisher2_desc |
| KANAN_POWERFILL1 | PRG_PowerFill1 | powerFill | 95 | 2 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| KANAN_CLIP1 | PRG_KNN_BlasterClip1 | clipSize | 10 | 1 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_KNN_BlasterClip1_desc |
| KANAN_HEALTH1 | PRG_KNN_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_KNN_Health1_desc |
| KANAN_MELEE_DMG2 | PRG_KNN_MeleeDmg2 | meleeHit,meleeDamage | 1,0.2 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_KNN_MeleeDmg2_desc |
