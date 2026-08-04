# REB_Ezra

`ProgressionTree = "IN3_REB_Ezra"` -- source: `gamedb/core/in3_reb_ezra.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| EZRA_BASE_FIRERATE | fireRate | 3 |
| EZRA_BASE_HEALTH | health | 150 |
| EZRA_BASE_FORCEPUSH | forcePush | 0 |
| EZRA_CHARGEATTACK | blockBreakerCombo | 1 |
| EZRA_BASE_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.6 |
| EZRA_BASE_FINISHER | finisherCombo,finisherCooldown | 0.0,20.0 |
| EZRA_BASE_RICOCHET_LIGHTSABER | lightsaberRicochetChance,lightsaberPerfectRicochetChance | 100,0 |
| EZRA_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| EZRA_BASE_PARRY | MeleeParry | 0 |
| EZRA_BASE_CLIP | clipSize | 8 |
| EZRA_BASE_DEFLECT | DeflectBlasterBoltsFromBlock,DeflectBlasterBoltsFromParry,DeflectBlasterBoltsFromIdle | 1.0,0.0,0.0 |
| EZRA_LAUNCHER | superStraightPunch | 1 |
| EZRA_BASE_RICOCHET | ricochetCount | 0 |
| EZRA_BASE_BLOCK_RICOCHET_LIGHTSABER | lightsaberBlockRicochetChance, lightsaberPerfectBlockRicochetChance | 100,10 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| EZRA_SPECIAL_RICOCHET2 | PRG_EZR_SpecialRicochet2 | specialChargeLevel,ricochetCount | 1.0,1.0 | 4 | 0 | 0 | 1 | 1 | PRG_SuperMoveUpgrade2_Screen | AV_PRG_SuperMoveUpgrade2 | PRG_EZR_SpecialRicochet2_desc |
| EZRA_RANGED_DMG2 | PRG_EZR_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.075,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_EZR_RangedDmg2_desc |
| EZRA_FORCE_JUMP | PRG_EZR_ForceJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1.0,11.4,0.6,44,10,12 | 2 | 0 | 0 | 0 |  | PRG_EZR_ForceJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_EZR_ForceJump1_desc |
| EZRA_RANGED_DMG3 | PRG_EZR_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.125,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_EZR_RangedDmg3_desc |
| EZRA_SPECIAL_DMG3 | PRG_EZR_SuperDmg3 | specialDamage | 0.4 | 5 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_EZR_SuperDmg3_desc |
| EZRA_CLIP2 | PRG_EZR_BlasterClip2 | clipSize | 14 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_EZR_BlasterClip2_desc |
| EZRA_FIRERATE2 | PRG_EZR_FireRate2 | fireRate | 4 | 4 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_EZR_FireRate2_desc |
| EZRA_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| EZRA_HEALTH1 | PRG_EZR_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_EZR_Health1_desc |
| EZRA_HEALTH2 | PRG_EZR_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_EZR_Health2_desc |
| EZRA_AUTO_DEFLECT | PRG_EZR_AutoDeflect | DeflectBlasterBoltsFromIdle | 1 | 5 | 0 | 0 | 1 |  | PRG_EZR_AutoDeflect_Screen | AV_PRG_DeflectAuto | PRG_EZR_AutoDeflect_desc |
| EZRA_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| EZRA_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| EZRA_POWERBAR2 | PRG_PowerBar2 | powerBar | 1 | 5 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar2_desc |
| EZRA_FORCE_PUSH | PRG_EZR_ForcePush1 | forcePush | 1 | 1 | 1 | 0 | 0 |  | PRG_EZR_ForcePush1_Screen | AV_PRG_ForcePull | PRG_EZR_ForcePush1_desc |
| EZRA_RANGED_DMG1 | PRG_EZR_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.05,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_EZR_RangedDmg1_desc |
| EZRA_PARRY | PRG_EZR_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_EZR_Parry_Screen | AV_PRG_RepelUnlock | PRG_EZR_Parry_desc |
| EZRA_TIMED_DEFLECT | PRG_EZR_TimedDeflect | DeflectBlasterBoltsFromParry | 1 | 3 | 0 | 0 | 0 |  | PRG_EZR_TimedDeflect_Screen | AV_PRG_DeflectTimed | PRG_EZR_TimedDeflect_desc |
| EZRA_FINISHER_DMG3 | PRG_EZR_FinisherDmg3 | finisherDamage | 0.35 | 6 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_EZR_FinisherDmg3_desc |
| EZRA_SPECIAL_DMG2 | PRG_EZR_SuperDmg2 | specialDamage | 0.35 | 3 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_EZR_SuperDmg2_desc |
| EZRA_POWERFILL1 | PRG_PowerFill1 | powerFill | 95 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| EZRA_MELEE_DMG1 | PRG_EZR_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_EZR_MeleeDmg1_desc |
| EZRA_FIRERATE1 | PRG_EZR_FireRate1 | fireRate | 3.5 | 2 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_EZR_FireRate1_desc |
| EZRA_FINISHER_DMG1 | PRG_EZR_FinisherDmg1 | finisherDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_EZR_FinisherDmg1_desc |
| EZRA_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Ezra |  |
| EZRA_POWERBAR1 | PRG_PowerBar1 | powerBar | 1 | 3 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar1_desc |
| EZRA_SPECIAL_DMG1 | PRG_EZR_SuperDmg1 | specialDamage | 0.25 | 1 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_EZR_SuperDmg1_desc |
| EZRA_FINISHER_DMG2 | PRG_EZR_FinisherDmg2 | finisherDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_EZR_FinisherDmg2_desc |
| EZRA_CLIP1 | PRG_EZR_BlasterClip1 | clipSize | 10 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_EZR_BlasterClip1_desc |
| EZRA_MELEE_DMG3 | PRG_EZR_MeleeDmg3 | meleeHit,meleeDamage | 1,0.35 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_EZR_MeleeDmg3_desc |
| EZRA_HEALTH3 | PRG_EZR_Health2 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_EZR_Health2_desc |
| EZRA_ATTACK_FINISHER2 | PRG_EZR_AttackFinisher2 | finisherCooldown | 20 | 5 | 0 | 0 | 0 |  | PRG_ComboFinisherUpgrade1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_EZR_AttackFinisher2_desc |
| EZRA_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| EZRA_ATTACK_FINISHER1 | PRG_EZR_AttackFinisher1 | finisherCombo,finisherCooldown | 1.0,25.0 | 2 | 1 | 0 | 1 |  | PRG_EZR_AttackFinisher1_Screen | AV_PRG_ComboFinisherUnlock | PRG_EZR_AttackFinisher1_desc |
| EZRA_PAUSECOMBO2 | PRG_EZR_PauseCombo2 | RangedComboYYRT | 1 | 4 | 0 | 0 | 0 |  | PRG_EZR_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_EZR_PauseCombo2_desc |
| EZRA_PAUSECOMBO1 | PRG_EZR_PauseCombo1 | PauseComboY_YY | 1 | 2 | 1 | 0 | 0 |  | PRG_EZR_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_EZR_PauseCombo1_desc |
| EZRA_SPECIAL_RICOCHET1 | PRG_EZR_SpecialRicochet1 | specialChargeLevel,ricochetCount | 1.0,1.0 | 2 | 0 | 0 | 1 | 1 | PRG_SuperMoveUpgrade1_Screen | AV_PRG_SuperMoveUpgrade1 | PRG_EZR_SpecialRicochet1_desc |
| EZRA_SPECIAL_UNLOCKED | PRG_EZR_SpecialUnlocked | specialUnlocked,powerBar,powerFill,specialChargeLevel,ricochetCount | 1.0,2.0,120.0,1.0,2.0 | 1 | 1 | 0 | 1 |  | PRG_EZR_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_EZR_SpecialUnlocked_desc |
