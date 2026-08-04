# GOG_StarLord

`ProgressionTree = "IN2_GOG_StarLord"` -- source: `gamedb/core/in2_gog_starlord.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| STARLORD_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.4 |
| STARLORD_BASETURRETHEALTHUNLOCKED | rocketTurretHealthUnlocked | 0 |
| STARLORD_BASECLIP | autopistolClipSize | 8 |
| STARLORD_BASERUNSPEED | runspeed | 6.5 |
| STARLORD_BASECLIP1UNLOCKED | clipSize1Unlocked | 0 |
| STARLORD_BASESUPERJUMP | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,21,8.5,8 |
| STARLORD_TURRETBASEDAMAGE | specialDamage | 1 |
| STARLORD_BASECLIP2UNLOCKED | clipSize2Unlocked | 0 |
| STARLORD_BASEFIRERATE | autopistolFireRate,sharedFireRateMultiplier | 3.0,1.0 |
| STARLORD_BASETURRETHEALTH | rocketTurretHealth | 40 |
| STARLORD_BASETURRETCLIPSIZE | turretClipSize | 8 |
| STARLORD_BASEHEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| STARLORD_FIRERATE1 | PRG_SLD_FireRate1 | autopistolFireRate,sharedFireRateMultiplier | 3.5,1.5 | 3 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_SLD_FireRate1_desc |
| STARLORD_SEEKINGSHELLS | PRG_SLD_ExplosiveShells | seekingShells | 1 | 6 | 0 | 0 | 0 |  | PRG_SLD_ExplosiveShells_Screen | AV_PRG_RangedUpgrade3 | PRG_SLD_ExplosiveShells_desc |
| STARLORD_SPECIAL_UNLOCKED | PRG_SLD_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,90.0 | 2 | 1 | 0 | 1 |  | PRG_SLD_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_SLD_SpecialUnlocked_desc |
| STARLORD_CLIP1 | PRG_SLD_SmgClip1 | autopistolClipSize | 20 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_SLD_SmgClip1_desc |
| STARLORD_POWERFILL1 | PRG_PowerFill1 | powerFill | 75 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| STARLORD_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| STARLORD_SUPER_DMG1 | PRG_SLD_SuperDmg1 | specialDamage | 0.2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_SLD_SuperDmg1_desc |
| STARLORD_SUPER_STRAIGHT_PUNCH | PRG_SLD_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 1 |  | PRG_SLD_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_SLD_SuperStraightPunch_desc |
| STARLORD_TURRET_CLIP2 | PRG_SLD_TurretClip2 | turretClipSize,clipSize2Unlocked | 15, 1 | 6 | 0 | 0 | 0 |  | PRG_TurretPowerUpgrade2_Screen | AV_PRG_TurretPowerUpgrade2 | PRG_SLD_TurretClip2_desc |
| STARLORD_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| STARLORD_SUPERJUMP2 | PRG_SLD_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,44,10,12 | 5 | 1 | 0 | 1 |  | PRG_SLD_SuperJump1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_SLD_SuperJump2_desc |
| STARLORD_CLIP2 | PRG_SLD_SmgClip2 | autopistolClipSize | 28 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_SLD_SmgClip2_desc |
| STARLORD_HEALTH3 | PRG_SLD_Health3 | health | 70 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SLD_Health3_desc |
| STARLORD_DUALWIELD | PRG_SLD_DualWield | autopistolDual,multiShotDamage,autopistolClipSize | 1.00,0.625,16 | 1 | 1 | 0 | 1 |  | PRG_SLD_DualWield_Screen | AV_PRG_DualWieldUnlock | PRG_SLD_DualWield_desc |
| STARLORD_HEALTH2 | PRG_SLD_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SLD_Health2_desc |
| STARLORD_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_StarLord |  |
| STARLORD_RANGED_DMG1 | PRG_SLD_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.05,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SLD_RangedDmg1_desc |
| STARLORD_BLOCKBREAKER_COMBO | PRG_SLD_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 0 |  | PRG_SLD_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_SLD_BlockBreakerCombo_desc |
| STARLORD_MELEE_DMG1 | PRG_SLD_MeleeDmg1 | meleeDamage | 0.25 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SLD_MeleeDmg1_desc |
| STARLORD_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| STARLORD_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| STARLORD_RUNSPEED1 | PRG_SLD_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_SLD_RunSpeed1_desc |
| STARLORD_TURRET_HEALTH1 | PRG_SLD_TurretHealth | rocketTurretHealth,rocketTurretHealthUnlocked | 100, 1 | 5 | 0 | 0 | 0 |  | PRG_TurretHealthUpgrade1_Screen | AV_PRG_TurretHealthUpgrade1 | PRG_SLD_TurretHealth_desc |
| STARLORD_SUPERJUMP1 | PRG_SLD_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 1 | 0 | 0 | 1 |  | PRG_SLD_SuperJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_SLD_SuperJump1_desc |
| STARLORD_SUPER_DMG3 | PRG_SLD_SuperDmg3 | specialDamage | 0.5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_SLD_SuperDmg3_desc |
| STARLORD_SUPER_DMG2 | PRG_SLD_SuperDmg2 | specialDamage | 0.3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_SLD_SuperDmg2_desc |
| STARLORD_GROUNDPOUND2 | PRG_SLD_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 0 | 0 | 0 |  | PRG_SLD_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_SLD_GroundPound2_desc |
| STARLORD_HEALTH1 | PRG_SLD_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SLD_Health1_desc |
| STARLORD_TURRET_CLIP1 | PRG_SLD_TurretClip1 | turretClipSize,clipSize1Unlocked | 12, 1 | 3 | 0 | 0 | 0 |  | PRG_TurretPowerUpgrade1_Screen | AV_PRG_TurretPowerUpgrade1 | PRG_SLD_TurretClip1_desc |
| STARLORD_MELEE_DMG2 | PRG_SLD_MeleeDmg2 | meleeDamage | 0.45 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SLD_MeleeDmg2_desc |
| STARLORD_RANGED_DMG3 | PRG_SLD_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.125,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SLD_RangedDmg3_desc |
| STARLORD_RANGED_DMG2 | PRG_SLD_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.075,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SLD_RangedDmg2_desc |
| STARLORD_FIRERATE2 | PRG_SLD_FireRate2 | autopistolFireRate,sharedFireRateMultiplier | 4.0,2.0 | 5 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_SLD_FireRate2_desc |
