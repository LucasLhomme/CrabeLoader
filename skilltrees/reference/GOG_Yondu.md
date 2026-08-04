# GOG_Yondu

`ProgressionTree = "IN2_GOG_Yondu"` -- source: `gamedb/core/in2_gog_yondu.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| YONDY_BASE_ARROWSPRAY_MAX_FOV | arrowSprayMaxFOV | 45 |
| YONDU_BASE_ARROWSPRAY_SPAWNOFFSET_Y | arrowSpawnOffsetY | 0 |
| YONDU_BASE_HEALTH | health | 150 |
| YONDU_BASE_ARROWSPRAY_SPAWNOFFSET_Z | arrowSpawnOffsetZ | 0 |
| YONDU_BASE_RUNSPEED | runspeed | 6.5 |
| YONDU_BASE_YAKAARROW | yakaArrow | 0 |
| YONDU_BASE_ARROWSPRAY_ROTATIONANGLE | arrowSprayRotationAngle | -150 |
| YONDU_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.4 |
| YONDU_BASE_ARROWSPRAY_REPEATEDHITFACTOR | arrowSprayRepeatedHitScoreFactor | 1.5 |
| YONDU_BASE_YAKARICOCHET | ricochetCount | 3 |
| YONDU_BASE_GROUNDPOUND | groundpound | 0 |
| YONDU_BASE_ARROWSPRAY_TIMETOROTATE | arrowSprayTimeToRotate | 1 |
| YONDU_BASE_ARROWSPRAY_SPAWNOFFSET_X | arrowSpawnOffsetX | 0 |
| YONDU_BASE_SPECIALRICOCHET | specialRicochetCount | 0 |
| YONDU_BASE_ARROWSPRAY_SPAWN_RADIUS | arrowSpawnRadius | 0.8 |
| YONDU_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| YONDU_BASE_ARROWSPRAY_NUM | arrowSprayNum | 12 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| YONDU_SUPERJUMP2 | PRG_YON_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,44,10,12 | 3 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_YON_SuperJump2_desc |
| YONDU_RANGED_CHARGETIME1 | PRG_YON_ChargeTime1 | yakaChargeTime | 1.7 | 4 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_YON_ChargeTime1_desc |
| YONDU_MELEE_DMG1 | PRG_YON_MeleeDmg1 | meleeDamage | 0.25 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_YON_MeleeDmg1_desc |
| YONDU_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Yondu |  |
| YONDU_SUPERJUMP1 | PRG_YON_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 1 | 0 | 0 | 1 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_YON_SuperJump1_desc |
| YONDU_CHARGED_RICOCHET1 | PRG_YON_Ricochet1 | ricochetCount | 4 | 3 | 0 | 0 | 0 |  | PRG_RangedUpgrade1_Screen | AV_PRG_RangedUpgrade1 | PRG_YON_Ricochet1_desc |
| YONDU_EXPLOSIVEARROWS | PRG_YON_ExplosivesUnlocked | yakaExplosiveArrow | 1 | 6 | 0 | 0 | 0 |  | PRG_YON_ExplosivesUnlocked_Screen | AV_PRG_ExplosiveArrow | PRG_YON_ExplosivesUnlocked_desc |
| YONDU_SUPER_STRAIGHT_PUNCH | PRG_YON_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 1 |  | PRG_YON_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_YON_SuperStraightPunch_desc |
| YONDU_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| YONDU_RANGED_DMG2 | PRG_YON_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_YON_RangedDmg2_desc |
| YONDU_HEALTH2 | PRG_YON_Health2 | health | 30 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_YON_Health2_desc |
| YONDU_BLOCKBREAKER_COMBO | PRG_YON_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 0 |  | PRG_YON_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_YON_BlockBreakerCombo_desc |
| YONDU_RANGED_DMG1 | PRG_YON_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_YON_RangedDmg1_desc |
| YONDU_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| YONDU_RANGED_DMG3 | PRG_YON_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.25,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_YON_RangedDmg3_desc |
| YONDU_CHARGEDATTACK | PRG_YON_ChargeUnlocked | allowYakaCharge,yakaChargeTime | 1,2 | 2 | 1 | 0 | 1 |  | PRG_YON_ChargeUnlocked_Screen | AV_PRG_RangedChargeUp | PRG_YON_ChargeUnlocked_desc |
| YONDU_FREEZEARROW | PRG_YON_FreezeArrow | yakaArrow | 2 | 4 | 0 | 0 | 1 |  | PRG_YON_FreezeArrow_Screen | AV_PRG_FreezeArrow | PRG_YON_FreezeArrow_desc |
| YONDU_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| YONDU_GROUNDCOMBO5 | PRG_YON_GroundCombo5 | groundCombo5 | 1 | 4 | 1 | 0 | 1 |  | PRG_YON_GroundCombo5_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_YON_GroundCombo5_desc |
| YONDU_SPECIAL_DMG3 | PRG_YON_SpecialDmg3 | specialDamage | 1 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_YON_SpecialDmg3_desc |
| YONDU_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| YONDU_MELEE_DMG2 | PRG_YON_MeleeDmg2 | meleeDamage | 0.45 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_YON_MeleeDmg2_desc |
| YONDU_SPECIAL_DMG1 | PRG_YON_SpecialDmg1 | specialDamage | 0.35 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_YON_SpecialDmg1_desc |
| YONDU_RUNSPEED2 | PRG_YON_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_YON_RunSpeed2_desc |
| YONDU_RUNSPEED1 | PRG_YON_RunSpeed1 | runspeed | 7.5 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_YON_RunSpeed1_desc |
| YONDU_SPECIAL_DMG2 | PRG_YON_SpecialDmg2 | specialDamage | 0.65 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_YON_SpecialDmg2_desc |
| YONDU_HEALTH1 | PRG_YON_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_YON_Health1_desc |
| YONDU_RANGED_CHARGETIME2 | PRG_YON_ChargeTime2 | yakaChargeTime | 1.3 | 6 | 0 | 0 | 0 |  | PRG_ChargeSpeed2_Screen | AV_PRG_RangedCharge2 | PRG_YON_ChargeTime2_desc |
| YONDU_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| YONDU_HEALTH4 | PRG_YON_Health4 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_YON_Health4_desc |
| YONDU_HEALTH3 | PRG_YON_Health3 | health | 30 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_YON_Health3_desc |
| YONDU_SPECIAL_UNLOCK | PRG_YON_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 2 | 1 | 0 | 1 |  | PRG_YON_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_YON_SpecialUnlocked_desc |
| YONDU_SHOCKARROW | PRG_YON_ShockArrow | yakaArrow | 1 | 4 | 0 | 0 | 1 |  | PRG_YON_ShockArrow_Screen | AV_PRG_FreezeArrow | PRG_YON_ShockArrow_desc |
| YONDU_CHARGED_RICOCHET2 | PRG_YON_Ricochet2 | ricochetCount | 5 | 5 | 0 | 0 | 0 |  | PRG_RangedUpgrade2_Screen | AV_PRG_RangedUpgrade2 | PRG_YON_Ricochet2_desc |
| YONDU_GROUNDPOUND2 | PRG_YON_GroundPound2 | groundpound,groundpoundDamage | 3,2.0 | 2 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_YON_GroundPound2_desc |
