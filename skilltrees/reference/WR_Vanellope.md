# WR_Vanellope

`ProgressionTree = "IN1_WR_Vanellope"` -- source: `gamedb/core/in1_wr_vanellope.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| VANELLOPE_BASESHIELD | shield,shieldRegenDelay,shieldRegen | 0.0,4.5,10.0 |
| VANELLOPE_BASERUNSPEED | runspeed | 7 |
| VANELLOPE_BASEHEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| VANELLOPE_RANGED_DMG1 | PRG_VAN_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_VAN_RangedDmg1_desc |
| VANELLOPE_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Vanellope |  |
| VANELLOPE_MELEE_DMG3 | PRG_VAN_MeleeDmg3 | meleeDamage | 0.15 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VAN_MeleeDmg3_desc |
| VANELLOPE_GROUNDPOUND2 | PRG_VAN_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 3 | 1 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_VAN_GroundPound2_desc |
| VANELLOPE_MAXSHIELD1 | PRG_VAN_BaseShield | shield | 15 | 3 | 1 | 0 | 1 | 1 | PRG_MaxShield1_Screen | AV_PRG_ShieldsUnlock | PRG_VAN_BaseShield_desc |
| VANELLOPE_HEALTH3 | PRG_VAN_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_VAN_Health3_desc |
| VANELLOPE_HEALTH2 | PRG_VAN_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_VAN_Health2_desc |
| VANELLOPE_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| VANELLOPE_RUNSPEED2 | PRG_VAN_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 1 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_VAN_RunSpeed2_desc |
| VANELLOPE_HEALTH1 | PRG_VAN_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_VAN_Health1_desc |
| VANELLOPE_RANGED_DMG4 | PRG_VAN_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.25,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_VAN_RangedDmg4_desc |
| VANELLOPE_MELEE_DMG5 | PRG_VAN_MeleeDmg5 | meleeDamage | 0.25 | 6 | 1 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VAN_MeleeDmg5_desc |
| VANELLOPE_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| VANELLOPE_MAXSHIELD2 | PRG_VAN_MaxShield1 | shield | 15 | 6 | 0 | 0 | 0 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_VAN_MaxShield1_desc |
| VANELLOPE_RANGED_DMG5 | PRG_VAN_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.3,0.5 | 6 | 0 | 0 | 1 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_VAN_RangedDmg5_desc |
| VANELLOPE_MELEE_DMG1 | PRG_VAN_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VAN_MeleeDmg1_desc |
| VANELLOPE_RANGED_DMG3 | PRG_VAN_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.2,0.3 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_VAN_RangedDmg3_desc |
| VANELLOPE_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| VANELLOPE_RANGED_DMG2 | PRG_VAN_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_VAN_RangedDmg2_desc |
| VANELLOPE_RUNSPEED1 | PRG_VAN_RunSpeed1 | runspeed | 8 | 1 | 1 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_VAN_RunSpeed1_desc |
| VANELLOPE_MELEE_DMG4 | PRG_VAN_MeleeDmg4 | meleeDamage | 0.2 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VAN_MeleeDmg4_desc |
| VANELLOPE_SHIELDREGEN1 | PRG_VAN_ShieldRegen1 | shieldRegenDelay,shieldRegen | 3.5,25.0 | 3 | 0 | 0 | 0 |  | PRG_ShieldRegen1_Screen | AV_PRG_ShieldRegenUpgrade1 | PRG_VAN_ShieldRegen1_desc |
| VANELLOPE_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| VANELLOPE_MELEE_DMG2 | PRG_VAN_MeleeDmg2 | meleeDamage | 0.1 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_VAN_MeleeDmg2_desc |
