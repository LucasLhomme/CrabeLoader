# AVG_BlackWidow

`ProgressionTree = "IN2_AVG_BlackWidow"` -- source: `gamedb/core/in2_avg_blackwidow.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| BLACKWIDOW_TARGETBONUSMULTIPLIER | targetModeBonusMultiplier | 1.4 |
| BLACKWIDOW_BASECLIP | stingerClipSize | 10 |
| BLACKWIDOW_BASEFIRERATE | autopistolFireRate,sharedFireRateMultiplier | 2.5,1.0 |
| BLACKWIDOW_LEDGE_OBB | ledgeMinY,ledgeMaxY,ledgeJumpMinY,ledgeJumpMaxY | 1.0,2.4,1.5,2.3 |
| BLACKWIDOW_BASESHIELD | shield,shieldRegenDelay,shieldRegen | 0.0,4.5,10.0 |
| BLACKWIDOW_BASESUPERJUMP | BM_SuperJumper | 0 |
| BLACKWIDOW_BASERUNSPEED | runspeed | 6.5 |
| BLACKWIDOW_BASEHEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| BLACKWIDOW_CLIP2 | PRG_BWD_StingerClip2 | stingerClipSize | 32 | 5 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_BWD_StingerClip2_desc |
| BLACKWIDOW_RUNSPEED1 | PRG_BWD_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_BWD_RunSpeed1_desc |
| BLACKWIDOW_FIRERATE2 | PRG_BWD_FireRate2 | autopistolFireRate,sharedFireRateMultiplier | 3.5,2.0 | 6 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_BWD_FireRate2_desc |
| BLACKWIDOW_SUPERJUMP | PRG_BWD_SuperJump | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,44,10,12 | 1 | 0 | 0 | 0 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_BWD_SuperJump_desc |
| BLACKWIDOW_MELEE_DMG3 | PRG_BWD_MeleeDmg3 | meleeDamage | 0.35 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BWD_MeleeDmg3_desc |
| BLACKWIDOW_POWERDURATION1 | PRG_BWD_PowerDuration1 | powerDuration | 20 | 4 | 0 | 0 | 0 |  | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_BWD_PowerDuration1_desc |
| BLACKWIDOW_POWERFILL1 | PRG_PowerFill1 | powerFill | 45 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| BLACKWIDOW_HEALTH2 | PRG_BWD_Health2 | health | 100 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BWD_Health2_desc |
| BLACKWIDOW_FIRERATE1 | PRG_BWD_FireRate1 | autopistolFireRate,sharedFireRateMultiplier | 2.9,1.5 | 4 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_BWD_FireRate1_desc |
| BLACKWIDOW_MELEE_DMG1 | PRG_BWD_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BWD_MeleeDmg1_desc |
| BLACKWIDOW_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| BLACKWIDOW_SPECIAL_UNLOCKED | PRG_BWD_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDuration | 1.0,1.0,60.0,12.0 | 2 | 1 | 0 | 1 |  | PRG_BWD_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_BWD_SpecialUnlocked_desc |
| BLACKWIDOW_SHIELDREGEN1 | PRG_BWD_ShieldRegen1 | shieldRegenDelay,shieldRegen | 3.5,25.0 | 6 | 0 | 0 | 0 |  | PRG_ShieldRegen1_Screen | AV_PRG_ShieldRegenUpgrade1 | PRG_BWD_ShieldRegen1_desc |
| BLACKWIDOW_SUPER_STRAIGHT_PUNCH | PRG_BWD_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 0 |  | PRG_BWD_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_BWD_SuperStraightPunch_desc |
| BLACKWIDOW_BLOCKBREAKER_COMBO | PRG_BWD_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 1 |  | PRG_BWD_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_BWD_BlockBreakerCombo_desc |
| BLACKWIDOW_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| BLACKWIDOW_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| BLACKWIDOW_RANGED_DMG3 | PRG_BWD_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.125,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BWD_RangedDmg3_desc |
| BLACKWIDOW_RANGED_DMG2 | PRG_BWD_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.075,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BWD_RangedDmg2_desc |
| BLACKWIDOW_MAXSHIELD1 | PRG_BWD_BaseShield | shield | 30 | 3 | 0 | 0 | 1 |  | PRG_MaxShield1_Screen | AV_PRG_ShieldsUnlock | PRG_BWD_BaseShield_desc |
| BLACKWIDOW_RANGED_DMG1 | PRG_BWD_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.05,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BWD_RangedDmg1_desc |
| BLACKWIDOW_HEALTH1 | PRG_BWD_Health1 | health | 50 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BWD_Health1_desc |
| BLACKWIDOW_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| BLACKWIDOW_MELEE_DMG2 | PRG_BWD_MeleeDmg2 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BWD_MeleeDmg2_desc |
| BLACKWIDOW_CLIP1 | PRG_BWD_StingerClip1 | stingerClipSize | 24 | 3 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_BWD_StingerClip1_desc |
| BLACKWIDOW_GROUNDPOUND2 | PRG_BWD_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 3 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_BWD_GroundPound2_desc |
| BLACKWIDOW_DUALWIELD | PRG_BWD_DualWield | dualWield,multiShotDamage,stingerClipSize | 1,0.625,20 | 3 | 0 | 0 | 1 |  | PRG_DualWieldUnlock_Screen | AV_PRG_DualWieldUnlock | PRG_BWD_DualWield_desc |
| BLACKWIDOW_POWERDURATION2 | PRG_BWD_PowerDuration2 | powerDuration | 30 | 6 | 0 | 0 | 0 |  | PRG_SuperMoveDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_BWD_PowerDuration2_desc |
| BLACKWIDOW_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_BlackWidow |  |
| BLACKWIDOW_GROUNDCOMBO5 | PRG_BWD_GroundCombo5 | groundCombo5 | 1 | 4 | 1 | 0 | 0 |  | PRG_BWD_GroundCombo5_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_BWD_GroundCombo5_desc |
