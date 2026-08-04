# AVG_Hulk

`ProgressionTree = "IN2_AVG_Hulk"` -- source: `gamedb/core/in2_avg_hulk.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| HULK_CHARGE1 | shoulderCharge | 1 |
| HULK_BASEHEALTH | health | 150 |
| HULK_BASERUNSPEED | runspeed | 8 |
| HULK_BASESUPERJUMP | BM_SuperJumper | 0 |
| HULK_BASEWALLCRAWL | BM_WallCrawler,wallCrawlSlowSpeed,wallCrawlRunSpeed | 0.0,3.0,7.0 |
| HULK_BASEHEALTHREGEN | healthRegen | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| HULK_SUPER_DMG1 | PRG_HLK_SuperDmg1 | specialDamage | 0.3 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_HLK_SuperDmg1_desc |
| HULK_RANGED_DMG2 | PRG_HLK_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.3,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HLK_RangedDmg2_desc |
| HULK_MELEE_DMG3 | PRG_HLK_MeleeDmg3 | meleeDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HLK_MeleeDmg3_desc |
| HULK_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Hulk |  |
| HULK_MELEE_DMG2 | PRG_HLK_MeleeDmg2 | meleeDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HLK_MeleeDmg2_desc |
| HULK_CHARGE2 | PRG_HLK_Charge2 | shoulderChargeSustained | 1 | 3 | 0 | 0 | 1 |  | PRG_HLK_Charge2_Screen | AV_PRG_RangedChargeUp | PRG_HLK_Charge2_desc |
| HULK_SUPER_DMG2 | PRG_HLK_SuperDmg2 | specialDamage | 0.45 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_HLK_SuperDmg2_desc |
| HULK_POWERAREA1 | PRG_HLK_PowerArea1 | powerArea | 12 | 3 | 0 | 0 | 0 |  | PRG_SuperMoveUpgrade1_Screen | AV_PRG_SuperMoveUpgrade1 | PRG_HLK_PowerArea1_desc |
| HULK_CHARGE3 | PRG_HLK_Charge3 | shoulderChargeFinisher | 1 | 5 | 0 | 0 | 0 |  | PRG_HLK_Charge3_Screen | AV_PRG_RangedUpgrade1 | PRG_HLK_Charge3_desc |
| HULK_WALLCRAWL1 | PRG_HLK_WallCrawl1 | BM_WallCrawler,wallcrawl,wallcrawlJump | 1.0,1.0,1.0 | 3 | 1 | 0 | 1 |  | PRG_HLK_WallCrawl1_Screen | AV_PRG_WallCrawlUnlock | PRG_HLK_WallCrawl1_desc |
| HULK_GROUNDPOUND2 | PRG_HLK_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 2 | 0 | 0 | 0 |  | PRG_HLK_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_HLK_GroundPound2_desc |
| HULK_RANGED_DMG1 | PRG_HLK_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.2,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HLK_RangedDmg1_desc |
| HULK_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| HULK_RUNSPEED1 | PRG_HLK_RunSpeed1 | runspeed | 9.75 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_HLK_RunSpeed1_desc |
| HULK_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| HULK_SPECIAL_UNLOCKED | PRG_HLK_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,200.0 | 2 | 1 | 0 | 1 |  | PRG_HLK_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_HLK_SpecialUnlocked_desc |
| HULK_SUPER_DMG3 | PRG_HLK_SuperDmg3 | specialDamage | 0.75 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_HLK_SuperDmg3_desc |
| HULK_HEALTHREGEN2 | PRG_HLK_HealthRegen2 | healthRegen | 5 | 6 | 0 | 0 | 0 |  | PRG_HealthRegen2_Screen | AV_PRG_RegenUpgrade1 | PRG_HLK_HealthRegen2_desc |
| HULK_MELEE_DMG1 | PRG_HLK_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HLK_MeleeDmg1_desc |
| HULK_RUNSPEED2 | PRG_HLK_RunSpeed2 | runspeed | 10.25 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_HLK_RunSpeed2_desc |
| HULK_BLOCKBREAKER_COMBO | PRG_HLK_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 0 |  | PRG_HLK_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_HLK_BlockBreakerCombo_desc |
| HULK_POWERFILL1 | PRG_PowerFill1 | powerFill | 150 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| HULK_SUPERJUMP2 | PRG_HLK_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,64,11.3,10 | 3 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_HLK_SuperJump2_desc |
| HULK_HEALTH3 | PRG_HLK_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_HLK_Health3_desc |
| HULK_SUPER_STRAIGHT_PUNCH | PRG_HLK_SuperStraightPunch | superStraightPunch | 1 | 4 | 0 | 0 | 0 |  | PRG_HLK_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_HLK_SuperStraightPunch_desc |
| HULK_SUPERJUMP1 | PRG_HLK_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,44,10,12 | 1 | 1 | 0 | 0 |  | PRG_HLK_SuperJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_HLK_SuperJump1_desc |
| HULK_GROUNDCOMBO5 | PRG_HLK_GroundCombo5 | groundCombo5 | 1 | 4 | 1 | 0 | 1 |  | PRG_HLK_GroundCombo5_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_HLK_GroundCombo5_desc |
| HULK_SUPERJUMP3 | PRG_HLK_SuperJump3 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,82,15,18 | 5 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade2_Screen | AV_PRG_SuperJumpUpgrade2 | PRG_HLK_SuperJump3_desc |
| HULK_HEALTHREGEN1 | PRG_HLK_HealthRegen1 | healthRegen | 2 | 4 | 0 | 0 | 1 |  | PRG_HealthRegen1_Screen | AV_PRG_RegenUnlock | PRG_HLK_HealthRegen1_desc |
| HULK_HEALTH2 | PRG_HLK_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_HLK_Health2_desc |
| HULK_RANGED_DMG3 | PRG_HLK_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.5,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HLK_RangedDmg3_desc |
| HULK_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| HULK_HEALTH1 | PRG_HLK_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_HLK_Health1_desc |
| HULK_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
