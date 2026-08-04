# PIR_JackSparrowInfinite

`ProgressionTree = "IN1_PIR_JackSparrow"` -- source: `gamedb/core/in1_pir_jacksparrow.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| SPARROW_BASERUNSPEED | runspeed | 7 |
| SPARROW_BASEHEALTH | health | 150 |
| SPARROW_BASEFIRERATE | sharedFireRateMultiplier | 1 |
| SPARROW_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.4 |
| SPARROW_BASECLIP | sharedAmmoMultiplier | 4 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| SPARROW_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| SPARROW_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| SPARROW_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| SPARROW_HEALTH4 | PRG_JKS_Health4 | health | 60 | 6 | 0 | 0 | 1 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_JKS_Health4_desc |
| SPARROW_HEALTH2 | PRG_JKS_Health2 | health | 30 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_JKS_Health2_desc |
| SPARROW_RANGED_DMG3 | PRG_JKS_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.05,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JKS_RangedDmg3_desc |
| SPARROW_FIRERATE2 | PRG_JKS_FireRate2 | sharedFireRateMultiplier | 2 | 5 | 0 | 0 | 1 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_JKS_FireRate2_desc |
| SPARROW_MELEE_DMG1 | PRG_JKS_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JKS_MeleeDmg1_desc |
| SPARROW_RUNSPEED1 | PRG_JKS_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_JKS_RunSpeed1_desc |
| SPARROW_RANGED_DMG5 | PRG_JKS_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.15,0.5 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JKS_RangedDmg5_desc |
| SPARROW_FIRERATE1 | PRG_JKS_FireRate1 | sharedFireRateMultiplier | 1.5 | 3 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_JKS_FireRate1_desc |
| SPARROW_CLIP1 | PRG_JKS_ClipSize1 | sharedAmmoMultiplier | 6 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_JKS_ClipSize1_desc |
| SPARROW_RANGED_DMG2 | PRG_JKS_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.05,0.3 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JKS_RangedDmg2_desc |
| SPARROW_GROUNDPOUND2 | PRG_JKS_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 2 | 1 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_JKS_GroundPound2_desc |
| SPARROW_RICOCHET_BLOCK | PRG_JKS_RicochetBlock | blockRicochet | 1 | 5 | 1 | 0 | 1 |  | PRG_RicochetBlock_Screen | AV_PRG_RicochetBlock | PRG_JKS_RicochetBlock_desc |
| SPARROW_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| SPARROW_BLOCKBREAKER_COMBO | PRG_JKS_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 0 |  | PRG_BlockBreaker_Screen | AV_PRG_BlockBreaker | PRG_JKS_BlockBreakerCombo_desc |
| SPARROW_HEIGHTENED_SENSE | PRG_JKS_HeightenedSense | heightenedSense,BM_HeightenedSenses | 1.0,1.0 | 6 | 1 | 0 | 1 |  | PRG_HeightenedSense_Screen | AV_PRG_SpideySenseUnlock | PRG_JKS_HeightenedSense_desc |
| SPARROW_RUNSPEED2 | PRG_JKS_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_JKS_RunSpeed2_desc |
| SPARROW_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_JackSparrow |  |
| SPARROW_HEALTH3 | PRG_JKS_Health3 | health | 30 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_JKS_Health3_desc |
| SPARROW_MELEE_DMG4 | PRG_JKS_MeleeDmg4 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JKS_MeleeDmg4_desc |
| SPARROW_RANGED_DMG4 | PRG_JKS_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.1,0.4 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JKS_RangedDmg4_desc |
| SPARROW_MELEE_DMG5 | PRG_JKS_MeleeDmg5 | meleeDamage | 0.25 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JKS_MeleeDmg5_desc |
| SPARROW_RANGED_DMG1 | PRG_JKS_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.05,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JKS_RangedDmg1_desc |
| SPARROW_CLIP2 | PRG_JKS_ClipSize2 | sharedAmmoMultiplier | 8 | 4 | 0 | 0 | 1 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_JKS_ClipSize2_desc |
| SPARROW_HEALTH1 | PRG_JKS_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_JKS_Health1_desc |
| SPARROW_MELEE_DMG3 | PRG_JKS_MeleeDmg3 | meleeDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JKS_MeleeDmg3_desc |
| SPARROW_MELEE_DMG2 | PRG_JKS_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JKS_MeleeDmg2_desc |
