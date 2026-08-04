# EMP_Luke

`ProgressionTree = "IN3_EMP_Luke"` -- source: `gamedb/core/in3_emp_luke.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| LUKE_BASE_HEALTH | health | 150 |
| LUKE_BASE_FIRERATE | fireRate | 3 |
| LUKE_LAUNCHER | superStraightPunch | 1 |
| LUKE_FINISHER_COOLDOWN | finisherCombo,finisherCooldown | 0,20 |
| LUKE_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| LUKE_BASEPARRY | MeleeParry | 0 |
| LUKE_CHARGEATTACK | blockBreakerCombo | 1 |
| LUKE_BASE_CLIP | clipSize | 8 |
| LUKE_BASE_FORCEPULL | forcePull | 1 |
| LUKE_BASE_HEALTHREGEN | healthRegen | 0 |
| LUKE_BASEDEFLECT | DeflectBlasterBoltsFromBlock,DeflectBlasterBoltsFromParry,DeflectBlasterBoltsFromIdle,lightsaberRicochetChance,lightsaberPerfectRicochetChance | 1.0,0.0,0.0,100.0,0.0 |
| LUKE_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.6 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| LUKE_CLIP2 | PRG_LUK_BlasterClip2 | clipSize | 14 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_LUK_BlasterClip2_desc |
| LUKE_GRID4 | PRG_LUK_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| LUKE_POWERFILL1 | PRG_PowerFill1 | powerFill | 95 | 2 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| LUKE_FORCE_SPECIAL1 | PRG_LUK_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDuration,DashAttack | 1.0,1.0,120.0,15.0,1.0 | 1 | 1 | 0 | 1 |  | PRG_LUK_SpecialUnlocked | AV_PRG_SuperMoveUnlock | PRG_LUK_SpecialUnlocked_desc |
| LUKE_FORCE_SPECIAL2 | PRG_LUK_ForceSpecial2 | AutoDeflectAndParry | 1 | 5 | 0 | 0 | 1 |  | PRG_LUK_ForceSpecial2 | AV_PRG_SuperMeterChargeUpgrade1 | PRG_LUK_ForceSpecial2_desc |
| LUKE_HEALTH2 | PRG_LUK_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_LUK_Health2_desc |
| LUKE_FORCE_JUMP | PRG_LUK_ForceJump | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1.0,11.4,0.6,44,10,12 | 2 | 0 | 0 | 0 |  | PRG_LUK_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_LUK_ForceJump_desc |
| LUKE_AERIALPAUSECOMBO | PRG_LUK_AerialPauseCombo | AerialPauseComboY_YY | 1 | 4 | 0 | 0 | 0 |  | PRG_LUK_AerialPauseCombo | AV_PRG_PauseComboAerial | PRG_LUK_AerialPauseCombo_desc |
| LUKE_FORCE_FINISHER1 | PRG_LUK_ForceFinisher1 | finisherCombo,finisherCooldown | 1.0,25.0 | 4 | 1 | 0 | 1 |  | PRG_LUK_ForceFinisher_Screen | AV_PRG_ComboFinisherUnlock | PRG_LUK_ForceFinisher1_desc |
| LUKE_SPECIAL_DURATION2 | PRG_LUK_PowerDuration2 | powerDuration | 6 | 5 | 0 | 0 | 0 | 1 | PRG_SuperMoveDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_LUK_PowerDuration2_desc |
| LUKE_AUTO_DEFLECT | PRG_LUK_AutoDeflect | DeflectBlasterBoltsFromIdle | 1 | 6 | 0 | 0 | 1 |  | PRG_LUK_AutoRicochetBlock_Screen | AV_PRG_DeflectAuto | PRG_LUK_AutoDeflect_desc |
| LUKE_TIMED_DEFLECT | PRG_LUK_TimedDeflect | DeflectBlasterBoltsFromParry | 1 | 4 | 0 | 0 | 0 |  | PRG_LUK_RicochetBlock_Screen | AV_PRG_DeflectTimed | PRG_LUK_TimedDeflect_desc |
| LUKE_RANGED_DMG1 | PRG_LUK_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.05,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LUK_RangedDmg1_desc |
| LUKE_PARRY | PRG_LUK_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_LUK_Repel_Screen | AV_PRG_RepelUnlock | PRG_LUK_Parry_desc |
| LUKE_FIRERATE2 | PRG_LUK_FireRate2 | fireRate | 4 | 4 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_LUK_FireRate2_desc |
| LUKE_HEALTH3 | PRG_LUK_Health2 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_LUK_Health2_desc |
| LUKE_MELEE_DMG1 | PRG_LUK_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LUK_MeleeDmg1_desc |
| LUKE_RANGED_DMG3 | PRG_LUK_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.125,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LUK_RangedDmg3_desc |
| LUKE_FIRERATE1 | PRG_LUK_FireRate1 | fireRate | 3.5 | 2 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_LUK_FireRate1_desc |
| LUKE_RANGED_DMG2 | PRG_LUK_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.075,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LUK_RangedDmg2_desc |
| LUKE_GRID1 | PRG_LUK_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| LUKE_MELEE_DMG2 | PRG_LUK_MeleeDmg2 | meleeHit,meleeDamage | 1,0.2 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LUK_MeleeDmg2_desc |
| LUKE_FORCE_FINISHER2 | PRG_LUK_ForceFinisher2 | finisherCooldown | 20 | 6 | 0 | 0 | 0 |  | PRG_LUK_ForceFinisher_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_LUK_ForceFinisher2_desc |
| LUKE_HEALTH1 | PRG_LUK_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_LUK_Health1_desc |
| LUKE_CLIP1 | PRG_LUK_BlasterClip1 | clipSize | 10 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_LUK_BlasterClip1_desc |
| LUKE_PAUSECOMBO2 | PRG_LUK_PauseCombo2 | PauseComboYY_YY | 1 | 6 | 0 | 0 | 0 |  | PRG_LUK_PauseCombo2 | AV_PRG_PauseCombo2 | PRG_LUK_PauseCombo2_desc |
| LUKE_SPECIAL_DURATION1 | PRG_LUK_PowerDuration1 | powerDuration | 4 | 3 | 0 | 0 | 0 | 1 | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_LUK_PowerDuration1_desc |
| LUKE_PAUSECOMBO1 | PRG_LUK_PauseCombo1 | PauseComboY_YY | 1 | 2 | 1 | 0 | 0 |  | PRG_LUK_PauseCombo1 | AV_PRG_PauseCombo1 | PRG_LUK_PauseCombo1_desc |
| LUKE_PARRY_COUNTER | PRG_LUK_ParryCounter | repelAttack | 1 | 5 | 1 | 0 | 0 |  | PRG_LUK_ParryCounter_Screen | AV_PRG_ParryCounter | PRG_LUK_ParryCounter_desc |
| LUKE_GRID3 | PRG_LUK_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| LUKE_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Luke |  |
| LUKE_GRID2 | PRG_LUK_ForceGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| LUKE_MELEE_DMG3 | PRG_LUK_MeleeDmg3 | meleeHit,meleeDamage | 1,0.35 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LUK_MeleeDmg3_desc |
