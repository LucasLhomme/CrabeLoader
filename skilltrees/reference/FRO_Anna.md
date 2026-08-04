# FRO_Anna

`ProgressionTree = "IN1_FRO_Anna"` -- source: `gamedb/core/in1_fro_anna.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| ANNA_BASERUNSPEED | runspeed | 7 |
| ANNA_BASEHEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ANNA_HEALTH1 | PRG_ANA_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_ANA_Health1_desc |
| ANNA_HEALTH4 | PRG_ANA_Health4 | health | 60 | 6 | 0 | 0 | 1 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_ANA_Health4_desc |
| ANNA_MELEE_DMG2 | PRG_ANA_MeleeDmg2 | meleeDamage | 0.1 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ANA_MeleeDmg2_desc |
| ANNA_RICOCHET_BLOCK | PRG_ANA_RicochetBlock | blockRicochet | 1 | 6 | 0 | 0 | 1 |  | PRG_RicochetBlock_Screen | AV_PRG_RicochetBlock | PRG_ANA_RicochetBlock_desc |
| ANNA_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| ANNA_RANGED_DMG2 | PRG_ANA_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.3,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ANA_RangedDmg2_desc |
| ANNA_MELEE_DMG3 | PRG_ANA_MeleeDmg3 | meleeDamage | 0.15 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ANA_MeleeDmg3_desc |
| ANNA_RUNSPEED1 | PRG_ANA_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_ANA_RunSpeed1_desc |
| ANNA_RANGED_DMG5 | PRG_ANA_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.5,0.5 | 6 | 1 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ANA_RangedDmg5_desc |
| ANNA_MELEE_DMG1 | PRG_ANA_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ANA_MeleeDmg1_desc |
| ANNA_RANGED_DMG4 | PRG_ANA_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.4,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ANA_RangedDmg4_desc |
| ANNA_RANGED_DMG3 | PRG_ANA_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.3,0.3 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ANA_RangedDmg3_desc |
| ANNA_GROUNDPOUND2 | PRG_ANA_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 3 | 1 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_ANA_GroundPound2_desc |
| ANNA_RANGED_DMG1 | PRG_ANA_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.25,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ANA_RangedDmg1_desc |
| ANNA_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Anna |  |
| ANNA_MELEE_DMG5 | PRG_ANA_MeleeDmg5 | meleeDamage | 0.25 | 6 | 1 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ANA_MeleeDmg5_desc |
| ANNA_HEALTH3 | PRG_ANA_Health3 | health | 30 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_ANA_Health3_desc |
| ANNA_RUNSPEED2 | PRG_ANA_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 1 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_ANA_RunSpeed2_desc |
| ANNA_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| ANNA_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| ANNA_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| ANNA_MELEE_DMG4 | PRG_ANA_MeleeDmg4 | meleeDamage | 0.2 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ANA_MeleeDmg4_desc |
| ANNA_BLOCKBREAKER_COMBO | PRG_ANA_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 1 |  | PRG_BlockBreaker_Screen | AV_PRG_BlockBreaker | PRG_ANA_BlockBreakerCombo_desc |
| ANNA_HEALTH2 | PRG_ANA_Health2 | health | 30 | 2 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_ANA_Health2_desc |
