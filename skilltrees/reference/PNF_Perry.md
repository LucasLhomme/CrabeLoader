# PNF_Perry

`ProgressionTree = "IN1_PNF_Perry"` -- source: `gamedb/core/in1_pnf_perry.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| PERRY_BASEMELEE_DMG | meleeDamage | 1 |
| PERRY_BASESUPERJUMP | BM_SuperJumper | 0 |
| PERRY_BASESHIELD | shield,shieldRegenDelay,shieldRegen | 0.0,4.5,10.0 |
| PERRY_BASERUNSPEED | runspeed | 7 |
| PERRY_BASERANGED_DMG | rangedDamage,sharedRangedDamage | 1.0,1.0 |
| PERRY_BASEHEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| PERRY_GROUNDPOUND2 | PRG_AGP_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 2 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_AGP_GroundPound2_desc |
| PERRY_BLOCKBREAKER_COMBO | PRG_AGP_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 0 |  | PRG_BlockBreaker_Screen | AV_PRG_BlockBreaker | PRG_AGP_BlockBreakerCombo_desc |
| PERRY_RANGED_DMG2 | PRG_AGP_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_AGP_RangedDmg2_desc |
| PERRY_HEIGHTENED_SENSE | PRG_AGP_HeightenedSense | heightenedSense,BM_HeightenedSenses | 1,1 | 5 | 1 | 0 | 1 |  | PRG_HeightenedSense_Screen | AV_PRG_SpideySenseUnlock | PRG_AGP_HeightenedSense_desc |
| PERRY_MELEE_DMG5 | PRG_AGP_MeleeDmg5 | meleeDamage | 0.25 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_AGP_MeleeDmg5_desc |
| PERRY_SHIELDREGEN1 | PRG_AGP_ShieldRegen1 | shieldRegenDelay,shieldRegen | 3.5,25.0 | 5 | 0 | 0 | 0 |  | PRG_ShieldRegen1_Screen | AV_PRG_ShieldRegenUpgrade1 | PRG_AGP_ShieldRegen1_desc |
| PERRY_RANGED_DMG4 | PRG_AGP_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.25,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_AGP_RangedDmg4_desc |
| PERRY_RUNSPEED2 | PRG_AGP_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_AGP_RunSpeed2_desc |
| PERRY_RUNSPEED1 | PRG_AGP_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_AGP_RunSpeed1_desc |
| PERRY_MAXSHIELD2 | PRG_AGP_MaxShield1 | shield | 15 | 5 | 0 | 0 | 0 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_AGP_MaxShield1_desc |
| PERRY_MELEE_DMG3 | PRG_AGP_MeleeDmg3 | meleeDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_AGP_MeleeDmg3_desc |
| PERRY_RANGED_DMG5 | PRG_AGP_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.3,0.5 | 6 | 0 | 0 | 1 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_AGP_RangedDmg5_desc |
| PERRY_MAXSHIELD1 | PRG_AGP_BaseShield | shield | 15 | 3 | 1 | 0 | 1 | 1 | PRG_MaxShield1_Screen | AV_PRG_ShieldsUnlock | PRG_AGP_BaseShield_desc |
| PERRY_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_AgentP |  |
| PERRY_SUPERJUMP1 | PRG_AGP_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 2 | 0 | 0 | 0 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_AGP_SuperJump1_desc |
| PERRY_RANGED_DMG3 | PRG_AGP_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.2,0.3 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_AGP_RangedDmg3_desc |
| PERRY_SUPER_STRAIGHT_PUNCH | PRG_AGP_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 1 |  | PRG_SuperPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_AGP_SuperStraightPunch_desc |
| PERRY_RANGED_DMG1 | PRG_AGP_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_AGP_RangedDmg1_desc |
| PERRY_HEALTH2 | PRG_AGP_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_AGP_Health2_desc |
| PERRY_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| PERRY_MELEE_DMG4 | PRG_AGP_MeleeDmg4 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_AGP_MeleeDmg4_desc |
| PERRY_MELEE_DMG2 | PRG_AGP_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_AGP_MeleeDmg2_desc |
| PERRY_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| PERRY_HEALTH1 | PRG_AGP_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_AGP_Health1_desc |
| PERRY_MELEE_DMG1 | PRG_AGP_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_AGP_MeleeDmg1_desc |
| PERRY_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| PERRY_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| PERRY_HEALTH3 | PRG_AGP_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_AGP_Health3_desc |
