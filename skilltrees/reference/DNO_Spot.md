# DNO_Spot

`ProgressionTree = "IN3_DNO_Spot"` -- source: `gamedb/core/in3_dno_spot.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| SPOT_BASE_LAUNCHER | blockBreakerCombo,superStraightPunch | 1.0,1.0 |
| SPOT_BASE_CHARGE | shoulderCharge | 1 |
| SPOT_BASE_POWERBAR | powerBar | 2 |
| SPOT_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| SPOT_BASE_HEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| SPOT_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| SPOT_POWERFILL1 | PRG_PowerFill1 | powerFill | 40 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| SPOT_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| SPOT_POWERBAR1 | PRG_PowerBar1 | powerBar | 2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar1_desc |
| SPOT_SPECIAL_DMG3 | PRG_SPT_SuperDmg3 | specialDamage | 0.5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_SPT_SuperDmg3_desc |
| SPOT_POWERFILL2 | PRG_PowerFill1 | powerFill | 20 | 6 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate2_Screen | AV_PRG_SuperMeterFillRate2 | PRG_PowerFill1_desc |
| SPOT_SUPER_JUMP1 | PRG_SPT_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 3 | 1 | 0 | 0 |  | PRG_SPT_SuperJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_SPT_SuperJump1_desc |
| SPOT_SPECIAL_DMG2 | PRG_SPT_SuperDmg2 | specialDamage | 0.3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_SPT_SuperDmg2_desc |
| SPOT_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| SPOT_HEALTH3 | PRG_SPT_Health3 | health | 100 | 6 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_SPT_Health3_desc |
| SPOT_RANGED_DMG3 | PRG_SPT_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.4,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SPT_RangedDmg3_desc |
| SPOT_MELEE_DMG2 | PRG_SPT_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SPT_MeleeDmg2_desc |
| SPOT_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Spot |  |
| SPOT_HEALTH2 | PRG_SPT_Health2 | health | 50 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_SPT_Health2_desc |
| SPOT_POWERBAR4 | PRG_PowerBar3 | powerBar | 2 | 5 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar3_desc |
| SPOT_POWERBAR3 | PRG_PowerBar3 | powerBar | 2 | 4 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar3_desc |
| SPOT_RANGED_DMG1 | PRG_SPT_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.15,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SPT_RangedDmg1_desc |
| SPOT_SPECIAL_DMG1 | PRG_SPT_SuperDmg1 | specialDamage | 0.2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_SPT_SuperDmg1_desc |
| SPOT_POWERBAR5 | PRG_PowerBar3 | powerBar | 2 | 6 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar3_desc |
| SPOT_MELEE_DMG3 | PRG_SPT_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SPT_MeleeDmg3_desc |
| SPOT_POWERBAR2 | PRG_PowerBar2 | powerBar | 2 | 3 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade3_Screen | AV_PRG_SuperMeterSizeUpgrade3 | PRG_PowerBar2_desc |
| SPOT_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| SPOT_STRENGTH | PRG_SPT_Strength | BM_SuperDuperStrong | 1 | 3 | 0 | 0 | 0 |  | PRG_StrengthUpgrade_Screen | AV_PRG_PickupThrowUpgrade1 | PRG_SPT_Strength_desc |
| SPOT_MELEE_DMG1 | PRG_SPT_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SPT_MeleeDmg1_desc |
| SPOT_IMP_CHARGE | PRG_SPT_ImpCharge | shoulderChargeSustained | 1 | 3 | 1 | 0 | 0 |  | PRG_SPT_ImpCharge_Screen | AV_PRG_DashAttack | PRG_SPT_ImpCharge_desc |
| SPOT_RANGED_DMG2 | PRG_SPT_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.25,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SPT_RangedDmg2_desc |
| SPOT_SPECIAL_UNLOCKED | PRG_SPT_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,4.0,60.0 | 1 | 1 | 0 | 1 |  | PRG_SPT_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_SPT_SpecialUnlocked_desc |
| SPOT_SUPER_JUMP2 | PRG_SPT_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,44,10,12 | 5 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_SPT_SuperJump2_desc |
| SPOT_HEALTH1 | PRG_SPT_Health1 | health | 50 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_SPT_Health1_desc |
| SPOT_GROUNDPOUND2 | PRG_SPT_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 1 | 0 | 0 |  | PRG_SPT_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_SPT_GroundPound2_desc |
