# AV_Violet

`ProgressionTree = "IN1_Inc_Violet"` -- source: `gamedb/core/in1_inc_violet.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| VIOLET_BASEHEALTH | health | 150 |
| VIOLET_BASERUNSPEED | runspeed | 7 |
| VIOLET_BASESHIELD | shield,shieldRegenDelay,shieldRegen | 0.0,4.5,10.0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| VIOLET_MAXSHIELD1 | PRG_VLT_BaseShield | shield | 15 | 1 | 0 | 0 | 1 | 1 | PRG_MaxShield1_Screen | AV_PRG_ShieldsUnlock | PRG_VLT_BaseShield_desc |
| VIOLET_HEALTH1 | PRG_VLT_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_VLT_Health1_desc |
| VIOLET_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| VIOLET_HEALTH4 | PRG_VLT_Health4 | health | 60 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_VLT_Health4_desc |
| VIOLET_HEALTH2 | PRG_VLT_Health2 | health | 30 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_VLT_Health2_desc |
| VIOLET_MAXSHIELD2 | PRG_VLT_MaxShield1 | shield | 15 | 3 | 0 | 0 | 0 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_VLT_MaxShield1_desc |
| VIOLET_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| VIOLET_GROUNDPOUND2 | PRG_VLT_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 2 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_VLT_GroundPound2_desc |
| VIOLET_RUNSPEED2 | PRG_VLT_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_VLT_RunSpeed2_desc |
| VIOLET_MELEE_DMG3 | PRG_VLT_MeleeDmg3 | meleeDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VLT_MeleeDmg3_desc |
| VIOLET_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Violet |  |
| VIOLET_SHIELDREGEN1 | PRG_VLT_ShieldRegen1 | shieldRegenDelay,shieldRegen | 3.5,25.0 | 6 | 0 | 0 | 0 |  | PRG_ShieldRegen1_Screen | AV_PRG_ShieldRegenUpgrade1 | PRG_VLT_ShieldRegen1_desc |
| VIOLET_RUNSPEED1 | PRG_VLT_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_VLT_RunSpeed1_desc |
| VIOLET_MELEE_DMG5 | PRG_VLT_MeleeDmg5 | meleeDamage | 0.25 | 6 | 1 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VLT_MeleeDmg5_desc |
| VIOLET_HEALTH3 | PRG_VLT_Health3 | health | 30 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_VLT_Health3_desc |
| VIOLET_MELEE_DMG4 | PRG_VLT_MeleeDmg4 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VLT_MeleeDmg4_desc |
| VIOLET_MELEE_DMG2 | PRG_VLT_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VLT_MeleeDmg2_desc |
| VIOLET_SUPER_STRAIGHT_PUNCH | PRG_VLT_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 1 |  | PRG_SuperPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_VLT_SuperStraightPunch_desc |
| VIOLET_RICOCHET_BLOCK | PRG_VLT_RicochetBlock | blockRicochet | 1 | 5 | 1 | 0 | 1 |  | PRG_RicochetBlock_Screen | AV_PRG_RicochetBlock | PRG_VLT_RicochetBlock_desc |
| VIOLET_HEIGHTENED_SENSE | PRG_VLT_HeightenedSense | heightenedSense,BM_HeightenedSenses | 1.0,1.0 | 6 | 0 | 0 | 0 |  | PRG_HeightenedSense_Screen | AV_PRG_SpideySenseUnlock | PRG_VLT_HeightenedSense_desc |
| VIOLET_MELEE_DMG1 | PRG_VLT_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VLT_MeleeDmg1_desc |
| VIOLET_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| VIOLET_BLOCKBREAKER_COMBO | PRG_VLT_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 0 |  | PRG_BlockBreaker_Screen | AV_PRG_BlockBreaker | PRG_VLT_BlockBreakerCombo_desc |
| VIOLET_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
