# FRO_Elsa

`ProgressionTree = "IN1_FRO_Elsa"` -- source: `gamedb/core/in1_fro_elsa.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| ELSA_BASEHEALTHREGEN | healthRegen | 0 |
| ELSA_BASEHEALTH | health | 150 |
| ELSA_BASERUNSPEED | runspeed | 7 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ELSA_MELEE_DMG1 | PRG_ELS_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ELS_MeleeDmg1_desc |
| ELSA_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| ELSA_HEALTHREGEN2 | PRG_ELS_HealthRegen2 | healthRegen | 5 | 6 | 1 | 0 | 1 |  | PRG_HealthRegen2_Screen | AV_PRG_RegenUpgrade1 | PRG_ELS_HealthRegen2_desc |
| ELSA_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| ELSA_RUNSPEED2 | PRG_ELS_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 1 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_ELS_RunSpeed2_desc |
| ELSA_GROUNDPOUND2 | PRG_ELS_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 3 | 1 | 0 | 1 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_ELS_GroundPound2_desc |
| ELSA_RUNSPEED1 | PRG_ELS_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_ELS_RunSpeed1_desc |
| ELSA_RANGED_DMG5 | PRG_ELS_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.3,0.50 | 6 | 1 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ELS_RangedDmg5_desc |
| ELSA_RANGED_DMG1 | PRG_ELS_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ELS_RangedDmg1_desc |
| ELSA_MELEE_DMG4 | PRG_ELS_MeleeDmg4 | meleeDamage | 0.2 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ELS_MeleeDmg4_desc |
| ELSA_RANGED_DMG3 | PRG_ELS_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.2,0.3 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ELS_RangedDmg3_desc |
| ELSA_RANGED_DMG4 | PRG_ELS_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.25,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ELS_RangedDmg4_desc |
| ELSA_RANGED_DMG2 | PRG_ELS_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ELS_RangedDmg2_desc |
| ELSA_MELEE_DMG5 | PRG_ELS_MeleeDmg5 | meleeDamage | 0.25 | 6 | 1 | 0 | 1 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ELS_MeleeDmg5_desc |
| ELSA_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Elsa |  |
| ELSA_HEALTHREGEN1 | PRG_ELS_HealthRegen1 | healthRegen | 2 | 5 | 0 | 0 | 0 |  | PRG_HealthRegen1_Screen | AV_PRG_RegenUnlock | PRG_ELS_HealthRegen1_desc |
| ELSA_HEALTH1 | PRG_ELS_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_ELS_Health1_desc |
| ELSA_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| ELSA_MELEE_DMG3 | PRG_ELS_MeleeDmg3 | meleeDamage | 0.15 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ELS_MeleeDmg3_desc |
| ELSA_MELEE_DMG2 | PRG_ELS_MeleeDmg2 | meleeDamage | 0.1 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ELS_MeleeDmg2_desc |
| ELSA_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| ELSA_HEALTH3 | PRG_ELS_Health3 | health | 70 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_ELS_Health3_desc |
| ELSA_HEALTH2 | PRG_ELS_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_ELS_Health2_desc |
