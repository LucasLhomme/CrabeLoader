# GOG_Drax

`ProgressionTree = "IN2_GOG_Drax"` -- source: `gamedb/core/in2_gog_drax.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| DRAX_BASERUNSPEED | runspeed | 7 |
| DRAX_BASESTRENGTH | BM_SuperDuperStrong | 0 |
| DRAX_BASEHEALTHREGEN | healthRegen | 0 |
| DRAX_CHARGE1 | shoulderCharge | 1 |
| DRAX_BASEHEALTH | health | 150 |
| DRAX_BASEWALLCRAWL | BM_WallCrawler,wallCrawlSlowSpeed,wallCrawlRunSpeed | 0.0,2.0,5.0 |
| DRAX_POWERDURATION | powerDuration | 10 |
| DRAX_BASESUPERJUMP | BM_SuperJumper | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| DRAX_HEALTH2 | PRG_DRX_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_DRX_Health2_desc |
| DRAX_HEALTHREGEN1 | PRG_DRX_HealthRegen1 | healthRegen | 2 | 3 | 0 | 0 | 0 |  | PRG_HealthRegen1_Screen | AV_PRG_RegenUnlock | PRG_DRX_HealthRegen1_desc |
| DRAX_POWERDURATION2 | PRG_DRX_PowerDuration2 | powerDuration | 25 | 5 | 0 | 0 | 0 |  | PRG_SuperMoveDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_DRX_PowerDuration2_desc |
| DRAX_CHARGE3 | PRG_DRX_Charge3 | shoulderChargeFinisher | 1 | 4 | 0 | 0 | 0 |  | PRG_DRX_RangedDMG2_Screen | AV_PRG_RangedUpgrade3 | PRG_DRX_Charge3_desc |
| DRAX_HEALTH1 | PRG_DRX_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_DRX_Health1_desc |
| DRAX_WALLCRAWL1 | PRG_DRX_WallCrawl1 | BM_WallCrawler,wallcrawl,wallcrawlJump | 1.0,1.0,1.0 | 1 | 0 | 0 | 1 |  | PRG_DRX_WallCrawl1_Screen | AV_PRG_WallCrawlUnlock | PRG_DRX_WallCrawl1_desc |
| DRAX_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| DRAX_SUPER_STRAIGHT_PUNCH | PRG_DRX_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 0 |  | PRG_DRX_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_DRX_SuperStraightPunch_desc |
| DRAX_SPECIAL_UNLOCKED | PRG_DRX_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDuration | 1.0,1.0,120.0,15.0 | 2 | 1 | 0 | 1 |  | PRG_DRX_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_DRX_SpecialUnlocked_desc |
| DRAX_SUPER_DMG2 | PRG_DRX_SuperDmg2 | specialDamage | 0.3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_DRX_SuperDmg2_desc |
| DRAX_MELEE_DMG2 | PRG_DRX_MeleeDmg2 | meleeDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DRX_MeleeDmg2_desc |
| DRAX_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| DRAX_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| DRAX_MELEE_DMG3 | PRG_DRX_MeleeDmg3 | meleeDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DRX_MeleeDmg3_desc |
| DRAX_RUNSPEED1 | PRG_DRX_RunSpeed1 | runspeed | 8.5 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_DRX_RunSpeed1_desc |
| DRAX_MELEE_DMG1 | PRG_DRX_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DRX_MeleeDmg1_desc |
| DRAX_HEALTHREGEN2 | PRG_DRX_HealthRegen2 | healthRegen | 5 | 6 | 0 | 0 | 0 |  | PRG_HealthRegen2_Screen | AV_PRG_RegenUpgrade1 | PRG_DRX_HealthRegen2_desc |
| DRAX_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| DRAX_RANGED_DMG3 | PRG_DRX_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.5,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DRX_RangedDmg3_desc |
| DRAX_SUPERJUMP2 | PRG_DRX_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,42.5,10,12 | 5 | 0 | 0 | 0 |  | PRG_DRX_SuperJump1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_DRX_SuperJump2_desc |
| DRAX_SUPERJUMP1 | PRG_DRX_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 2 | 0 | 0 | 0 |  | PRG_DRX_SuperJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_DRX_SuperJump1_desc |
| DRAX_GROUNDCOMBO6 | PRG_DRX_GroundCombo6 | groundCombo6 | 1 | 4 | 0 | 0 | 0 |  | PRG_DRX_GroundCombo6_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_DRX_GroundCombo6_desc |
| DRAX_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Drax |  |
| DRAX_SUPER_DMG3 | PRG_DRX_SuperDmg3 | specialDamage | 0.5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_DRX_SuperDmg3_desc |
| DRAX_RANGED_DMG2 | PRG_DRX_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.3,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DRX_RangedDmg2_desc |
| DRAX_CHARGE2 | PRG_DRX_Charge2 | shoulderChargeSustained | 1 | 2 | 0 | 0 | 1 |  | PRG_DRX_RangedDMG1_Screen | AV_PRG_RangedChargeUp | PRG_DRX_Charge2_desc |
| DRAX_BLOCKBREAKER_COMBO | PRG_DRX_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 0 |  | PRG_DRX_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_DRX_BlockBreakerCombo_desc |
| DRAX_RANGED_DMG1 | PRG_DRX_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.2,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DRX_RangedDmg1_desc |
| DRAX_POWERDURATION1 | PRG_DRX_PowerDuration1 | powerDuration | 20 | 3 | 0 | 0 | 0 |  | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_DRX_PowerDuration1_desc |
| DRAX_SUPER_DMG1 | PRG_DRX_SuperDmg1 | specialDamage | 0.2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_DRX_SuperDmg1_desc |
| DRAX_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| DRAX_HEALTH3 | PRG_DRX_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_DRX_Health3_desc |
| DRAX_STRENGTH | PRG_DRX_Strength | BM_SuperDuperStrong | 1 | 3 | 0 | 0 | 0 |  | PRG_StrengthUpgrade_Screen | AV_PRG_PickupThrowUpgrade1 | PRG_DRX_Strength_desc |
| DRAX_GROUNDPOUND2 | PRG_DRX_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 1 | 0 | 0 |  | PRG_DRX_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_DRX_GroundPound2_desc |
