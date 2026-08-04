# AV_Syndrome

`ProgressionTree = "IN1_Inc_Syndrome"` -- source: `gamedb/core/in1_inc_syndrome.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| SYNDROME_BASERUNSPEED | runspeed | 7 |
| SYNDROME_BASEHEALTH | health | 150 |
| SYNDROME_BASESUPERJUMP | BM_SuperJumper | 0 |
| SYNDROME_BASESHIELD | shield,shieldRegenDelay,shieldRegen | 0.0,4.5,12.5 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| SYNDROME_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| SYNDROME_HEALTH1 | PRG_SYN_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SYN_Health1_desc |
| SYNDROME_SUPER_STRAIGHT_PUNCH | PRG_SYN_SuperStraightPunch | superStraightPunch | 1 | 4 | 0 | 0 | 0 |  | PRG_SuperPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_SYN_SuperStraightPunch_desc |
| SYNDROME_MELEE_DMG4 | PRG_SYN_MeleeDmg4 | meleeDamage | 0.2 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SYN_MeleeDmg4_desc |
| SYNDROME_SUPERJUMP1 | PRG_SYN_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 3 | 0 | 0 | 0 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_SYN_SuperJump1_desc |
| SYNDROME_MELEE_DMG1 | PRG_SYN_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SYN_MeleeDmg1_desc |
| SYNDROME_MELEE_DMG2 | PRG_SYN_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SYN_MeleeDmg2_desc |
| SYNDROME_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| SYNDROME_RANGED_DMG5 | PRG_SYN_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.3,0.5 | 6 | 1 | 0 | 1 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SYN_RangedDmg5_desc |
| SYNDROME_RANGED_DMG4 | PRG_SYN_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.25,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SYN_RangedDmg4_desc |
| SYNDROME_RUNSPEED1 | PRG_SYN_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_SYN_RunSpeed1_desc |
| SYNDROME_MAXSHIELD1 | PRG_SYN_BaseShield | shield | 15 | 2 | 0 | 0 | 1 | 1 | PRG_MaxShield1_Screen | AV_PRG_ShieldsUnlock | PRG_SYN_BaseShield_desc |
| SYNDROME_MAXSHIELD3 | PRG_SYN_MaxShield2 | shield | 15 | 6 | 0 | 0 | 0 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_SYN_MaxShield2_desc |
| SYNDROME_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| SYNDROME_RANGED_DMG2 | PRG_SYN_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.3 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SYN_RangedDmg2_desc |
| SYNDROME_RANGED_DMG3 | PRG_SYN_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.2,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SYN_RangedDmg3_desc |
| SYNDROME_MELEE_DMG5 | PRG_SYN_MeleeDmg5 | meleeDamage | 0.25 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SYN_MeleeDmg5_desc |
| SYNDROME_RUNSPEED2 | PRG_SYN_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_SYN_RunSpeed2_desc |
| SYNDROME_BLOCKBREAKER_COMBO | PRG_SYN_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 1 |  | PRG_BlockBreaker_Screen | AV_PRG_BlockBreaker | PRG_SYN_BlockBreakerCombo_desc |
| SYNDROME_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Syndrome |  |
| SYNDROME_RANGED_DMG1 | PRG_SYN_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SYN_RangedDmg1_desc |
| SYNDROME_HEIGHTENED_SENSE | PRG_SYN_HeightenedSense | heightenedSense,BM_HeightenedSenses | 1.0,1.0 | 6 | 0 | 0 | 1 |  | PRG_HeightenedSense_Screen | AV_PRG_SpideySenseUnlock | PRG_SYN_HeightenedSense_desc |
| SYNDROME_GROUNDPOUND2 | PRG_SYN_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 3 | 1 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_SYN_GroundPound2_desc |
| SYNDROME_HEALTH2 | PRG_SYN_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SYN_Health2_desc |
| SYNDROME_MELEE_DMG3 | PRG_SYN_MeleeDmg3 | meleeDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SYN_MeleeDmg3_desc |
| SYNDROME_HEALTH3 | PRG_SYN_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SYN_Health3_desc |
| SYNDROME_MAXSHIELD2 | PRG_SYN_MaxShield1 | shield | 15 | 4 | 1 | 0 | 0 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_SYN_MaxShield1_desc |
| SYNDROME_SHIELDREGEN1 | PRG_SYN_ShieldRegen1 | shieldRegenDelay,shieldRegen | 4.5,25.0 | 6 | 0 | 0 | 0 |  | PRG_ShieldRegen1_Screen | AV_PRG_ShieldRegenUpgrade1 | PRG_SYN_ShieldRegen1_desc |
| SYNDROME_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
