# PIR_DavyJones

`ProgressionTree = "IN1_PIR_DavyJones"` -- source: `gamedb/core/in1_pir_davyjones.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| DAVYJONES_BASERUNSPEED | runspeed | 7 |
| DAVYJONES_BASEHEALTH | health | 150 |
| DAVYJONES_BASECLIP | sharedAmmoMultiplier | 4 |
| DAVYJONES_BASEFIRERATE | sharedFireRateMultiplier | 1 |
| DAVYJONES_BASEHEALTHREGEN | healthRegen | 0 |
| DAVYJONES_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.4 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| DAVYJONES_RICOCHET_BLOCK | PRG_DVY_RicochetBlock | blockRicochet | 1 | 5 | 1 | 0 | 1 |  | PRG_RicochetBlock_Screen | AV_PRG_RicochetBlock | PRG_DVY_RicochetBlock_desc |
| DAVYJONES_FIRERATE2 | PRG_DVY_FireRate2 | sharedFireRateMultiplier | 2 | 5 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_DVY_FireRate2_desc |
| DAVYJONES_HEALTH3 | PRG_DVY_Health3 | health | 30 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_DVY_Health3_desc |
| DAVYJONES_GROUNDPOUND2 | PRG_DVY_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 2 | 1 | 0 | 1 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_DVY_GroundPound2_desc |
| DAVYJONES_RUNSPEED2 | PRG_DVY_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_DVY_RunSpeed2_desc |
| DAVYJONES_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| DAVYJONES_HEALTH2 | PRG_DVY_Health2 | health | 30 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_DVY_Health2_desc |
| DAVYJONES_RUNSPEED1 | PRG_DVY_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_DVY_RunSpeed1_desc |
| DAVYJONES_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| DAVYJONES_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| DAVYJONES_HEALTH4 | PRG_DVY_Health4 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_DVY_Health4_desc |
| DAVYJONES_RANGED_DMG3 | PRG_DVY_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.05,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DVY_RangedDmg3_desc |
| DAVYJONES_RANGED_DMG5 | PRG_DVY_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.15,0.5 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DVY_RangedDmg5_desc |
| DAVYJONES_HEALTH1 | PRG_DVY_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_DVY_Health1_desc |
| DAVYJONES_RANGED_DMG4 | PRG_DVY_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.1,0.4 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DVY_RangedDmg4_desc |
| DAVYJONES_MELEE_DMG5 | PRG_DVY_MeleeDmg5 | meleeDamage | 0.25 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DVY_MeleeDmg5_desc |
| DAVYJONES_MELEE_DMG2 | PRG_DVY_MeleeDmg2 | meleeDamage | 0.1 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DVY_MeleeDmg2_desc |
| DAVYJONES_RANGED_DMG2 | PRG_DVY_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.05,0.3 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DVY_RangedDmg2_desc |
| DAVYJONES_RANGED_DMG1 | PRG_DVY_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.05,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DVY_RangedDmg1_desc |
| DAVYJONES_MELEE_DMG3 | PRG_DVY_MeleeDmg3 | meleeDamage | 0.15 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DVY_MeleeDmg3_desc |
| DAVYJONES_CLIP2 | PRG_DVY_ClipSize2 | sharedAmmoMultiplier | 8 | 4 | 0 | 0 | 1 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_DVY_ClipSize2_desc |
| DAVYJONES_CLIP1 | PRG_DVY_ClipSize1 | sharedAmmoMultiplier | 6 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_DVY_ClipSize1_desc |
| DAVYJONES_MELEE_DMG4 | PRG_DVY_MeleeDmg4 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DVY_MeleeDmg4_desc |
| DAVYJONES_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_DavyJones |  |
| DAVYJONES_MELEE_DMG1 | PRG_DVY_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DVY_MeleeDmg1_desc |
| DAVYJONES_FIRERATE1 | PRG_DVY_FireRate1 | sharedFireRateMultiplier | 1.5 | 3 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_DVY_FireRate1_desc |
| DAVYJONES_BLOCKBREAKER_COMBO | PRG_DVY_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 0 |  | PRG_BlockBreaker_Screen | AV_PRG_BlockBreaker | PRG_DVY_BlockBreakerCombo_desc |
| DAVYJONES_HEALTHREGEN2 | PRG_DVY_HealthRegen2 | healthRegen | 5 | 5 | 0 | 0 | 1 |  | PRG_HealthRegen2_Screen | AV_PRG_RegenUpgrade1 | PRG_DVY_HealthRegen2_desc |
| DAVYJONES_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| DAVYJONES_HEALTHREGEN1 | PRG_DVY_HealthRegen1 | healthRegen | 2 | 3 | 1 | 0 | 0 |  | PRG_HealthRegen1_Screen | AV_PRG_RegenUnlock | PRG_DVY_HealthRegen1_desc |
