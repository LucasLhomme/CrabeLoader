# EMP_BobaFett

`ProgressionTree = "IN3_EMP_BobaFett"` -- source: `gamedb/core/in3_emp_bobafett.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| BOBAFETT_BASE_CLIP | clipSize | 10 |
| BOBAFETT_BASE_FIRERATE | fireRate | 3 |
| BOBAFETT_BASE_SUPERJUMP | BM_SuperJumper,BM_Flier | 0,0 |
| BOBAFETT_BASE_HEALTH | health | 150 |
| BOBAFETT_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.6 |
| BOBAFETT_BASEPARRY | MeleeParry | 0 |
| BOBAFETT_BASE_GLIDE | floatspeed,floatgravity | 5.0,2.0 |
| BOBAFETT_FINISHER_COOLDOWN | finisherCooldown | 20 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| BOBAFETT_MELEE_DMG1 | PRG_BFT_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BFT_MeleeDmg1_desc |
| BOBAFETT_RANGED_DMG1 | PRG_BFT_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.05,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BFT_RangedDmg1_desc |
| BOBAFETT_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| BOBAFETT_MELEE_DMG2 | PRG_BFT_MeleeDmg2 | meleeHit,meleeDamage | 1,0.2 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BFT_MeleeDmg2_desc |
| BOBAFETT_FIRERATE2 | PRG_BFT_FireRate2 | fireRate | 4 | 4 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_BFT_FireRate2_desc |
| BOBAFETT_PAUSE_COMBO2 | PRG_BFT_PauseCombo2 | PauseComboYY_YY | 1 | 6 | 0 | 0 | 0 |  | PRG_BFT_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_BFT_PauseCombo2_desc |
| BOBAFETT_SPECIAL_DMG1 | PRG_BFT_SuperDmg1 | specialDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_BFT_SuperDmg1_desc |
| BOBAFETT_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| BOBAFETT_RANGED_DMG2 | PRG_BFT_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.075,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BFT_RangedDmg2_desc |
| BOBAFETT_SPECIAL_UNLOCKED | PRG_BFT_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 1 | 1 | 0 | 1 |  | PRG_BFT_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_BFT_SpecialUnlocked_desc |
| BOBAFETT_LAUNCHER_FOLLOW | PRG_BFT_LauncherFollow | launcherFollow | 1 | 3 | 1 | 0 | 1 |  | PRG_BFT_LauncherFollow_Screen | AV_PRG_SuperPunchUnlock | PRG_BFT_LauncherFollow_desc |
| BOBAFETT_CLIP1 | PRG_BFT_BlasterClip1 | clipSize | 12 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_BFT_BlasterClip1_desc |
| BOBAFETT_SPECIAL_DMG2 | PRG_BFT_SuperDmg2 | specialDamage | 0.15 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_BFT_SuperDmg2_desc |
| BOBAFETT_MELEE_DMG3 | PRG_BFT_MeleeDmg3 | meleeHit,meleeDamage | 1,0.35 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BFT_MeleeDmg3_desc |
| BOBAFETT_RANGED_DMG3 | PRG_BFT_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.125,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BFT_RangedDmg3_desc |
| BOBAFETT_JET_PACK | PRG_BFT_JetPack | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ,BM_Flier,flightSpeed,maxFlightSpeed,hoverSpeed,BM_SuperJumper | 11.4,0.6,44,10,12,1,24.00,31.00,10.00,0 | 2 | 1 | 0 | 0 |  | PRG_BFT_JetPack_Screen | AV_PRG_FlightJetPack | PRG_BFT_JetPack_desc |
| BOBAFETT_PARRY | PRG_BFT_Parry | MeleeParry | 1 | 3 | 0 | 0 | 0 |  | PRG_BFT_Parry_Screen | AV_PRG_RepelUnlock | PRG_BFT_Parry_desc |
| BOBAFETT_PAUSE_COMBO1 | PRG_BFT_PauseCombo1 | PauseComboY_YY | 1 | 2 | 0 | 0 | 0 |  | PRG_BFT_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_BFT_PauseCombo1_desc |
| BOBAFETT_COMBO_FINISHER1 | PRG_BFT_ForceFinisher1 | finisherCombo,finisherCooldown | 1.0,20.0 | 4 | 0 | 0 | 1 |  | PRG_BFT_ForceFinisher1_Screen | AV_PRG_ComboFinisherUnlock | PRG_BFT_ForceFinisher1_desc |
| BOBAFETT_SPECIAL_DMG3 | PRG_BFT_SuperDmg3 | specialDamage | 0.25 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_BFT_SuperDmg3_desc |
| BOBAFETT_CLIP2 | PRG_BFT_BlasterClip2 | clipSize | 16 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_BFT_BlasterClip2_desc |
| BOBAFETT_GRAPPLE_HOOK | PRG_BFT_GrappleHook | forcePull | 1 | 1 | 1 | 0 | 1 |  | PRG_BFT_GrappleHook_Screen | AV_PRG_ForcePull | PRG_BFT_GrappleHook_desc |
| BOBAFETT_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_BobaFett |  |
| BOBAFETT_PAUSE_COMBO3 | PRG_BFT_AerialPauseCombo | AirComboYY_Y, AerialPauseComboY_YY | 1, 1 | 5 | 0 | 0 | 0 |  | PRG_BFT_AerialPauseCombo_Screen | AV_PRG_PauseComboAerial | PRG_BFT_AerialPauseCombo_desc |
| BOBAFETT_PARRY_COUNTER | PRG_BFT_ParryCounter | repelAttack | 1 | 5 | 0 | 0 | 0 |  | PRG_BFT_ParryCounter_Screen | AV_PRG_ParryCounter | PRG_BFT_ParryCounter_desc |
| BOBAFETT_IMP_SPECIAL | PRG_BFT_ImpSpecial | specialUpgrade1 | 1 | 4 | 0 | 0 | 1 |  | PRG_SuperMeterChargeUpgrade1_Screen | AV_PRG_SuperMeterChargeUpgrade1 | PRG_BFT_ImpSpecial_desc |
| BOBAFETT_POWERFILL1 | PRG_PowerFill1 | powerFill | 95 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| BOBAFETT_HEALTH1 | PRG_BFT_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_BFT_Health1_desc |
| BOBAFETT_HEALTH2 | PRG_BFT_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_BFT_Health2_desc |
| BOBAFETT_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| BOBAFETT_FIRERATE1 | PRG_BFT_FireRate1 | fireRate | 3.5 | 2 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_BFT_FireRate1_desc |
| BOBAFETT_COMBO_FINISHER2 | PRG_BFT_ForceFinisher2 | finisherCooldown | 15 | 6 | 0 | 0 | 0 |  | PRG_BFT_ForceFinisher1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_BFT_ForceFinisher2_desc |
| BOBAFETT_HEALTH3 | PRG_BFT_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_BFT_Health3_desc |
| BOBAFETT_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
