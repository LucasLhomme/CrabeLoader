# TCW_ObiWan

`ProgressionTree = "IN3_TCW_Obiwan"` -- source: `gamedb/core/in3_tcw_obiwan.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| OBIWAN_BASE_HEALTHREGEN | healthRegen | 0 |
| OBIWAN_BASE_DEFLECT_FROM_PARRY | DeflectBlasterBoltsFromParry | 0 |
| OBIWAN_BASE_RICOCHET_LIGHTSABER | lightsaberRicochetChance,lightsaberPerfectRicochetChance | 100,0 |
| OBIWAN_BASE_HEALTH | health | 150 |
| OBIWAN_BASE_FORCEPUSH | forcePush | 1 |
| OBIWAN_BASE_BLOCK_RICOCHET_LIGHTSABER | lightsaberBlockRicochetChance, lightsaberPerfectBlockRicochetChance | 100,10 |
| OBIWAN_BASE_FINISHERSTUN | powerStun | 1.5 |
| OBIWAN_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| OBIWAN_LAUNCHER | superStraightPunch | 1 |
| OBIWAN_BASE_DEFLECT_FROM_IDLE | DeflectBlasterBoltsFromIdle | 0 |
| OBIWAN_BASEPARRY | MeleeParry | 0 |
| OBIWAN_BASE_FINISHER | finisherCombo,finisherCooldown | 0.0,30.0 |
| OBIWAN_CHARGEATTACK | blockBreakerCombo | 1 |
| OBIWAN_BASE_DEFLECT_FROM_BLOCK | DeflectBlasterBoltsFromBlock | 1 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| OBIWAN_FORCE_FINISHER1 | PRG_BEN_ForceFinisher1 | finisherCombo,finisherCooldown,powerStunNPC,powerStunIGP | 1.0,25.0,10.0,5.0 | 2 | 0 | 0 | 1 |  | PRG_ComboFinisherUnlock_Screen | AV_PRG_ComboFinisherUnlock | PRG_BEN_ForceFinisher1_desc |
| OBIWAN_SPECIAL_STUN1 | PRG_BEN_SpecialStun1 | powerDurationNPC,powerDurationIGP | 30.0,6.0 | 2 | 0 | 0 | 0 |  | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_BEN_SpecialStun1_desc |
| OBIWAN_RANGED_DMG1 | PRG_BEN_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.15,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BEN_RangedDmg1_desc |
| OBIWAN_POWERBAR2 | PRG_PowerBar2 | powerBar | 1 | 4 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar2_desc |
| OBIWAN_POWERBAR1 | PRG_PowerBar1 | powerBar | 1 | 1 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar1_desc |
| OBIWAN_FORCE_FINISHER2 | PRG_BEN_ForceFinisher2 | finisherCooldown | 20 | 4 | 0 | 0 | 0 |  | PRG_ComboFinisherUpgrade1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_BEN_ForceFinisher2_desc |
| OBIWAN_POWERFILL2 | PRG_PowerFill2 | powerFill | 95 | 5 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate2_Screen | AV_PRG_SuperMeterFillRate2 | PRG_PowerFill2_desc |
| OBIWAN_HEALTH2 | PRG_BEN_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_BEN_Health2_desc |
| OBIWAN_PARRY_COUNTER | PRG_BEN_ParryCounter | repelAttack | 1 | 4 | 1 | 0 | 0 |  | PRG_BEN_ParryCounter_Screen | AV_PRG_ParryCounter | PRG_BEN_ParryCounter_desc |
| OBIWAN_MELEE_DMG2 | PRG_BEN_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BEN_MeleeDmg2_desc |
| OBIWAN_POWERFILL1 | PRG_PowerFill1 | powerFill | 120 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| OBIWAN_GRID1 | PRG_TCW_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| OBIWAN_FINISHER_STUN3 | PRG_BEN_FinisherStun3 | powerStunNPC,powerStunIGP | 2.0,1.0 | 6 | 0 | 0 | 0 | 1 | PRG_FinisherDuration_A_Screen | AV_PRG_FinisherDuration_A | PRG_BEN_FinisherStun3_desc |
| OBIWAN_FINISHER_STUN1 | PRG_BEN_FinisherStun1 | powerStunNPC,powerStunIGP | 2.0,1.0 | 2 | 0 | 0 | 0 | 1 | PRG_FinisherDuration_A_Screen | AV_PRG_FinisherDuration_A | PRG_BEN_FinisherStun1_desc |
| OBIWAN_HEALTH3 | PRG_BEN_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_BEN_Health3_desc |
| OBIWAN_FORCE_SPECIAL2 | PRG_BEN_ForceSpecial2 | specialUpgrade1 | 1 | 3 | 0 | 0 | 1 |  | PRG_SuperMeterChargeUpgrade1_Screen | AV_PRG_SuperMeterChargeUpgrade1 | PRG_BEN_ForceSpecial2_desc |
| OBIWAN_POWERBAR3 | PRG_PowerBar3 | powerBar | 1 | 6 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar3_desc |
| OBIWAN_GRID2 | PRG_TCW_ForceGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| OBIWAN_FORCE_SPECIAL1 | PRG_BEN_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDurationNPC,powerDurationIGP | 1.0,1.0,150.0,20.0,4.0 | 1 | 1 | 0 | 1 |  | PRG_BEN_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_BEN_SpecialUnlocked_desc |
| OBIWAN_GRID4 | PRG_TCW_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| OBIWAN_MELEE_DMG3 | PRG_BEN_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BEN_MeleeDmg3_desc |
| OBIWAN_TIMED_DEFLECT | PRG_BEN_TimedDeflect | DeflectBlasterBoltsFromParry | 1 | 4 | 1 | 0 | 0 |  | PRG_BEN_TimedDeflect_Screen | AV_PRG_DeflectTimed | PRG_BEN_TimedDeflect_desc |
| OBIWAN_HEALTH1 | PRG_BEN_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_BEN_Health1_desc |
| OBIWAN_RANGED_DMG3 | PRG_BEN_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.4,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BEN_RangedDmg3_desc |
| OBIWAN_RANGED_DMG2 | PRG_BEN_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.25,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BEN_RangedDmg2_desc |
| OBIWAN_DASH_ATTACK | PRG_BEN_DashAttack | DashAttack | 1 | 1 | 0 | 0 | 0 |  | PRG_BEN_DashAttack_Screen | AV_PRG_DashAttack | PRG_BEN_DashAttack_desc |
| OBIWAN_AERIALPAUSECOMBO | PRG_BEN_AerialPauseCombo | AerialPauseComboY_YY | 1 | 3 | 0 | 0 | 0 |  | PRG_BEN_AerialPauseCombo_Screen | AV_PRG_PauseComboAerial | PRG_BEN_AerialPauseCombo_desc |
| OBIWAN_FINISHER_STUN2 | PRG_BEN_FinisherStun2 | powerStunNPC,powerStunIGP | 2.0,1.0 | 4 | 0 | 0 | 0 | 1 | PRG_FinisherDuration_A_Screen | AV_PRG_FinisherDuration_A | PRG_BEN_FinisherStun2_desc |
| OBIWAN_PAUSECOMBO2 | PRG_BEN_PauseCombo2 | PauseComboYY_YY | 1 | 5 | 1 | 0 | 0 |  | PRG_BEN_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_BEN_PauseCombo2_desc |
| OBIWAN_FORCE_JUMP | PRG_BEN_ForceJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1.0,11.4,0.6,44,10,12 | 2 | 0 | 0 | 0 |  | PRG_BEN_ForceJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_BEN_ForceJump1_desc |
| OBIWAN_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Obiwan |  |
| OBIWAN_PAUSECOMBO1 | PRG_BEN_PauseCombo1 | PauseComboY_YY | 1 | 2 | 0 | 0 | 0 |  | PRG_BEN_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_BEN_PauseCombo1_desc |
| OBIWAN_PARRY | PRG_BEN_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_BEN_Parry_Screen | AV_PRG_RepelUnlock | PRG_BEN_Parry_desc |
| OBIWAN_AUTO_DEFLECT | PRG_BEN_AutoDeflect | DeflectBlasterBoltsFromIdle | 1 | 6 | 0 | 0 | 1 |  | PRG_BEN_AutoDeflect_Screen | AV_PRG_DeflectAuto | PRG_BEN_AutoDeflect_desc |
| OBIWAN_MELEE_DMG1 | PRG_BEN_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BEN_MeleeDmg1_desc |
| OBIWAN_SPECIAL_STUN2 | PRG_BEN_SpecialStun2 | powerDurationNPC,powerDurationIGP | 40.0,8.0 | 5 | 0 | 0 | 0 |  | PRG_SuperMoveDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_BEN_SpecialStun2_desc |
| OBIWAN_GRID3 | PRG_TCW_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
