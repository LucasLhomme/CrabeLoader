# LR_LoneRangerInfinite

`ProgressionTree = "IN1_LR_LoneRanger"` -- source: `gamedb/core/in1_lr_loneranger.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| LONERANGER_BASERUNSPEED | runspeed | 7 |
| LONERANGER_BASEHEALTH | health | 150 |
| LONERANGER_BASEFIRERATE | LRPistolFireRate | 2 |
| LONERANGER_BASECLIP | LRPistolClipSize | 6 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| LONERANGER_HEALTH4 | PRG_LNR_Health4 | health | 60 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_LNR_Health4_desc |
| LONERANGER_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| LONERANGER_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| LONERANGER_RANGED_DMG1 | PRG_LNR_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.05,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LNR_RangedDmg1_desc |
| LONERANGER_CLIP1 | PRG_LNR_ClipSize1 | LRPistolClipSize | 8 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_LNR_ClipSize1_desc |
| LONERANGER_GROUNDPOUND2 | PRG_LNR_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 3 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_LNR_GroundPound2_desc |
| LONERANGER_RANGED_DMG2 | PRG_LNR_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.05,0.3 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LNR_RangedDmg2_desc |
| LONERANGER_MELEE_DMG5 | PRG_LNR_MeleeDmg5 | meleeDamage | 0.25 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LNR_MeleeDmg5_desc |
| LONERANGER_RUNSPEED1 | PRG_LNR_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_LNR_RunSpeed1_desc |
| LONERANGER_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| LONERANGER_MELEE_DMG4 | PRG_LNR_MeleeDmg4 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LNR_MeleeDmg4_desc |
| LONERANGER_MELEE_DMG2 | PRG_LNR_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LNR_MeleeDmg2_desc |
| LONERANGER_RANGED_DMG5 | PRG_LNR_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.15,0.5 | 6 | 1 | 0 | 1 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LNR_RangedDmg5_desc |
| LONERANGER_RICOCHET_BLOCK | PRG_LNR_RicochetBlock | blockRicochet | 1 | 5 | 1 | 0 | 1 |  | PRG_RicochetBlock_Screen | AV_PRG_RicochetBlock | PRG_LNR_RicochetBlock_desc |
| LONERANGER_CLIP2 | PRG_LNR_ClipSize2 | LRPistolClipSize | 12 | 4 | 1 | 0 | 1 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_LNR_ClipSize2_desc |
| LONERANGER_MELEE_DMG3 | PRG_LNR_MeleeDmg3 | meleeDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LNR_MeleeDmg3_desc |
| LONERANGER_RANGED_DMG4 | PRG_LNR_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.1,0.4 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LNR_RangedDmg4_desc |
| LONERANGER_RANGED_DMG3 | PRG_LNR_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.05,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LNR_RangedDmg3_desc |
| LONERANGER_FIRERATE2 | PRG_LNR_FireRate2 | LRPistolFireRate | 2.5 | 5 | 1 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_LNR_FireRate2_desc |
| LONERANGER_RUNSPEED2 | PRG_LNR_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_LNR_RunSpeed2_desc |
| LONERANGER_FIRERATE1 | PRG_LNR_FireRate1 | LRPistolFireRate | 2.25 | 3 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_LNR_FireRate1_desc |
| LONERANGER_HEALTH3 | PRG_LNR_Health3 | health | 30 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_LNR_Health3_desc |
| LONERANGER_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_LoneRanger |  |
| LONERANGER_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| LONERANGER_MELEE_DMG1 | PRG_LNR_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LNR_MeleeDmg1_desc |
| LONERANGER_HEALTH1 | PRG_LNR_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_LNR_Health1_desc |
| LONERANGER_HEALTH2 | PRG_LNR_Health2 | health | 30 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_LNR_Health2_desc |
