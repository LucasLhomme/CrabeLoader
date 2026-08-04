# LR_Tonto

`ProgressionTree = "IN1_LR_Tonto"` -- source: `gamedb/core/in1_lr_tonto.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| TONTO_BASEHEALTH | health | 150 |
| TONTO_BASERUNSPEED | runspeed | 7 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| TONTO_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| TONTO_HEALAURA_BLOCK | PRG_TTO_HealAuraBlock | blockHealthRegen,blockHealthDelay,blockHealthAura | 10.0,1.0,1.0 | 5 | 1 | 0 | 1 |  | PRG_HealAuraBlock_Screen | AV_PRG_HealAuraBlock | PRG_TTO_HealAuraBlock_desc |
| TONTO_HEALTH1 | PRG_TTO_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_TTO_Health1_desc |
| TONTO_MELEE_DMG4 | PRG_TTO_MeleeDmg4 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_TTO_MeleeDmg4_desc |
| TONTO_HEALTH3 | PRG_TTO_Health3 | health | 30 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_TTO_Health3_desc |
| TONTO_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| TONTO_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| TONTO_MELEE_DMG5 | PRG_TTO_MeleeDmg5 | meleeDamage | 0.25 | 6 | 1 | 0 | 1 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_TTO_MeleeDmg5_desc |
| TONTO_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| TONTO_RUNSPEED1 | PRG_TTO_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_TTO_RunSpeed1_desc |
| TONTO_HEALING_BLOCK | PRG_TTO_HealingBlock | blockHealthRegen,blockHealthDelay | 5.0,1.0 | 3 | 0 | 0 | 0 |  | PRG_HealingBlock_Screen | AV_PRG_HealingBlock | PRG_TTO_HealingBlock_desc |
| TONTO_GROUNDPOUND2 | PRG_TTO_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 3 | 1 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_TTO_GroundPound2_desc |
| TONTO_RUNSPEED2 | PRG_TTO_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_TTO_RunSpeed2_desc |
| TONTO_HEALTH2 | PRG_TTO_Health2 | health | 30 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_TTO_Health2_desc |
| TONTO_RANGED_DMG1 | PRG_TTO_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_TTO_RangedDmg1_desc |
| TONTO_MELEE_DMG1 | PRG_TTO_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_TTO_MeleeDmg1_desc |
| TONTO_MELEE_DMG2 | PRG_TTO_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_TTO_MeleeDmg2_desc |
| TONTO_MELEE_DMG3 | PRG_TTO_MeleeDmg3 | meleeDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_TTO_MeleeDmg3_desc |
| TONTO_RANGED_DMG4 | PRG_TTO_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.25,0.4 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_TTO_RangedDmg4_desc |
| TONTO_RANGED_DMG5 | PRG_TTO_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.3,0.5 | 6 | 1 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_TTO_RangedDmg5_desc |
| TONTO_RANGED_DMG3 | PRG_TTO_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.2,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_TTO_RangedDmg3_desc |
| TONTO_RANGED_DMG2 | PRG_TTO_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.3 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_TTO_RangedDmg2_desc |
| TONTO_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Tonto |  |
| TONTO_HEALTH4 | PRG_TTO_Health4 | health | 60 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_TTO_Health4_desc |
