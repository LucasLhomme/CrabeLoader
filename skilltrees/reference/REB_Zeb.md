# REB_Zeb

`ProgressionTree = "IN3_REB_Zeb"` -- source: `gamedb/core/in3_reb_zeb.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| ZEB_BASE_FINISHER | finisherCooldown | 20 |
| ZEB_BASE_HEALTH | health | 150 |
| ZEB_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| ZEB_BASE_FIRERATE | fireRate | 3.5 |
| ZEB_BASE_WALLCRAWL | BM_WallCrawler | 0 |
| ZEB_RANGEDCOMBO_YYRT | RangedComboYYRT | 1 |
| ZEB_CHARGEATTACK | blockBreakerCombo | 1 |
| ZEB_LAUNCHER | superStraightPunch | 1 |
| ZEB_BASE_CLIP | clipSize | 10 |
| ZEB_BASE_PARRY | MeleeParry | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ZEB_SPECIAL_DMG3 | PRG_ZEB_SuperDmg3 | specialDamage | 0.5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_ZEB_SuperDmg3_desc |
| ZEB_MELEE_DMG3 | PRG_ZEB_MeleeDmg3 | meleeHit,meleeDamage | 1,0.35 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ZEB_MeleeDmg3_desc |
| ZEB_CLIP1 | PRG_ZEB_BlasterClip1 | clipSize | 14 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_ZEB_BlasterClip1_desc |
| ZEB_ATHLETIC_JUMP1 | PRG_ZEB_AthleticJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 2 | 0 | 0 | 0 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_ZEB_AthleticJump1_desc |
| ZEB_POWERFILL1 | PRG_PowerFill1 | powerFill | 95 | 2 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| ZEB_SPECIAL_DURATION2 | PRG_ZEB_PowerDuration2 | powerDuration | 20 | 4 | 0 | 0 | 0 |  | PRG_SuperMoveDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_ZEB_PowerDuration2_desc |
| ZEB_HEALTH3 | PRG_ZEB_Health3 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_ZEB_Health3_desc |
| ZEB_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| ZEB_ATTACK_FINISHER2 | PRG_ZEB_ForceFinisher2 | finisherCooldown | 20 | 5 | 0 | 0 | 0 |  | PRG_ZEB_ForceFinisher1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_ZEB_ForceFinisher2_desc |
| ZEB_WALL_CRAWL | PRG_ZEB_WallCrawl1 | BM_WallCrawler,wallcrawl,wallcrawlJump | 1.0,1.0,1.0 | 3 | 0 | 0 | 1 |  | PRG_WallCrawlUnlock_Screen | AV_PRG_WallCrawlUnlock | PRG_ZEB_WallCrawl1_desc |
| ZEB_PAUSECOMBO2 | PRG_ZEB_PauseCombo2 | PauseComboYY_YY | 1 | 4 | 1 | 0 | 0 |  | PRG_ZEB_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_ZEB_PauseCombo2_desc |
| ZEB_MELEE_DMG2 | PRG_ZEB_MeleeDmg2 | meleeHit,meleeDamage | 1,0.2 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ZEB_MeleeDmg2_desc |
| ZEB_ATHLETIC_JUMP2 | PRG_ZEB_AthleticJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,44,10,12 | 4 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_ZEB_AthleticJump2_desc |
| ZEB_FIRERATE2 | PRG_ZEB_FireRate2 | fireRate | 6.5 | 4 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_ZEB_FireRate2_desc |
| ZEB_RANGED_DMG1 | PRG_ZEB_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.05,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ZEB_RangedDmg1_desc |
| ZEB_ATTACK_FINISHER1 | PRG_ZEB_ForceFinisher1 | finisherCombo,finisherCooldown | 1.0,25.0 | 2 | 1 | 0 | 1 |  | PRG_ZEB_ForceFinisher1_Screen | AV_PRG_ComboFinisherUnlock | PRG_ZEB_ForceFinisher1_desc |
| ZEB_FIRERATE1 | PRG_ZEB_FireRate1 | fireRate | 5 | 2 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_ZEB_FireRate1_desc |
| ZEB_PAUSECOMBO1 | PRG_ZEB_PauseCombo1 | PauseComboY_YY | 1 | 2 | 0 | 0 | 0 |  | PRG_ZEB_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_ZEB_PauseCombo1_desc |
| ZEB_FINISHER_DMG1 | PRG_ZEB_FinisherDmg1 | finisherDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_ZEB_FinisherDmg1_desc |
| ZEB_SPECIAL_DMG2 | PRG_ZEB_SuperDmg2 | specialDamage | 0.3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_ZEB_SuperDmg2_desc |
| ZEB_SPECIAL_UNLOCKED | PRG_ZEB_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDuration | 1.0,1.0,120.0,10.0 | 1 | 1 | 0 | 1 |  | PRG_ZEB_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_ZEB_SpecialUnlocked_desc |
| ZEB_FINISHER_DMG3 | PRG_ZEB_FinisherDmg3 | finisherDamage | 0.35 | 6 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_ZEB_FinisherDmg3_desc |
| ZEB_RANGED_DMG2 | PRG_ZEB_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.075,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ZEB_RangedDmg2_desc |
| ZEB_SPECIAL_DMG1 | PRG_ZEB_SuperDmg1 | specialDamage | 0.2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_ZEB_SuperDmg1_desc |
| ZEB_SPECIAL_DURATION1 | PRG_ZEB_PowerDuration1 | powerDuration | 15 | 2 | 0 | 0 | 0 |  | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_ZEB_PowerDuration1_desc |
| ZEB_HEALTH2 | PRG_ZEB_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_ZEB_Health2_desc |
| ZEB_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Zeb |  |
| ZEB_HEALTH1 | PRG_ZEB_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_ZEB_Health1_desc |
| ZEB_FINISHER_DMG2 | PRG_ZEB_FinisherDmg2 | finisherDamage | 0.2 | 3 | 0 | 0 | 0 | 1 | PRG_FinisherDamage_B_Screen | AV_PRG_FinisherDamage_B | PRG_ZEB_FinisherDmg2_desc |
| ZEB_PARRY | PRG_ZEB_Parry | MeleeParry | 1 | 2 | 1 | 0 | 0 |  | PRG_ZEB_Parry_Screen | AV_PRG_RepelUnlock | PRG_ZEB_Parry_desc |
| ZEB_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| ZEB_CLIP2 | PRG_ZEB_BlasterClip2 | clipSize | 20 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_ZEB_BlasterClip2_desc |
| ZEB_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| ZEB_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| ZEB_MELEE_DMG1 | PRG_ZEB_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ZEB_MeleeDmg1_desc |
| ZEB_RANGED_DMG3 | PRG_ZEB_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.125,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ZEB_RangedDmg3_desc |
