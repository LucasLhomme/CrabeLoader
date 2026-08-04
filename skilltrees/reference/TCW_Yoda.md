# TCW_Yoda

`ProgressionTree = "IN3_TCW_Yoda"` -- source: `gamedb/core/in3_tcw_yoda.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| YODA_LAUNCHER | superStraightPunch | 1 |
| YODA_BASEPARRY | MeleeParry | 0 |
| YODA_BASE_RICOCHET_LIGHTSABER | lightsaberRicochetChance,lightsaberPerfectRicochetChance | 100,0 |
| YODA_BASE_FINISHER | finisherCombo,finisherCooldown | 0.0,30.0 |
| YODA_BASE_SUPERJUMP | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 0.0,11.4,0.6,14,8.5,8 |
| YODA_CHARGEATTACK | blockBreakerCombo | 1 |
| YODA_BASE_DEFLECT_FROM_IDLE | DeflectBlasterBoltsFromIdle | 0 |
| YODA_BASE_DEFLECT_FROM_BLOCK | DeflectBlasterBoltsFromBlock | 1 |
| YODA_BASE_FORCEPUSH | forcePush | 1 |
| YODA_BASE_HEALTHREGEN | healthRegen | 0 |
| YODA_BASE_BLOCK_RICOCHET_LIGHTSABER | lightsaberBlockRicochetChance, lightsaberPerfectBlockRicochetChance | 100,10 |
| YODA_BASE_DEFLECT_FROM_PARRY | DeflectBlasterBoltsFromParry | 0 |
| YODA_BASE_HEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| YODA_AUTO_DEFLECT | PRG_YDA_AutoDeflect | DeflectBlasterBoltsFromIdle | 1 | 6 | 0 | 0 | 1 |  | PRG_YDA_AutoDeflect_Screen | AV_PRG_DeflectAuto | PRG_YDA_AutoDeflect_desc |
| YODA_AERIALPAUSECOMBO | PRG_YDA_AerialPauseCombo | AerialPauseComboY_YY | 1 | 3 | 0 | 0 | 0 |  | PRG_YDA_AerialPauseCombo_Screen | AV_PRG_PauseComboAerial | PRG_YDA_AerialPauseCombo_desc |
| YODA_FORCE_SPECIAL1 | PRG_YDA_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,3.0,60 | 1 | 1 | 0 | 1 |  | PRG_YDA_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_YDA_SpecialUnlocked_desc |
| YODA_POWERBAR1 | PRG_PowerBar1 | powerBar | 3 | 1 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade4_Screen | AV_PRG_SuperMeterSizeUpgrade4 | PRG_PowerBar1_desc |
| YODA_POWERBAR3 | PRG_PowerBar3 | powerBar | 3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade4_Screen | AV_PRG_SuperMeterSizeUpgrade4 | PRG_PowerBar3_desc |
| YODA_GRID4 | PRG_TCW_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| YODA_RANGED_DMG1 | PRG_YDA_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.15,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_YDA_RangedDmg1_desc |
| YODA_POWERBAR4 | PRG_PowerBar3 | powerBar | 3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade4_Screen | AV_PRG_SuperMeterSizeUpgrade4 | PRG_PowerBar3_desc |
| YODA_SPECIAL_DMG3 | PRG_YDA_SuperDmg3 | specialDamage | 0.5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_YDA_SuperDmg3_desc |
| YODA_DASH_ATTACK | PRG_YDA_DashAttack | DashAttack | 1 | 1 | 0 | 0 | 1 |  | PRG_DashAttack_Screen | AV_PRG_DashAttack | PRG_YDA_DashAttack_desc |
| YODA_SPECIAL_DMG2 | PRG_YDA_SuperDmg2 | specialDamage | 0.3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_YDA_SuperDmg2_desc |
| YODA_FINISHER_DMG2 | PRG_YDA_FinisherDmg2 | finisherDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_YDA_FinisherDmg2_desc |
| YODA_GRID3 | PRG_TCW_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| YODA_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Yoda |  |
| YODA_PAUSECOMBO1 | PRG_YDA_PauseCombo1 | PauseComboY_YY | 1 | 2 | 1 | 0 | 0 |  | PRG_YDA_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_YDA_PauseCombo1_desc |
| YODA_POWERBAR5 | PRG_PowerBar3 | powerBar | 3 | 6 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade4_Screen | AV_PRG_SuperMeterSizeUpgrade4 | PRG_PowerBar3_desc |
| YODA_FINISHER_DMG1 | PRG_YDA_FinisherDmg1 | finisherDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_YDA_FinisherDmg1_desc |
| YODA_MELEE_DMG2 | PRG_YDA_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_YDA_MeleeDmg2_desc |
| YODA_FORCE_FINISHER2 | PRG_YDA_ForceFinisher2 | finisherCooldown | 20 | 5 | 0 | 0 | 0 |  | PRG_ComboFinisherUpgrade1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_YDA_ForceFinisher2_desc |
| YODA_FORCE_JUMP | PRG_YDA_ForceJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1.0,11.4,0.6,44,10,12 | 2 | 0 | 0 | 0 |  | PRG_YDA_ForceJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_YDA_ForceJump1_desc |
| YODA_FINISHER_DMG3 | PRG_YDA_FinisherDmg3 | finisherDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_YDA_FinisherDmg3_desc |
| YODA_POWERBAR2 | PRG_PowerBar2 | powerBar | 3 | 1 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade4_Screen | AV_PRG_SuperMeterSizeUpgrade4 | PRG_PowerBar2_desc |
| YODA_HEALTH1 | PRG_YDA_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_YDA_Health1_desc |
| YODA_RANGED_DMG3 | PRG_YDA_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.4,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_YDA_RangedDmg3_desc |
| YODA_MELEE_DMG3 | PRG_YDA_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_YDA_MeleeDmg3_desc |
| YODA_RANGED_DMG2 | PRG_YDA_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.25,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_YDA_RangedDmg2_desc |
| YODA_MELEE_DMG1 | PRG_YDA_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_YDA_MeleeDmg1_desc |
| YODA_POWERFILL1 | PRG_PowerFill1 | powerFill | 30 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| YODA_PAUSECOMBO2 | PRG_YDA_PauseCombo2 | PauseComboYY_YY | 1 | 5 | 0 | 0 | 0 |  | PRG_YDA_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_YDA_PauseCombo2_desc |
| YODA_GRID2 | PRG_TCW_ForceGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| YODA_PARRY | PRG_YDA_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_YDA_Parry_Screen | AV_PRG_RepelUnlock | PRG_YDA_Parry_desc |
| YODA_TIMED_DEFLECT | PRG_YDA_TimedDeflect | DeflectBlasterBoltsFromParry | 1 | 4 | 1 | 0 | 0 |  | PRG_YDA_TimedDeflect_Screen | AV_PRG_DeflectTimed | PRG_YDA_TimedDeflect_desc |
| YODA_HEALTH3 | PRG_YDA_Health3 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_YDA_Health3_desc |
| YODA_FORCE_FINISHER1 | PRG_YDA_ForceFinisher1 | finisherCombo,finisherCooldown,whirlwindDamage | 1.0,25,40 | 3 | 1 | 0 | 1 |  | PRG_YDA_ForceFinisher1 | AV_PRG_ComboFinisherUnlock | PRG_YDA_ForceFinisher1_desc |
| YODA_HEALTH2 | PRG_YDA_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_YDA_Health2_desc |
| YODA_SPECIAL_DMG1 | PRG_YDA_SuperDmg1 | specialDamage | 0.2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_YDA_SuperDmg1_desc |
| YODA_GRID1 | PRG_TCW_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
