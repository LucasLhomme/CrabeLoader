# SPD_Venom

`ProgressionTree = "IN2_SPD_Venom"` -- source: `gamedb/core/in2_spd_venom.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| VENOM_BASESUPERJUMP | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ,SJDoStart | 11.4,0.6,48.9,10,12,1 |
| VENOM_WALLCRAWL1 | wallcrawl | 1 |
| VENOM_BASEWEBSWING | BM_WebSwinger | 0 |
| VENOM_BASEHEALTH | health | 150 |
| VENOM_BASEWALLCRAWL | wallCrawlSlowSpeed,wallCrawlRunSpeed,wallcrawlJump | 3.0,7.0,1.0 |
| VENOM_BASERUNSPEED | runspeed | 8 |
| VENOM_BASEFIRERATE | webFireRate | 3 |
| VENOM_BASEHEALTHREGEN | healthRegen | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| VENOM_WEBLINE_STUN1 | PRG_VNM_WebLineStun1 | webBoltStun | 1 | 3 | 0 | 0 | 0 |  | PRG_RangedStunTime1_Screen | AV_PRG_RangedStunTime1 | PRG_VNM_WebLineStun1_desc |
| VENOM_SUPER_STRAIGHT_PUNCH | PRG_VNM_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 0 |  | PRG_VNM_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_VNM_SuperStraightPunch_desc |
| VENOM_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| VENOM_WEBSWING | PRG_VNM_WebSwing | webSwing,BM_WebSwinger | 1,1 | 1 | 1 | 0 | 1 |  | PRG_VNM_WebSwing_Screen | AV_PRG_WebSwingUnlock | PRG_VNM_WebSwing_desc |
| VENOM_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| VENOM_MELEE_DMG3 | PRG_VNM_MeleeDmg3 | meleeDamage | 0.35 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VNM_MeleeDmg3_desc |
| VENOM_HEALTHREGEN1 | PRG_VNM_HealthRegen1 | healthRegen | 2 | 3 | 0 | 0 | 0 |  | PRG_HealthRegen1_Screen | AV_PRG_RegenUnlock | PRG_VNM_HealthRegen1_desc |
| VENOM_MELEE_DMG2 | PRG_VNM_MeleeDmg2 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VNM_MeleeDmg2_desc |
| VENOM_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Venom |  |
| VENOM_FIRERATE1 | PRG_VNM_FireRate1 | webFireRate | 3.5 | 3 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_VNM_FireRate1_desc |
| VENOM_SWINGSPEED | PRG_VNM_SwingSpeed | swingSpeed | 1 | 4 | 0 | 0 | 0 |  | PRG_SPD_SwingSpeed_Screen | AV_PRG_WebSwingUpgrade1 | PRG_VNM_SwingSpeed_desc |
| VENOM_RANGED_DMG2 | PRG_VNM_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.3,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_VNM_RangedDmg2_desc |
| VENOM_GROUNDPOUND2 | PRG_VNM_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_VNM_GroundPound2_desc |
| VENOM_SPECIAL_UNLOCKED | PRG_VNM_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 2 | 1 | 0 | 1 |  | PRG_VNM_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_VNM_SpecialUnlocked_desc |
| VENOM_WEBLINE2 | PRG_VNM_WebLine2 | weblineContextualFlurry | 1 | 4 | 0 | 0 | 0 |  | PRG_VNM_WebLine2_Screen | AV_PRG_RangedContextSend | PRG_VNM_WebLine2_desc |
| VENOM_SUPER_DMG2 | PRG_VNM_SuperDmg2 | specialDamage | 0.65 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_VNM_SuperDmg2_desc |
| VENOM_HEALTH2 | PRG_VNM_Health2 | health | 100 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_VNM_Health2_desc |
| VENOM_SUPERJUMP1 | PRG_VNM_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,61,11.3,10 | 2 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_VNM_SuperJump1_desc |
| VENOM_GROUNDCOMBO5 | PRG_VNM_GroundCombo5 | groundCombo5 | 1 | 4 | 1 | 0 | 1 |  | PRG_VNM_GroundCombo5_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_VNM_GroundCombo5_desc |
| VENOM_RUNSPEED2 | PRG_VNM_RunSpeed2 | runspeed | 10.25 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_VNM_RunSpeed2_desc |
| VENOM_RUNSPEED1 | PRG_VNM_RunSpeed1 | runspeed | 9.75 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_VNM_RunSpeed1_desc |
| VENOM_VENOM_SENSE | PRG_VNM_VenomSense | repel,heightenedSense,BM_HeightenedSenses | 1.0,1.0,1 | 4 | 0 | 0 | 0 |  | PRG_VNM_VenomSense_Screen | AV_PRG_SpideySenseUnlock | PRG_VNM_VenomSense_desc |
| VENOM_MELEE_DMG1 | PRG_VNM_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VNM_MeleeDmg1_desc |
| VENOM_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| VENOM_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| VENOM_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| VENOM_SUPER_DMG1 | PRG_VNM_SuperDmg1 | specialDamage | 0.35 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_VNM_SuperDmg1_desc |
| VENOM_RANGED_DMG3 | PRG_VNM_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.5,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_VNM_RangedDmg3_desc |
| VENOM_RANGED_DMG1 | PRG_VNM_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.2,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_VNM_RangedDmg1_desc |
| VENOM_HEALTH1 | PRG_VNM_Health1 | health | 50 | 2 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_VNM_Health1_desc |
| VENOM_HEALTHREGEN2 | PRG_VNM_HealthRegen2 | healthRegen | 5 | 6 | 0 | 0 | 0 |  | PRG_HealthRegen2_Screen | AV_PRG_RegenUpgrade1 | PRG_VNM_HealthRegen2_desc |
| VENOM_WEBLINE1 | PRG_VNM_WebLine1 | weblineAttack | 1 | 1 | 0 | 0 | 1 |  | PRG_VNM_WebLine1_Screen | AV_PRG_WebLine | PRG_VNM_WebLine1_desc |
| VENOM_SUPER_DMG3 | PRG_VNM_SuperDmg3 | specialDamage | 1 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_VNM_SuperDmg3_desc |
| VENOM_BLOCKBREAKER_COMBO | PRG_VNM_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 0 |  | PRG_VNM_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_VNM_BlockBreakerCombo_desc |
