# TAN_Rapunzel

`ProgressionTree = "IN1_TAN_Rapunzel"` -- source: `gamedb/core/in1_tan_rapunzel.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| RAPUNZEL_BASERUNSPEED | runspeed | 7 |
| RAPUNZEL_BASEMELEE_DMG | meleeDamage | 1 |
| RAPUNZEL_BASEHEALTHREGEN | healthRegen | 0 |
| RAPUNZEL_BASEHEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| RAPUNZEL_HEALTH4 | PRG_RAP_Health4 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RAP_Health4_desc |
| RAPUNZEL_MELEE_DMG5 | PRG_RAP_MeleeDmg5 | meleeDamage | 0.25 | 5 | 1 | 0 | 1 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RAP_MeleeDmg5_desc |
| RAPUNZEL_HEALTH2 | PRG_RAP_Health2 | health | 30 | 2 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RAP_Health2_desc |
| RAPUNZEL_MELEE_DMG4 | PRG_RAP_MeleeDmg4 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RAP_MeleeDmg4_desc |
| RAPUNZEL_HEALTH1 | PRG_RAP_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RAP_Health1_desc |
| RAPUNZEL_HEALING_BLOCK | PRG_RAP_HealingBlock | blockHealthRegen,blockHealthDelay | 5.0,1.0 | 2 | 0 | 0 | 0 |  | PRG_HealingBlock_Screen | AV_PRG_HealingBlock | PRG_RAP_HealingBlock_desc |
| RAPUNZEL_RANGED_DMG1 | PRG_RAP_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RAP_RangedDmg1_desc |
| RAPUNZEL_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| RAPUNZEL_RUNSPEED1 | PRG_RAP_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_RAP_RunSpeed1_desc |
| RAPUNZEL_RANGED_DMG5 | PRG_RAP_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.3,0.5 | 5 | 0 | 0 | 1 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RAP_RangedDmg5_desc |
| RAPUNZEL_RANGED_DMG4 | PRG_RAP_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.25,0.4 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_B_Screen | AV_PRG_RangedDamage_B | PRG_RAP_RangedDmg4_desc |
| RAPUNZEL_RANGED_DMG2 | PRG_RAP_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.3 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RAP_RangedDmg2_desc |
| RAPUNZEL_BLOCKBREAKER_COMBO | PRG_RAP_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 0 |  | PRG_BlockBreaker_Screen | AV_PRG_BlockBreaker | PRG_RAP_BlockBreakerCombo_desc |
| RAPUNZEL_HEALAURA_BLOCK | PRG_RAP_HealAuraBlock | blockHealthRegen,blockHealthDelay,blockHealthAura | 10.0,1.0,1.0 | 4 | 1 | 0 | 1 |  | PRG_HealAuraBlock_Screen | AV_PRG_HealAuraBlock | PRG_RAP_HealAuraBlock_desc |
| RAPUNZEL_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Rapunzel |  |
| RAPUNZEL_RANGED_DMG3 | PRG_RAP_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.2,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RAP_RangedDmg3_desc |
| RAPUNZEL_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| RAPUNZEL_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| RAPUNZEL_MELEE_DMG2 | PRG_RAP_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RAP_MeleeDmg2_desc |
| RAPUNZEL_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| RAPUNZEL_GROUNDPOUND2 | PRG_RAP_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 2 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_RAP_GroundPound2_desc |
| RAPUNZEL_HEALTHREGEN2 | PRG_RAP_HealthRegen2 | healthRegen | 5 | 5 | 0 | 0 | 0 |  | PRG_HealthRegen2_Screen | AV_PRG_RegenUpgrade1 | PRG_RAP_HealthRegen2_desc |
| RAPUNZEL_MELEE_DMG3 | PRG_RAP_MeleeDmg3 | meleeDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RAP_MeleeDmg3_desc |
| RAPUNZEL_RUNSPEED2 | PRG_RAP_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 1 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_RAP_RunSpeed2_desc |
| RAPUNZEL_HEALTH3 | PRG_RAP_Health3 | health | 30 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RAP_Health3_desc |
| RAPUNZEL_MELEE_DMG1 | PRG_RAP_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RAP_MeleeDmg1_desc |
| RAPUNZEL_HEALTHREGEN1 | PRG_RAP_HealthRegen1 | healthRegen | 2 | 3 | 1 | 0 | 0 |  | PRG_HealthRegen1_Screen | AV_PRG_RegenUnlock | PRG_RAP_HealthRegen1_desc |
