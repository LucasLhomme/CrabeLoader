# GOG_Groot

`ProgressionTree = "IN2_GOG_Groot"` -- source: `gamedb/core/in2_gog_groot.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| GROOT_BASEWALLCRAWL | BM_WallCrawler | 0 |
| GROOT_BASEHEALTH | health | 150 |
| GROOT_BASERUNSPEED | runspeed | 8 |
| GROOT_BASEHEALTHREGEN | healthRegen | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| GROOT_BLOCKBREAKER_COMBO | PRG_GRT_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 0 |  | PRG_GRT_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_GRT_BlockBreakerCombo_desc |
| GROOT_MELEE_DMG2 | PRG_GRT_MeleeDmg2 | meleeDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_GRT_MeleeDmg2_desc |
| GROOT_WALLCRAWL1 | PRG_GRT_WallCrawl1 | BM_WallCrawler,wallcrawl,wallcrawlJump | 1.0,1.0,1.0 | 1 | 0 | 0 | 1 |  | PRG_GRT_WallCrawl1_Screen | AV_PRG_WallCrawlUnlock | PRG_GRT_WallCrawl1_desc |
| GROOT_SUPER_DMG3 | PRG_GRT_SuperDmg3 | specialDamage | 0.75 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_GRT_SuperDmg3_desc |
| GROOT_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| GROOT_MELEE_DMG3 | PRG_GRT_MeleeDmg3 | meleeDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_GRT_MeleeDmg3_desc |
| GROOT_RUNSPEED1 | PRG_GRT_RunSpeed1 | runspeed | 9.5 | 2 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_GRT_RunSpeed1_desc |
| GROOT_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Groot |  |
| GROOT_MELEE_DMG1 | PRG_GRT_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_GRT_MeleeDmg1_desc |
| GROOT_SPECIAL_HEALTHREGEN | PRG_GRT_SpecialHealthRegen | blockShieldRegen,blockShieldDelay | 20.0,1.0 | 6 | 1 | 0 | 0 |  | PRG_GRT_SpecialHealthRegen_Screen | AV_PRG_SuperMoveUpgrade2 | PRG_GRT_SpecialHealthRegen_desc |
| GROOT_GROUNDPOUND2 | PRG_GRT_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_GRT_GroundPound2_desc |
| GROOT_SUPER_DMG1 | PRG_GRT_SuperDmg1 | specialDamage | 0.3 | 3 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_GRT_SuperDmg1_desc |
| GROOT_HEALTH4 | PRG_GRT_Health4 | health | 25 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_GRT_Health4_desc |
| GROOT_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| GROOT_SUPER_STRAIGHT_PUNCH | PRG_GRT_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 0 |  | PRG_GRT_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_GRT_SuperStraightPunch_desc |
| GROOT_RANGED_DMG3 | PRG_GRT_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.5,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_GRT_RangedDmg3_desc |
| GROOT_ARM_AOE | PRG_GRT_ArmAOE | armAttakAOE | 1 | 4 | 0 | 0 | 1 |  | PRG_GRT_ArmAOE_Screen | AV_PRG_RangedChargeUp | PRG_GRT_ArmAOE_desc |
| GROOT_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| GROOT_RANGED_DMG2 | PRG_GRT_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.3,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_GRT_RangedDmg2_desc |
| GROOT_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| GROOT_HEALTH2 | PRG_GRT_Health2 | health | 25 | 2 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_GRT_Health2_desc |
| GROOT_SPECIAL_BARKSHIELD | PRG_GRT_SpecialBarkShield | shieldArmor | 75 | 3 | 1 | 0 | 0 |  | PRG_GRT_SpecialBarkShield_Screen | AV_PRG_SuperMoveUpgrade1 | PRG_GRT_SpecialBarkShield_desc |
| GROOT_HEALTH3 | PRG_GRT_Health3 | health | 25 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_GRT_Health3_desc |
| GROOT_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| GROOT_SPECIAL_UNLOCKED | PRG_GRT_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 2 | 1 | 0 | 1 |  | PRG_GRT_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_GRT_SpecialUnlocked_desc |
| GROOT_HEALTH5 | PRG_GRT_Health5 | health | 50 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_GRT_Health5_desc |
| GROOT_SUPER_DMG2 | PRG_GRT_SuperDmg2 | specialDamage | 0.45 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_GRT_SuperDmg2_desc |
| GROOT_RANGED_DMG1 | PRG_GRT_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.2,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_GRT_RangedDmg1_desc |
| GROOT_HEALTH1 | PRG_GRT_Health1 | health | 25 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_GRT_Health1_desc |
| GROOT_GROUNDCOMBO5 | PRG_GRT_GroundCombo5 | groundCombo5 | 1 | 4 | 0 | 0 | 1 |  | PRG_GRT_GroundCombo5_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_GRT_GroundCombo5_desc |
