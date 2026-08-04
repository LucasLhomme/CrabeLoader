# TCW_Anakin

`ProgressionTree = "IN3_TCW_Anakin"` -- source: `gamedb/core/in3_tcw_anakin.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| ANAKIN_BASE_DEFLECT_FROM_IDLE | DeflectBlasterBoltsFromIdle | 0 |
| ANAKIN_BASE_DEFLECT_FROM_PARRY | DeflectBlasterBoltsFromParry | 0 |
| ANAKIN_BASEPARRY | MeleeParry | 0 |
| ANAKIN_BASE_BLOCK_RICOCHET_LIGHTSABER | lightsaberBlockRicochetChance, lightsaberPerfectBlockRicochetChance | 100,10 |
| ANAKIN_LAUNCHER | superStraightPunch | 1 |
| ANAKIN_BASE_RICOCHET_LIGHTSABER | lightsaberRicochetChance,lightsaberPerfectRicochetChance | 100,0 |
| ANAKIN_BASE_DEFLECT_FROM_BLOCK | DeflectBlasterBoltsFromBlock | 1 |
| ANAKIN_BASE_FINISHER | finisherCombo,finisherCooldown | 0.0,30.0 |
| ANAKIN_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| ANAKIN_BASE_FORCEPULL | forcePull | 1 |
| ANAKIN_CHARGEATTACK | blockBreakerCombo | 1 |
| ANAKIN_BASE_SPECIALCHARGE | specialChargeLevel | 1 |
| SPECIAL_NOTARGET_SHOOT_DISTANCE | NoTargetShootDistance | 8 |
| ANAKIN_BASE_HEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ANAKIN_POWERBAR2 | PRG_PowerBar2 | powerBar | 1 | 4 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar2_desc |
| ANAKIN_MELEE_DMG1 | PRG_ANI_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ANI_MeleeDmg1_desc |
| ANAKIN_FINISHER_DMG2 | PRG_ANI_FinisherDmg2 | finisherDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_ANI_FinisherDmg2_desc |
| ANAKIN_SPECIAL_DMG1 | PRG_ANI_SuperDmg1 | specialDamage | 0.2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_ANI_SuperDmg1_desc |
| ANAKIN_FORCE_SPECIAL3 | PRG_ANI_ForceSpecial3 | specialChargeLevel | 1 | 5 | 0 | 0 | 1 | 1 | PRG_SuperMoveUpgrade1_Screen | AV_PRG_SuperMoveUpgrade1 | PRG_ANI_ForceSpecial3_desc |
| ANAKIN_GRID2 | PRG_TCW_ForceGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| ANAKIN_MELEE_DMG2 | PRG_ANI_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ANI_MeleeDmg2_desc |
| ANAKIN_TIMED_DEFLECT | PRG_ANI_TimedDeflect | DeflectBlasterBoltsFromParry | 1 | 4 | 0 | 0 | 0 |  | PRG_ANI_TimedDeflect_Screen | AV_PRG_DeflectTimed | PRG_ANI_TimedDeflect_desc |
| ANAKIN_HEALTH1 | PRG_ANI_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_ANI_Health1_desc |
| ANAKIN_PAUSECOMBO1 | PRG_ANI_PauseCombo1 | PauseComboY_YY | 1 | 2 | 0 | 0 | 0 |  | PRG_ANI_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_ANI_PauseCombo1_desc |
| ANAKIN_POWERFILL1 | PRG_PowerFill1 | powerFill | 120 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| ANAKIN_FINISHER_DMG1 | PRG_ANI_FinisherDmg1 | finisherDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_ANI_FinisherDmg1_desc |
| ANAKIN_RANGED_DMG2 | PRG_ANI_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.25,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ANI_RangedDmg2_desc |
| ANAKIN_MELEE_DMG3 | PRG_ANI_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ANI_MeleeDmg3_desc |
| ANAKIN_FORCE_SPECIAL1 | PRG_ANI_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,150.0 | 1 | 1 | 0 | 1 |  | PRG_ANI_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_ANI_SpecialUnlocked_desc |
| ANAKIN_FORCE_FINISHER2 | PRG_ANI_ForceFinisher2 | finisherCooldown | 20 | 4 | 0 | 0 | 0 |  | PRG_ANI_ForceFinisher1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_ANI_ForceFinisher2_desc |
| ANAKIN_FORCE_JUMP | PRG_ANI_ForceJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1.0,11.4,0.6,44,10,12 | 2 | 0 | 0 | 0 |  | PRG_ANI_ForceJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_ANI_ForceJump1_desc |
| ANAKIN_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Anakin |  |
| ANAKIN_GRID1 | PRG_TCW_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| ANAKIN_POWERFILL2 | PRG_PowerFill2 | powerFill | 95 | 5 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate2_Screen | AV_PRG_SuperMeterFillRate2 | PRG_PowerFill2_desc |
| ANAKIN_FORCE_SPECIAL2 | PRG_ANI_ForceSpecial2 | specialChargeLevel | 1 | 3 | 0 | 0 | 1 | 1 | PRG_SuperMoveUpgrade1_Screen | AV_PRG_SuperMoveUpgrade1 | PRG_ANI_ForceSpecial2_desc |
| ANAKIN_RANGED_DMG3 | PRG_ANI_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.4,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ANI_RangedDmg3_desc |
| ANAKIN_FORCE_FINISHER1 | PRG_ANI_ForceFinisher1 | finisherCombo,finisherCooldown | 1.0,25.0 | 2 | 1 | 0 | 1 |  | PRG_ANI_ForceFinisher1_Screen | AV_PRG_ComboFinisherUnlock | PRG_ANI_ForceFinisher1_desc |
| ANAKIN_FINISHER_DMG3 | PRG_ANI_FinisherDmg3 | finisherDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_ANI_FinisherDmg3_desc |
| ANAKIN_RANGED_DMG1 | PRG_ANI_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.15,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ANI_RangedDmg1_desc |
| ANAKIN_HEALTH2 | PRG_ANI_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_ANI_Health2_desc |
| ANAKIN_DASH_ATTACK | PRG_ANI_DashAttack | DashAttack | 1 | 1 | 1 | 0 | 0 |  | PRG_ANI_ForceFinisher2_Screen | AV_PRG_DashAttack | PRG_ANI_DashAttack_desc |
| ANAKIN_AERIALPAUSECOMBO | PRG_ANI_AerialPauseCombo | AerialPauseComboY_YY | 1 | 3 | 1 | 0 | 0 |  | PRG_ANI_PauseCombo3_Screen | AV_PRG_PauseComboAerial | PRG_ANI_AerialPauseCombo_desc |
| ANAKIN_HEALTH3 | PRG_ANI_Health3 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_ANI_Health3_desc |
| ANAKIN_PAUSECOMBO2 | PRG_ANI_PauseCombo2 | PauseComboYY_YY | 1 | 5 | 0 | 0 | 0 |  | PRG_ANI_PauseCombo1_Screen | AV_PRG_PauseCombo2 | PRG_ANI_PauseCombo2_desc |
| ANAKIN_GRID4 | PRG_TCW_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| ANAKIN_SPECIAL_DMG3 | PRG_ANI_SuperDmg3 | specialDamage | 0.5 | 5 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_ANI_SuperDmg3_desc |
| ANAKIN_GRID3 | PRG_TCW_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| ANAKIN_SPECIAL_DMG2 | PRG_ANI_SuperDmg2 | specialDamage | 0.3 | 3 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_ANI_SuperDmg2_desc |
| ANAKIN_POWERBAR1 | PRG_PowerBar1 | powerBar | 1 | 2 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar1_desc |
| ANAKIN_AUTO_DEFLECT | PRG_ANI_AutoDeflect | DeflectBlasterBoltsFromIdle | 1 | 6 | 0 | 0 | 1 |  | PRG_ANI_AutoDeflect_Screen | AV_PRG_DeflectAuto | PRG_ANI_AutoDeflect_desc |
| ANAKIN_PARRY | PRG_ANI_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_ANI_Parry_Screen | AV_PRG_RepelUnlock | PRG_ANI_Parry_desc |
