# EMP_DarthVader

`ProgressionTree = "IN3_EMP_DarthVader"` -- source: `gamedb/core/in3_emp_darthvader.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| DARTHVADER_FORCEREPULSE | repulseLvl2,repulseLvl3,repulseLvl4,repulseMax,movementScale,drawSpeed,minDrawDist,rangeLvl1,rangeLvl2,rangeLvl3,rangeLvl4 | 0.4,1.2,2.0,2.5,0.25,180.0,2.0,10.0,15.0,20.0,30.0 |
| DARTHVADER_BASE_PARRY | MeleeParry | 0 |
| DARTHVADER_BASE_SPECIALCHARGE | chargeLevels | 1 |
| DARTHVADER_BASE_RICOCHET_LIGHTSABER | lightsaberRicochetChance,lightsaberPerfectRicochetChance | 100,0 |
| DARTHVADER_BASE_HEALTH | health | 150 |
| DARTHVADER_BASE_BLOCK_RICOCHET_LIGHTSABER | lightsaberBlockRicochetChance, lightsaberPerfectBlockRicochetChance | 100,10 |
| DARTHVADER_BASE_FORCEPULL | forcePull | 1 |
| DARTHVADER_LAUNCHER | superStraightPunch | 1 |
| DARTHVADER_BASE_FINISHER | finisherCombo,finisherCooldown | 0.0,20.0 |
| DARTHVADER_CHARGEATTACK | blockBreakerCombo | 1 |
| DARTHVADER_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| DARTHVADER_RUNSPEED | runspeed | 7 |
| DARTHVADER_BASE_DEFLECT | DeflectBlasterBoltsFromBlock,DeflectBlasterBoltsFromParry,DeflectBlasterBoltsFromIdlee | 1.0,0.0,0.0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| DARTHVADER_RANGED_DMG2 | PRG_VDR_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.15,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_VDR_RangedDmg2_desc |
| DARTHVADER_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_DarthVader |  |
| DARTHVADER_GRID4 | PRG_TCW_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| DARTHVADER_PAUSECOMBO1 | PRG_VDR_PauseCombo1 | PauseComboY_YY | 1 | 2 | 0 | 0 | 0 |  | PRG_VDR_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_VDR_PauseCombo1_desc |
| DARTHVADER_GRID2 | PRG_TCW_ForceGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| DARTHVADER_POWERFILL1 | PRG_PowerFill1 | powerFill | 60 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| DARTHVADER_MELEE_DMG1 | PRG_VDR_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VDR_MeleeDmg1_desc |
| DARTHVADER_RANGED_DMG1 | PRG_VDR_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.1,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_VDR_RangedDmg1_desc |
| DARTHVADER_FORCE_FINISHER2 | PRG_VDR_ForceFinisher2 | finisherCooldown | 20 | 4 | 0 | 0 | 0 |  | PRG_VDR_ForceFinisher1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_VDR_ForceFinisher2_desc |
| DARTHVADER_HEALTH3 | PRG_VDR_Health3 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_VDR_Health3_desc |
| DARTHVADER_POWERBAR1 | PRG_PowerBar1 | powerBar | 1 | 2 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar1_desc |
| DARTHVADER_AUTO_DEFLECT | PRG_VDR_AutoDeflect | DeflectBlasterBoltsFromIdle | 1 | 6 | 0 | 0 | 1 |  | PRG_VDR_AutoDeflect_Screen | AV_PRG_DeflectAuto | PRG_VDR_AutoDeflect_desc |
| DARTHVADER_GRID3 | PRG_TCW_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| DARTHVADER_GRID1 | PRG_TCW_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| DARTHVADER_PARRY | PRG_VDR_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_VDR_Parry_desc |
| DARTHVADER_POWERBAR3 | PRG_PowerBar3 | powerBar | 1 | 6 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar3_desc |
| DARTHVADER_SPECIAL_DMG1 | PRG_VDR_SuperDmg1 | specialDamage | 0.25 | 1 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_VDR_SuperDmg1_desc |
| DARTHVADER_POWERBAR2 | PRG_PowerBar2 | powerBar | 1 | 4 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar2_desc |
| DARTHVADER_FORCECHOKE_DAMAGE2 | PRG_VDR_FinisherDmg2 | forceChokeDamage | 450 | 4 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_VDR_FinisherDmg2_desc |
| DARTHVADER_FORCECHOKE_DAMAGE1 | PRG_VDR_FinisherDmg1 | forceChokeDamage | 250 | 2 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_VDR_FinisherDmg1_desc |
| DARTHVADER_FORCECHOKE_DAMAGE3 | PRG_VDR_FinisherDmg3 | forceChokeDamage | 775 | 6 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_VDR_FinisherDmg3_desc |
| DARTHVADER_SPECIAL_DMG2 | PRG_VDR_SuperDmg2 | specialDamage | 0.45 | 3 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_VDR_SuperDmg2_desc |
| DARTHVADER_MELEE_DMG3 | PRG_VDR_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VDR_MeleeDmg3_desc |
| DARTHVADER_FORCE_FINISHER1 | PRG_VDR_ForceFinisher1 | finisherCombo,finisherCooldown,forceChokeDamage | 1.0,25.0,1850 | 2 | 1 | 0 | 1 |  | PRG_VDR_ForceFinisher1_Screen | AV_PRG_ComboFinisherUnlock | PRG_VDR_ForceFinisher1_desc |
| DARTHVADER_SPECIAL_DMG3 | PRG_VDR_SuperDmg3 | specialDamage | 0.8 | 5 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_VDR_SuperDmg3_desc |
| DARTHVADER_SPECIAL_CHARGELEVEL1 | PRG_VDR_ForceSpecial2 | chargeLevels | 1 | 3 | 0 | 0 | 1 | 1 | PRG_SuperMeterChargeUnlock_Screen | AV_PRG_SuperMeterChargeUnlock | PRG_VDR_ForceSpecial2_desc |
| DARTHVADER_FORCE_SPECIAL1 | PRG_VDR_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,95.0 | 1 | 1 | 0 | 1 |  | PRG_VDR_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_VDR_SpecialUnlocked_desc |
| DARTHVADER_HEALTH1 | PRG_VDR_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_VDR_Health1_desc |
| DARTHVADER_TIMED_DEFLECT | PRG_VDR_TimedDeflect | DeflectBlasterBoltsFromParry | 1 | 4 | 1 | 0 | 0 |  | PRG_VDR_TimedDeflect_Screen | AV_PRG_DeflectTimed | PRG_VDR_TimedDeflect_desc |
| DARTHVADER_SPECIAL_CHARGELEVEL3 | PRG_VDR_ForceSpecial2 | chargeLevels | 1 | 6 | 0 | 0 | 1 | 1 | PRG_SuperMeterChargeUnlock_Screen | AV_PRG_SuperMeterChargeUnlock | PRG_VDR_ForceSpecial2_desc |
| DARTHVADER_PAUSECOMBO2 | PRG_VDR_PauseCombo2 | PauseComboYY_YY | 1 | 4 | 1 | 0 | 0 |  | PRG_VDR_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_VDR_PauseCombo2_desc |
| DARTHVADER_FORCE_JUMP | PRG_VDR_ForceJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,12,0.4,52,12,14 | 2 | 0 | 0 | 0 |  | PRG_VDR_ForceJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_VDR_ForceJump1_desc |
| DARTHVADER_SPECIAL_CHARGELEVEL2 | PRG_VDR_ForceSpecial2 | chargeLevels | 1 | 5 | 0 | 0 | 1 | 1 | PRG_SuperMeterChargeUnlock_Screen | AV_PRG_SuperMeterChargeUnlock | PRG_VDR_ForceSpecial2_desc |
| DARTHVADER_MELEE_DMG2 | PRG_VDR_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VDR_MeleeDmg2_desc |
| DARTHVADER_RANGED_DMG3 | PRG_VDR_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.25,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_VDR_RangedDmg3_desc |
| DARTHVADER_HEALTH2 | PRG_VDR_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_VDR_Health2_desc |
