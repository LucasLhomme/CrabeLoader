# LAS_Stitch

`ProgressionTree = "IN2_LAS_Stitch"` -- source: `gamedb/core/in2_las_stitch.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| STITCH_BASEHEALTH | health | 150 |
| STITCH_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.4 |
| STITCH_WALLCRAWL1 | wallcrawl,wallcrawlJump | 1.0,1.0 |
| STITCH_BASECLIP | blasterClipSize | 8 |
| STITCH_BASERUNSPEED | runspeed | 6.5 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| STITCH_MELEE_DMG2 | PRG_STC_MeleeDmg2 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_STC_MeleeDmg2_desc |
| STITCH_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Stitch |  |
| STITCH_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| STITCH_SUPERJUMP1 | PRG_STC_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 2 | 0 | 0 | 1 |  | PRG_STC_SuperJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_STC_SuperJump1_desc |
| STITCH_SUPERJUMP2 | PRG_STC_SuperJump2 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,44,10,12 | 5 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_STC_SuperJump2_desc |
| STITCH_RANGED_DMG3 | PRG_STC_RangedDmg3 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.125,0.25,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_STC_RangedDmg3_desc |
| STITCH_MELEE_DMG1 | PRG_STC_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_STC_MeleeDmg1_desc |
| STITCH_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| STITCH_MELEE_DMG3 | PRG_STC_MeleeDmg3 | meleeDamage | 0.35 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_STC_MeleeDmg3_desc |
| STITCH_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| STITCH_RUNSPEED1 | PRG_STC_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_STC_RunSpeed1_desc |
| STITCH_CLIP1 | PRG_STC_BlasterClip1 | blasterClipSize | 12 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_STC_BlasterClip1_desc |
| STITCH_CHARGESPEED1 | PRG_STC_ChargeSpeed1 | blasterCharge | 1.2 | 4 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_STC_ChargeSpeed1_desc |
| STITCH_SUPER_DMG3 | PRG_STC_SuperDmg3 | specialDamage | 1 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_STC_SuperDmg3_desc |
| STITCH_SPECIAL_UNLOCKED | PRG_STC_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDuration | 1.0,1.0,120.0,4.0 | 2 | 1 | 0 | 1 |  | PRG_STC_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_STC_SpecialUnlocked_desc |
| STITCH_CHARGEDSHOT | PRG_STC_ChargedShot1 | allowBlasterCharge,blasterCharge | 1,1.5 | 2 | 1 | 0 | 0 |  | PRG_STC_ChargedShot1_Screen | AV_PRG_RangedChargeUp | PRG_STC_ChargedShot1_desc |
| STITCH_SUPER_DMG1 | PRG_STC_SuperDmg1 | specialDamage | 0.35 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_STC_SuperDmg1_desc |
| STITCH_BLOCKBREAKER_COMBO | PRG_STC_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 1 |  | PRG_STC_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_STC_BlockBreakerCombo_desc |
| STITCH_RANGED_DMG2 | PRG_STC_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.075,0.15,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_STC_RangedDmg2_desc |
| STITCH_CHARGESPEED2 | PRG_STC_ChargeSpeed2 | blasterCharge | 0.75 | 6 | 0 | 0 | 0 |  | PRG_ChargeSpeed2_Screen | AV_PRG_RangedCharge2 | PRG_STC_ChargeSpeed2_desc |
| STITCH_SUPER_DMG2 | PRG_STC_SuperDmg2 | specialDamage | 0.65 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_STC_SuperDmg2_desc |
| STITCH_RANGED_DMG1 | PRG_STC_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.05,0.1,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_STC_RangedDmg1_desc |
| STITCH_CLIP2 | PRG_STC_BlasterClip2 | blasterClipSize | -1 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade1 | PRG_STC_BlasterClip2_desc |
| STITCH_DUALWIELD | PRG_STC_DualWield | dualWield,multiShotDamage | 1,0.625 | 3 | 1 | 0 | 1 |  | PRG_STC_DualWield_Screen | AV_PRG_DualWieldUnlock | PRG_STC_DualWield_desc |
| STITCH_HEALTH1 | PRG_STC_Health1 | health | 50 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_STC_Health1_desc |
| STITCH_RUNSPEED2 | PRG_STC_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_STC_RunSpeed2_desc |
| STITCH_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| STITCH_GROUNDPOUND2 | PRG_STC_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_STC_GroundPound2_desc |
| STITCH_HEALTH2 | PRG_STC_Health2 | health | 100 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_STC_Health2_desc |
| STITCH_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
