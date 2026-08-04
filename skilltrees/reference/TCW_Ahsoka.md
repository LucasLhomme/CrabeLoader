# TCW_Ahsoka

`ProgressionTree = "IN3_TCW_Ahsoka"` -- source: `gamedb/core/in3_tcw_ahsoka.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| AHSOKA_LAUNCHER | superStraightPunch | 1 |
| AHSOKA_LEDGE_OBB | ledgeMinY,ledgeMaxY,ledgeJumpMinY,ledgeJumpMaxY | 1.0,2.4,1.5,2.3 |
| AHSOKA_SHOTO_BASE | ahsokaShoto | 0 |
| AHSOKA_CHARGEATTACK | blockBreakerCombo | 1 |
| AHSOKA_BASE_HEALTHREGEN | healthRegen | 0 |
| AHSOKA_BASE_HEALTH | health | 150 |
| AHSOKA_BASEPARRY | MeleeParry | 0 |
| AHSOKA_BASE_DEFLECT_FROM_PARRY | DeflectBlasterBoltsFromParry | 0 |
| AHSOKA_BASE_FINISHER | finisherCombo,finisherCooldown | 0.0,30.0 |
| AHSOKA_BASE_FORCEPUSH | forcePush | 1 |
| AHSOKA_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| AHSOKA_BASE_RICOCHET_LIGHTSABER | lightsaberRicochetChance,lightsaberPerfectRicochetChance | 100,0 |
| AHSOKA_BASE_SPECIALCHARGE | specialChargeLevel | 1 |
| AHSOKA_BASE_DEFLECT_FROM_BLOCK | DeflectBlasterBoltsFromBlock | 1 |
| AHSOKA_BASE_BLOCK_RICOCHET_LIGHTSABER | lightsaberBlockRicochetChance, lightsaberPerfectBlockRicochetChance | 100,10 |
| AHSOKA_BASE_DEFLECT_FROM_IDLE | DeflectBlasterBoltsFromIdle | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| AHSOKA_GRID3 | PRG_TCW_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| AHSOKA_POWERBAR1 | PRG_PowerBar1 | powerBar | 2 | 3 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar1_desc |
| AHSOKA_AUTO_DEFLECT | PRG_AHS_AutoDeflect | DeflectBlasterBoltsFromIdle | 1 | 6 | 0 | 0 | 1 |  | PRG_AHS_AutoDeflect_Screen | AV_PRG_DeflectAuto | PRG_AHS_AutoDeflect_desc |
| AHSOKA_FORCE_SPECIAL_HOLD3 | PRG_AHS_ForceSpecial3 | specialChargeLevel | 1 | 5 | 0 | 0 | 1 | 1 | PRG_SuperMoveUpgrade1_Screen | AV_PRG_SuperMoveUpgrade1 | PRG_AHS_ForceSpecial3_desc |
| AHSOKA_PAUSECOMBO1 | PRG_AHS_PauseCombo1 | PauseComboY_YY | 1 | 2 | 1 | 0 | 0 |  | PRG_AHS_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_AHS_PauseCombo1_desc |
| AHSOKA_FORCE_SPECIAL_HOLD1 | PRG_AHS_ForceSpecial2 | specialUnlockedHold | 1 | 2 | 0 | 0 | 1 |  | PRG_SuperMoveUpgrade1_Screen | AV_PRG_SuperMoveUpgrade1 | PRG_AHS_ForceSpecial2_desc |
| AHSOKA_FORCE_FINISHER1 | PRG_AHS_ForceFinisher1 | finisherCombo,finisherCooldown | 1.0,25.0 | 2 | 1 | 0 | 1 |  | PRG_AHS_ForceFinisher1_Screen | AV_PRG_ComboFinisherUnlock | PRG_AHS_ForceFinisher1_desc |
| AHSOKA_GRID4 | PRG_TCW_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| AHSOKA_FINISHER_DMG3 | PRG_AHS_FinisherDmg3 | finisherDamage | 0.4 | 5 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_AHS_FinisherDmg3_desc |
| AHSOKA_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Ahsoka |  |
| AHSOKA_MELEE_DMG3 | PRG_AHS_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_AHS_MeleeDmg3_desc |
| AHSOKA_RANGED_DMG3 | PRG_AHS_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.4,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_AHS_RangedDmg3_desc |
| AHSOKA_POWERBAR2 | PRG_PowerBar2 | powerBar | 2 | 3 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar2_desc |
| AHSOKA_SPECIAL_DMG2 | PRG_AHS_SuperDmg2 | specialDamage | 0.3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_AHS_SuperDmg2_desc |
| AHSOKA_DASH_ATTACK | PRG_AHS_DashAttack | DashAttack | 1 | 1 | 0 | 0 | 0 |  | PRG_AHS_DashAttack_Screen | AV_PRG_DashAttack | PRG_AHS_DashAttack_desc |
| AHSOKA_AERIALPAUSECOMBO | PRG_AHS_PauseCombo3 | AerialPauseComboY_YY | 1 | 3 | 0 | 0 | 0 |  | PRG_AHS_PauseCombo3_Screen | AV_PRG_PauseComboAerial | PRG_AHS_PauseCombo3_desc |
| AHSOKA_MELEE_DMG1 | PRG_AHS_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_AHS_MeleeDmg1_desc |
| AHSOKA_SPECIAL_DMG3 | PRG_AHS_SuperDmg3 | specialDamage | 0.5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_AHS_SuperDmg3_desc |
| AHSOKA_GRID2 | PRG_TCW_ForceGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| AHSOKA_GRID1 | PRG_TCW_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| AHSOKA_TIMED_DEFLECT | PRG_AHS_TimedDeflect | DeflectBlasterBoltsFromParry | 1 | 4 | 1 | 0 | 0 |  | PRG_AHS_TimedDeflect_Screen | AV_PRG_DeflectTimed | PRG_AHS_TimedDeflect_desc |
| AHSOKA_HEALTH2 | PRG_AHS_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_AHS_Health2_desc |
| AHSOKA_MELEE_DMG2 | PRG_AHS_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_AHS_MeleeDmg2_desc |
| AHSOKA_FORCE_SPECIAL_HOLD2 | PRG_AHS_ForceSpecial3 | specialChargeLevel | 1 | 5 | 0 | 0 | 1 | 1 | PRG_SuperMoveUpgrade1_Screen | AV_PRG_SuperMoveUpgrade1 | PRG_AHS_ForceSpecial3_desc |
| AHSOKA_RANGED_DMG1 | PRG_AHS_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.15,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_AHS_RangedDmg1_desc |
| AHSOKA_HEALTH1 | PRG_AHS_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_AHS_Health1_desc |
| AHSOKA_FINISHER_DMG2 | PRG_AHS_FinisherDmg2 | finisherDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_AHS_FinisherDmg2_desc |
| AHSOKA_FINISHER_DMG1 | PRG_AHS_FinisherDmg1 | finisherDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_AHS_FinisherDmg1_desc |
| AHSOKA_SHOTO | PRG_AHS_Shoto | ahsokaShoto | 1 | 1 | 0 | 0 | 1 |  | PRG_MeleeShoto_Screen | AV_PRG_MeleeShoto | PRG_AHS_Shoto_desc |
| AHSOKA_RANGED_DMG2 | PRG_AHS_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.25,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_AHS_RangedDmg2_desc |
| AHSOKA_SPECIAL_DMG1 | PRG_AHS_SuperDmg1 | specialDamage | 0.2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_AHS_SuperDmg1_desc |
| AHSOKA_FORCE_SPECIAL1 | PRG_AHS_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,2.0,95.0 | 1 | 1 | 0 | 1 |  | PRG_AHS_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_AHS_SpecialUnlocked_desc |
| AHSOKA_FORCE_JUMP | PRG_AHS_ForceJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1.0,11.4,0.6,44,10,12 | 1 | 0 | 0 | 0 |  | PRG_AHS_ForceJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_AHS_ForceJump1_desc |
| AHSOKA_HEALTH3 | PRG_AHS_Health3 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_AHS_Health3_desc |
| AHSOKA_PAUSECOMBO2 | PRG_AHS_PauseCombo2 | PauseComboYY_YY | 1 | 5 | 0 | 0 | 0 |  | PRG_AHS_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_AHS_PauseCombo2_desc |
| AHSOKA_FORCE_FINISHER2 | PRG_AHS_ForceFinisher2 | finisherCooldown | 20 | 4 | 0 | 0 | 0 |  | PRG_ComboFinisherUpgrade1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_AHS_ForceFinisher2_desc |
| AHSOKA_PARRY | PRG_AHS_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_AHS_Parry_Screen | AV_PRG_RepelUnlock | PRG_AHS_Parry_desc |
| AHSOKA_POWERFILL1 | PRG_PowerFill1 | powerFill | 60 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
