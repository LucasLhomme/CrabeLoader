# TCW_DarthMaul

`ProgressionTree = "IN3_TCW_DarthMaul"` -- source: `gamedb/core/in3_tcw_darthmaul.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| DARTHMAUL_BASE_BLOCK_RICOCHET_LIGHTSABER | lightsaberBlockRicochetChance, lightsaberPerfectBlockRicochetChance | 100,10 |
| DARTHMAUL_BASE_DEFLECT_FROM_IDLE | DeflectBlasterBoltsFromIdle | 0 |
| DARTHMAUL_BASE_DEFLECT_FROM_PARRY | DeflectBlasterBoltsFromParry | 0 |
| DARTHMAUL_BASE_HEALTH | health | 150 |
| DARTHMAUL_BASE_HEALTHREGEN | healthRegen | 0 |
| DARTHMAUL_BASEPARRY | MeleeParry | 0 |
| DARTHMAUL_SITHPOWER | sithpowerUnlocked | 0 |
| DARTHMAUL_SITHPOWER_DMG | sithpowerDamage | 1 |
| DARTHMAUL_BASE_FORCEPUSH | forcePush | 1 |
| DARTHMAUL_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| DARTHMAUL_LAUNCHER | superStraightPunch | 1 |
| DARTHMAUL_BASE_FINISHER | finisherCombo,finisherCooldown | 0.0,30.0 |
| DARTHMAUL_CHARGEATTACK | blockBreakerCombo | 1 |
| DARTHMAUL_BASE_RICOCHET_LIGHTSABER | lightsaberRicochetChance,lightsaberPerfectRicochetChance | 100,0 |
| DARTHMAUL_BASE_DEFLECT_FROM_BLOCK | DeflectBlasterBoltsFromBlock | 1 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| DARTHMAUL_TIMED_DEFLECT | PRG_DML_TimedDeflect | DeflectBlasterBoltsFromParry | 1 | 4 | 0 | 0 | 0 |  | PRG_DeflectTimed_Screen | AV_PRG_DeflectTimed | PRG_DML_TimedDeflect_desc |
| DARTHMAUL_GRID1 | PRG_TCW_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| DARTHMAUL_FINISHER_DMG3 | PRG_DML_FinisherDmg3 | finisherDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_DML_FinisherDmg3_desc |
| DARTHMAUL_HEALTH1 | PRG_DML_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_DML_Health1_desc |
| DARTHMAUL_RANGED_DMG2 | PRG_DML_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.25,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DML_RangedDmg2_desc |
| DARTHMAUL_PARRY | PRG_DML_Parry | MeleeParry | 1 | 2 | 1 | 0 | 0 |  | PRG_DML_Parry_Screen | AV_PRG_RepelUnlock | PRG_DML_Parry_desc |
| DARTHMAUL_GRID4 | PRG_TCW_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| DARTHMAUL_PAUSECOMBO1 | PRG_DML_PauseCombo1 | PauseComboY_YY | 1 | 2 | 1 | 0 | 0 |  | PRG_DML_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_DML_PauseCombo1_desc |
| DARTHMAUL_POWERFILL1 | PRG_PowerFill1 | powerFill | 120 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| DARTHMAUL_SPECIAL_DMG2 | PRG_DML_SuperDmg2 | specialDamage | 0.45 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_DML_SuperDmg2_desc |
| DARTHMAUL_SITHPOWER_DMG3 | PRG_DML_SithPowerDmg3 | sithpowerDamage | 0.12 | 6 | 0 | 0 | 0 | 1 | PRG_SithPowerUnlock_Screen | AV_PRG_SithPowerUnlock | PRG_DML_SithPowerDmg3_desc |
| DARTHMAUL_POWERBAR2 | PRG_PowerBar2 | powerBar | 1 | 5 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar2_desc |
| DARTHMAUL_RANGED_DMG3 | PRG_DML_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.4,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DML_RangedDmg3_desc |
| DARTHMAUL_FORCE_JUMP | PRG_DML_ForceJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1.0,11.4,0.6,44,10,12 | 2 | 0 | 0 | 0 |  | PRG_DML_ForceJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_DML_ForceJump1_desc |
| DARTHMAUL_HEALTH2 | PRG_DML_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_DML_Health2_desc |
| DARTHMAUL_GRID3 | PRG_TCW_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| DARTHMAUL_MELEE_DMG2 | PRG_DML_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DML_MeleeDmg2_desc |
| DARTHMAUL_AERIALPAUSECOMBO | PRG_BEN_AerialPauseCombo | AerialPauseComboY_YY | 1 | 3 | 0 | 0 | 0 |  | PRG_PauseComboAerial_Screen | AV_PRG_PauseComboAerial | PRG_BEN_AerialPauseCombo_desc |
| DARTHMAUL_DASH_ATTACK | PRG_DML_DashAttack | DashAttack | 1 | 1 | 0 | 0 | 1 |  | PRG_MeleeDmg1_Screen | AV_PRG_DashAttack | PRG_DML_DashAttack_desc |
| DARTHMAUL_SPECIAL_DMG1 | PRG_DML_SuperDmg1 | specialDamage | 0.3 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_DML_SuperDmg1_desc |
| DARTHMAUL_FORCE_SPECIAL2 | PRG_DML_ForceSpecial2 | specialUpgrade1 | 1 | 4 | 1 | 0 | 1 |  | PRG_DML_ForceSpecial2_Screen | AV_PRG_SuperMeterChargeUpgrade1 | PRG_DML_ForceSpecial2_desc |
| DARTHMAUL_FINISHER_DMG1 | PRG_DML_FinisherDmg1 | finisherDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_DML_FinisherDmg1_desc |
| DARTHMAUL_FORCE_FINISHER2 | PRG_DML_ForceFinisher2 | finisherCooldown | 20 | 4 | 0 | 0 | 0 |  | PRG_ComboFinisherUpgrade1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_DML_ForceFinisher2_desc |
| DARTHMAUL_SITHPOWER_DMG2 | PRG_DML_SithPowerDmg2 | sithpowerDamage | 0.08 | 4 | 0 | 0 | 0 | 1 | PRG_SithPowerUnlock_Screen | AV_PRG_SithPowerUnlock | PRG_DML_SithPowerDmg2_desc |
| DARTHMAUL_GRID2 | PRG_TCW_ForceGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| DARTHMAUL_SITHPOWER_DMG1 | PRG_DML_SithPowerDmg1 | sithpowerDamage | 0.05 | 4 | 0 | 0 | 0 | 1 | PRG_SithPowerUnlock_Screen | AV_PRG_SithPowerUnlock | PRG_DML_SithPowerDmg1_desc |
| DARTHMAUL_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_DarthMaul |  |
| DARTHMAUL_SPECIAL_DMG3 | PRG_DML_SuperDmg3 | specialDamage | 0.75 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_DML_SuperDmg3_desc |
| DARTHMAUL_SITHPOWER | PRG_DML_SithPower | sithpowerUnlocked | 1 | 3 | 0 | 0 | 1 |  | PRG_SithPowerUnlock_Screen | AV_PRG_SithPowerUnlock | PRG_DML_SithPower_desc |
| DARTHMAUL_FINISHER_DMG2 | PRG_DML_FinisherDmg2 | finisherDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_DML_FinisherDmg2_desc |
| DARTHMAUL_FORCE_SPECIAL1 | PRG_DML_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,150 | 1 | 1 | 0 | 1 |  | PRG_DML_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_DML_SpecialUnlocked_desc |
| DARTHMAUL_RANGED_DMG1 | PRG_DML_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.15,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DML_RangedDmg1_desc |
| DARTHMAUL_HEALTH3 | PRG_DML_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_DML_Health3_desc |
| DARTHMAUL_FORCE_FINISHER1 | PRG_DML_ForceFinisher1 | finisherCombo,finisherCooldown | 1.0,25.0 | 2 | 0 | 0 | 1 |  | PRG_ComboFinisherUnlock_Screen | AV_PRG_ComboFinisherUnlock | PRG_DML_ForceFinisher1_desc |
| DARTHMAUL_POWERBAR1 | PRG_PowerBar1 | powerBar | 1 | 2 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar1_desc |
| DARTHMAUL_MELEE_DMG3 | PRG_DML_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DML_MeleeDmg3_desc |
| DARTHMAUL_AUTO_DEFLECT | PRG_DML_AutoDeflect | DeflectBlasterBoltsFromIdle | 1 | 6 | 0 | 0 | 1 |  | PRG_DML_AutoDeflect_Screen | AV_PRG_DeflectAuto | PRG_DML_AutoDeflect_desc |
| DARTHMAUL_PAUSECOMBO2 | PRG_DML_PauseCombo2 | PauseComboYY_YY | 1 | 5 | 0 | 0 | 0 |  | PRG_DML_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_DML_PauseCombo2_desc |
| DARTHMAUL_MELEE_DMG1 | PRG_DML_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DML_MeleeDmg1_desc |
