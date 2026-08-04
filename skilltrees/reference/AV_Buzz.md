# AV_Buzz

`ProgressionTree = "IN1_TS_Buzz"` -- source: `gamedb/core/in1_ts_buzz.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| BUZZ_BASEFIRERATE | sharedFireRate | 2.25 |
| BUZZ_BASECLIP | sharedClipSize | 6 |
| BUZZ_BASERUNSPEED | runspeed | 7 |
| BUZZ_BASEHEALTH | health | 150 |
| BUZZ_BASESHIELD | shield,shieldRegenDelay,shieldRegen | 0.0,5.0,10.0 |
| BUZZ_BASESUPERJUMP | BM_SuperJumper | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| BUZZ_MAXSHIELD1 | PRG_BUZ_BaseShield | shield | 15 | 2 | 1 | 0 | 0 | 1 | PRG_MaxShield1_Screen | AV_PRG_ShieldsUnlock | PRG_BUZ_BaseShield_desc |
| BUZZ_SUPERJUMP1 | PRG_BUZ_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 2 | 1 | 0 | 0 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_BUZ_SuperJump1_desc |
| BUZZ_MAXSHIELD3 | PRG_BUZ_MaxShield2 | shield | 15 | 5 | 0 | 0 | 0 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_BUZ_MaxShield2_desc |
| BUZZ_HEALTH1 | PRG_BUZ_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BUZ_Health1_desc |
| BUZZ_RANGED_DMG2 | PRG_BUZ_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.3 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BUZ_RangedDmg2_desc |
| BUZZ_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| BUZZ_MELEE_DMG2 | PRG_BUZ_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BUZ_MeleeDmg2_desc |
| BUZZ_RUNSPEED2 | PRG_BUZ_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_BUZ_RunSpeed2_desc |
| BUZZ_RUNSPEED1 | PRG_BUZ_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_BUZ_RunSpeed1_desc |
| BUZZ_SUPERJUMP2 | PRG_BUZ_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,44,10,12 | 4 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_BUZ_SuperJump2_desc |
| BUZZ_RANGED_DMG4 | PRG_BUZ_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.25,0.4 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BUZ_RangedDmg4_desc |
| BUZZ_MELEE_DMG1 | PRG_BUZ_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BUZ_MeleeDmg1_desc |
| BUZZ_MELEE_DMG5 | PRG_BUZ_MeleeDmg5 | meleeDamage | 0.25 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BUZ_MeleeDmg5_desc |
| BUZZ_RANGED_DMG3 | PRG_BUZ_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.2,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BUZ_RangedDmg3_desc |
| BUZZ_SHIELDREGEN1 | PRG_BUZ_ShieldRegen1 | shieldRegenDelay,shieldRegen | 4.0,17.5 | 6 | 0 | 0 | 0 |  | PRG_ShieldRegen1_Screen | AV_PRG_ShieldRegenUpgrade1 | PRG_BUZ_ShieldRegen1_desc |
| BUZZ_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Buzz |  |
| BUZZ_RANGED_DMG1 | PRG_BUZ_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BUZ_RangedDmg1_desc |
| BUZZ_HEALTH4 | PRG_BUZ_Health4 | health | 60 | 5 | 0 | 0 | 1 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BUZ_Health4_desc |
| BUZZ_MELEE_DMG4 | PRG_BUZ_MeleeDmg4 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BUZ_MeleeDmg4_desc |
| BUZZ_MAXSHIELD2 | PRG_BUZ_MaxShield1 | shield | 15 | 4 | 0 | 0 | 1 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_BUZ_MaxShield1_desc |
| BUZZ_MAXSHIELD4 | PRG_BUZ_MaxShield3 | shield | 15 | 6 | 0 | 0 | 0 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_BUZ_MaxShield3_desc |
| BUZZ_MELEE_DMG3 | PRG_BUZ_MeleeDmg3 | meleeDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BUZ_MeleeDmg3_desc |
| BUZZ_HEALTH2 | PRG_BUZ_Health2 | health | 30 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BUZ_Health2_desc |
| BUZZ_GROUNDPOUND2 | PRG_BUZ_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 2 | 1 | 0 | 1 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_BUZ_GroundPound2_desc |
| BUZZ_RANGED_DMG5 | PRG_BUZ_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.3,0.5 | 6 | 1 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BUZ_RangedDmg5_desc |
| BUZZ_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| BUZZ_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| BUZZ_HEALTH3 | PRG_BUZ_Health3 | health | 30 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BUZ_Health3_desc |
| BUZZ_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
