# PSX_Grimm

`ProgressionTree = "IN3_PSX_Grimm"` -- source: `gamedb/core/in3_psx_grimm.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| GRIMM_FINISHER_COOLDOWN | finisherCooldown | 20 |
| GRIMM_BASEMELEE_DMG | meleeDamage | 1 |
| GRIMM_BLOCKBREAKER | blockBreakerCombo | 1 |
| GRIMM_BASESPECIAL_POWERDURATION | powerDuration,powerDurationOnPlayers | 7.5,3.0 |
| GRIMM_BASERANGED_DMG | rangedDamage | 1 |
| GRIMM_BASERUNSPEED | runspeed | 6.5 |
| GRIMM_BASEDEFLECT | DeflectBlasterBoltsFromBlock,DeflectBlasterBoltsFromParry,DeflectBlasterBoltsFromIdle,lightsaberRicochetChance,lightsaberPerfectRicochetChance | 1.0,0.0,0.0,100.0,0.0 |
| GRIMM_BASEPARRY | MeleeParry | 0 |
| GRIMM_BASE_HEALTH | health | 150 |
| GRIMM_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| GRIMM_BASESPECIAL_DMG | specialDamage | 1 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| GRIMM_MELEE_DMG3 | PRG_GRM_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_GRM_MeleeDmg3_desc |
| GRIMM_FORCE_SPECIAL4 | PRG_GRM_PowerDuration3 | powerMove,powerArea,powerDuration,powerDurationOnPlayers | 2,25.0,20.0,5.0 | 6 | 0 | 0 | 1 |  | PRG_SuperMoveUpgrade3_Screen | AV_PRG_SuperMoveUpgrade3 | PRG_GRM_PowerDuration3_desc |
| GRIMM_HEALTH4 | PRG_GRM_Health4 | health | 60 | 6 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_GRM_Health4_desc |
| GRIMM_FORCE_SPECIAL1 | PRG_GRM_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerMove,powerArea,powerDuration,powerDurationOnPlayers | 1.0,1.0,150.0,0,7.5,10.0,3.0 | 1 | 1 | 0 | 1 |  | psx_PRG_GRM_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_GRM_SpecialUnlocked_desc |
| GRIMM_PAUSECOMBO1 | PRG_GRM_PauseCombo1 | PauseComboY_YY | 1 | 2 | 1 | 0 | 0 |  | psx_PRG_GRM_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_GRM_PauseCombo1_desc |
| GRIMM_GRID1 | PRG_TCW_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| GRIMM_MELEE_DMG1 | PRG_GRM_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_GRM_MeleeDmg1_desc |
| GRIMM_HEALTH3 | PRG_GRM_Health3 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_GRM_Health3_desc |
| GRIMM_AERIALPAUSECOMBO | PRG_GRM_AerialPauseCombo | AerialPauseComboY_YY | 1 | 4 | 0 | 0 | 0 |  | psx_PRG_GRM_AerialPauseCombo_Screen | AV_PRG_PauseComboAerial | PRG_GRM_AerialPauseCombo_desc |
| GRIMM_FORCE_SPECIAL3 | PRG_GRM_PowerDuration2 | powerMove,powerArea,powerDuration,powerDurationOnPlayers | 2,20.0,20.0,4.0 | 4 | 0 | 0 | 1 |  | PRG_SuperMoveUpgrade2_Screen | AV_PRG_SuperMoveUpgrade2 | PRG_GRM_PowerDuration2_desc |
| GRIMM_FORCE_JUMP | PRG_GRM_ForceJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1.0,11.4,0.6,44,10,12 | 2 | 0 | 0 | 0 |  | psx_PRG_GRM_ForceJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_GRM_ForceJump1_desc |
| GRIMM_FORCE_SPECIAL_FINISHER | PRG_GRM_ForceFinisher2 | specialfinisher,finisherCooldown | 1,20.0 | 5 | 0 | 0 | 0 |  | PRG_ComboFinisherUpgrade1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_GRM_ForceFinisher2_desc |
| GRIMM_LAUNCH_COMBO | PRG_GRM_LauncherCombo | superStraightPunch | 1 | 3 | 0 | 0 | 0 |  | psx_PRG_GRM_LauncherCombo_Screen | AV_PRG_SuperPunchUnlock | PRG_GRM_LauncherCombo_desc |
| GRIMM_PAUSECOMBO2 | PRG_GRM_PauseCombo2 | PauseComboYY_YY | 1 | 5 | 0 | 0 | 0 |  | psx_PRG_GRM_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_GRM_PauseCombo2_desc |
| GRIMM_FORCE_SPECIAL2 | PRG_GRM_PowerDuration1 | powerMove,powerArea,powerDuration,powerDurationOnPlayers | 1,15.0,15.0,3.5 | 2 | 0 | 0 | 1 |  | PRG_SuperMoveUpgrade1_Screen | AV_PRG_SuperMoveUpgrade1 | PRG_GRM_PowerDuration1_desc |
| GRIMM_FORCE_FINISHER | PRG_GRM_ForceFinisher1 | finisherCombo,finisherCooldown | 1.0,25.0 | 3 | 0 | 0 | 1 |  | psx_PRG_GRM_ForceFinisher1_Screen | AV_PRG_ComboFinisherUnlock | PRG_GRM_ForceFinisher1_desc |
| GRIMM_AUTO_DEFLECT | PRG_GRM_AutoDeflect | DeflectBlasterBoltsFromIdle | 1 | 6 | 1 | 0 | 1 |  | psx_PRG_GRM_AutoDeflect_Screen | AV_PRG_DeflectAuto | PRG_GRM_AutoDeflect_desc |
| GRIMM_SPECIAL_DMG1 | PRG_GRM_SuperDmg1 | specialDamage | 0.2 | 3 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_GRM_SuperDmg1_desc |
| GRIMM_GRID2 | PRG_TCW_ForceGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| GRIMM_PARRY_COUNTER | PRG_GRM_ParryCounter | repelAttack | 1 | 3 | 0 | 0 | 0 |  | psx_PRG_GRM_ParryCounter_Screen | AV_PRG_ParryCounter | PRG_GRM_ParryCounter_desc |
| GRIMM_DASH_ATTACK | PRG_GRM_DashAttack | DashAttack | 1 | 1 | 0 | 0 | 0 |  | psx_PRG_GRM_DashAttack_Screen | AV_PRG_DashAttack | PRG_GRM_DashAttack_desc |
| GRIMM_HELMBREAKER_COMBO | PRG_GRM_GroundPound2 | HelmbreakerCombo_X | 1 | 3 | 0 | 0 | 0 |  | psx_PRG_GRM_GroundPound2_Screen | AV_CORE_Helmbreaker | PRG_GRM_GroundPound2_desc |
| GRIMM_GRID3 | PRG_TCW_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Saber |  |
| GRIMM_RANGED_DMG3 | PRG_GRM_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.4,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_GRM_RangedDmg3_desc |
| GRIMM_HEALTH2 | PRG_GRM_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_GRM_Health2_desc |
| GRIMM_RANGED_DMG2 | PRG_GRM_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.25,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_GRM_RangedDmg2_desc |
| GRIMM_RANGED_DMG1 | PRG_GRM_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.15,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_GRM_RangedDmg1_desc |
| GRIMM_TIMED_DEFLECT | PRG_GRM_TimedDeflect | DeflectBlasterBoltsFromParry | 1 | 4 | 0 | 0 | 0 |  | psx_PRG_GRM_TimedDeflect_Screen | AV_PRG_DeflectTimed | PRG_GRM_TimedDeflect_desc |
| GRIMM_GRID4 | PRG_TCW_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Telekinesis |  |
| GRIMM_HEALTH1 | PRG_GRM_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_GRM_Health1_desc |
| GRIMM_MELEE_DMG2 | PRG_GRM_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_GRM_MeleeDmg2_desc |
| GRIMM_POWERFILL1 | PRG_PowerFill1 | powerFill | 120 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| GRIMM_FORCE_PULL | PRG_GRM_ForcePull1 | forcePull | 1 | 1 | 1 | 0 | 0 |  | psx_PRG_GRM_ForcePull1_Screen | AV_PRG_ForcePull | PRG_GRM_ForcePull1_desc |
| GRIMM_PARRY | PRG_GRM_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_GRM_Parry_desc |
| GRIMM_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Grimm |  |
| GRIMM_SPECIAL_DMG2 | PRG_GRM_SuperDmg2 | specialDamage | 0.3 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_GRM_SuperDmg2_desc |
