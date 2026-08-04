# PNF_Phineas

`ProgressionTree = "IN1_PNF_Phineas"` -- source: `gamedb/core/in1_pnf_phineas.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| PHINEAS_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.6 |
| PHINEAS_BASERUNSPEED | runspeed | 7 |
| PHINEAS_BASERANGED_DMG | rangedDamage | 0.8 |
| PHINEAS_BASEHEALTH | health | 150 |
| PHINEAS_BASESUPERJUMP | BM_SuperJumper | 0 |
| PHINEAS_BASECLIP | sharedAmmoMultiplier | 4 |
| PHINEAS_BASEFIRERATE | sharedFireRateMultiplier | 1 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| PHINEAS_CLIP1 | PRG_PHI_ClipSize1 | sharedAmmoMultiplier | 8 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_PHI_ClipSize1_desc |
| PHINEAS_MELEE_DMG2 | PRG_PHI_MeleeDmg2 | meleeDamage | 0.1 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_PHI_MeleeDmg2_desc |
| PHINEAS_RANGED_DMG5 | PRG_PHI_RangedDmg5 | rangedDamage,sharedRangedDamage | 0.15,0.5 | 6 | 1 | 0 | 1 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_PHI_RangedDmg5_desc |
| PHINEAS_SUPERJUMP1 | PRG_PHI_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 3 | 1 | 0 | 1 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_PHI_SuperJump1_desc |
| PHINEAS_HEALTH1 | PRG_PHI_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_PHI_Health1_desc |
| PHINEAS_MELEE_DMG4 | PRG_PHI_MeleeDmg4 | meleeDamage | 0.2 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_PHI_MeleeDmg4_desc |
| PHINEAS_MELEE_DMG3 | PRG_PHI_MeleeDmg3 | meleeDamage | 0.15 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_PHI_MeleeDmg3_desc |
| PHINEAS_GROUNDPOUND2 | PRG_PHI_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 3 | 1 | 0 | 1 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_PHI_GroundPound2_desc |
| PHINEAS_CLIP2 | PRG_PHI_ClipSize2 | sharedAmmoMultiplier | 12 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_PHI_ClipSize2_desc |
| PHINEAS_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Phineas |  |
| PHINEAS_RANGED_DMG4 | PRG_PHI_RangedDmg4 | rangedDamage,sharedRangedDamage | 0.125,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_PHI_RangedDmg4_desc |
| PHINEAS_FIRERATE2 | PRG_PHI_FireRate2 | sharedFireRateMultiplier | 2 | 5 | 1 | 0 | 1 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_PHI_FireRate2_desc |
| PHINEAS_MELEE_DMG1 | PRG_PHI_MeleeDmg1 | meleeDamage | 0.1 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_PHI_MeleeDmg1_desc |
| PHINEAS_HEALTH2 | PRG_PHI_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_PHI_Health2_desc |
| PHINEAS_RUNSPEED2 | PRG_PHI_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_PHI_RunSpeed2_desc |
| PHINEAS_HEALTH3 | PRG_PHI_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_PHI_Health3_desc |
| PHINEAS_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| PHINEAS_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| PHINEAS_RUNSPEED1 | PRG_PHI_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_PHI_RunSpeed1_desc |
| PHINEAS_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| PHINEAS_RANGED_DMG3 | PRG_PHI_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.1,0.3 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_PHI_RangedDmg3_desc |
| PHINEAS_RANGED_DMG2 | PRG_PHI_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.075,0.3 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_PHI_RangedDmg2_desc |
| PHINEAS_RANGED_DMG1 | PRG_PHI_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.05,0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_PHI_RangedDmg1_desc |
| PHINEAS_FIRERATE1 | PRG_PHI_FireRate1 | sharedFireRateMultiplier | 1.5 | 3 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_PHI_FireRate1_desc |
| PHINEAS_MELEE_DMG5 | PRG_PHI_MeleeDmg5 | meleeDamage | 0.25 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_PHI_MeleeDmg5_desc |
| PHINEAS_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
