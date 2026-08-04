# SPD_Spiderman

`ProgressionTree = "IN2_SPD_Spiderman"` -- source: `gamedb/core/in2_spd_spiderman.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| SPIDERMAN_BASEWALLCRAWL | wallCrawlSlowSpeed,wallCrawlRunSpeed,wallcrawlJump | 1.9,5.0,1.0 |
| SPIDERMAN_BASEHEALTHREGEN | healthRegen | 0 |
| SPIDERMAN_BASEHEALTH | health | 150 |
| SPIDERMAN_WEBBOLT_BASESTUN | webBoltStun,webStormStun | 1.5,5.0 |
| SPIDERMAN_BASESUPERJUMP | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ,SJDoStart | 11.4,0.6,48.9,10,12,1 |
| SPIDERMAN_WALLCRAWL1 | wallcrawl | 1 |
| SPIDERMAN_BASERUNSPEED | runspeed | 6.5 |
| SPIDERMAN_BASEREPEL | repel | 1 |
| SPIDERMAN_BASEFIRERATE | webFireRate | 2 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| SPIDERMAN_SWINGSPEED | PRG_SPD_SwingSpeed | swingSpeed | 1 | 5 | 0 | 0 | 0 |  | PRG_SPD_SwingSpeed_Screen | AV_PRG_WebSwingUpgrade1 | PRG_SPD_SwingSpeed_desc |
| SPIDERMAN_BLOCKBREAKER_COMBO | PRG_SPD_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 1 |  | PRG_SPD_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_SPD_BlockBreakerCombo_desc |
| SPIDERMAN_RUNSPEED2 | PRG_SPD_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_SPD_RunSpeed2_desc |
| SPIDERMAN_RANGED_DMG1 | PRG_SPD_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.5,0.75 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SPD_RangedDmg1_desc |
| SPIDERMAN_SPECIAL_UNLOCKED | PRG_SPD_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,60.0 | 2 | 1 | 0 | 1 |  | PRG_SPD_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_SPD_SpecialUnlocked_desc |
| SPIDERMAN_HEALTH4 | PRG_SPD_Health4 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SPD_Health4_desc |
| SPIDERMAN_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| SPIDERMAN_WEBLINE1 | PRG_SPD_WebLine1 | weblineAttack | 1 | 1 | 0 | 0 | 1 |  | PRG_SPD_WebLine1_Screen | AV_PRG_WebLine | PRG_SPD_WebLine1_desc |
| SPIDERMAN_RANGED_DMG2 | PRG_SPD_RangedDmg2 | rangedDamage,sharedRangedDamage | 1.0,1.0 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SPD_RangedDmg2_desc |
| SPIDERMAN_GROUNDCOMBO6 | PRG_SPD_GroundCombo6 | groundCombo6 | 1 | 4 | 0 | 0 | 0 |  | PRG_SPD_GroundCombo6_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_SPD_GroundCombo6_desc |
| SPIDERMAN_MELEE_DMG1 | PRG_SPD_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SPD_MeleeDmg1_desc |
| SPIDERMAN_SUPERJUMP1 | PRG_SPD_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,64,11.3,10 | 2 | 0 | 0 | 0 |  | PRG_SPD_SuperJump1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_SPD_SuperJump1_desc |
| SPIDERMAN_SPIDEY_SENSE | PRG_SPD_SpideySense | heightenedSense,BM_HeightenedSenses | 1,1 | 4 | 1 | 0 | 1 |  | PRG_SPD_SpideySense_Screen | AV_PRG_SpideySenseUnlock | PRG_SPD_SpideySense_desc |
| SPIDERMAN_WEBBOLT_STUN1 | PRG_SPD_WebBoltStun1 | webBoltStun,webStormStun | 3.0,8.0 | 2 | 0 | 0 | 0 |  | PRG_RangedStunTime1_Screen | AV_PRG_RangedStunTime1 | PRG_SPD_WebBoltStun1_desc |
| SPIDERMAN_MELEE_DMG3 | PRG_SPD_MeleeDmg3 | meleeDamage | 0.35 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SPD_MeleeDmg3_desc |
| SPIDERMAN_RUNSPEED1 | PRG_SPD_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_SPD_RunSpeed1_desc |
| SPIDERMAN_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| SPIDERMAN_POWERFILL1 | PRG_PowerFill1 | powerFill | 45 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| SPIDERMAN_SPECIAL_DMG1 | PRG_SPD_SuperDmg1 | specialDamage | 1 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_SPD_SuperDmg1_desc |
| SPIDERMAN_SPECIAL_DMG2 | PRG_SPD_SuperDmg2 | specialDamage | 1 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_SPD_SuperDmg2_desc |
| SPIDERMAN_FIRERATE1 | PRG_SPD_FireRate1 | webFireRate | 2.5 | 3 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_SPD_FireRate1_desc |
| SPIDERMAN_FIRERATE2 | PRG_SPD_FireRate2 | webFireRate | 3 | 5 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_SPD_FireRate2_desc |
| SPIDERMAN_DUALWEBBOLT | PRG_SPD_DualWebBolt | webBoltDual | 1 | 3 | 0 | 0 | 0 |  | PRG_SPD_DualWebBolt_Screen | AV_PRG_DualWebBoltUpgrade | PRG_SPD_DualWebBolt_desc |
| SPIDERMAN_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| SPIDERMAN_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Spiderman |  |
| SPIDERMAN_HEALTH1 | PRG_SPD_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SPD_Health1_desc |
| SPIDERMAN_MELEE_DMG2 | PRG_SPD_MeleeDmg2 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SPD_MeleeDmg2_desc |
| SPIDERMAN_WEBLINE3 | PRG_SPD_WebLine3 | weblineContextualFlurry | 1 | 6 | 0 | 0 | 1 |  | PRG_SPD_WebLine3_Screen | AV_PRG_RangedContextFlurry | PRG_SPD_WebLine3_desc |
| SPIDERMAN_GROUNDPOUND2 | PRG_SPD_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 0 | 0 | 0 |  | PRG_SPD_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_SPD_GroundPound2_desc |
| SPIDERMAN_HEALTH3 | PRG_SPD_Health3 | health | 30 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SPD_Health3_desc |
| SPIDERMAN_WEBBOLT_STUN2 | PRG_SPD_WebBoltStun2 | webBoltStun,webStormStun | 5,10.0 | 4 | 0 | 0 | 0 |  | PRG_RangedStunTime2_Screen | AV_PRG_RangedStunTime2 | PRG_SPD_WebBoltStun2_desc |
| SPIDERMAN_HEALTH2 | PRG_SPD_Health2 | health | 30 | 2 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SPD_Health2_desc |
| SPIDERMAN_WEBLINE2 | PRG_SPD_WebLine2 | weblineContextual | 1 | 4 | 1 | 0 | 0 |  | PRG_SPD_WebLine2_Screen | AV_PRG_RangedContextSend | PRG_SPD_WebLine2_desc |
| SPIDERMAN_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
