# AVG_CaptainAmerica

`ProgressionTree = "IN2_AVG_CaptainAmerica"` -- source: `gamedb/core/in2_avg_captainamerica.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| CAPAMERICA_BASEHEALTH | health | 150 |
| CAPAMERICA_BASESUPERJUMP | BM_SuperJumper | 0 |
| CAPAMERICA_BASERUNSPEED | runspeed | 8 |
| CAPAMERICA_BASERICOCHET | shieldRicochet,ricochetCount | 0,0.0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| CAPAMERICA_SUPERJUMP2 | PRG_CAP_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,64,11.3,10 | 3 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_CAP_SuperJump2_desc |
| CAPAMERICA_SUPERJUMP1 | PRG_CAP_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,44,10,12 | 1 |  | 0 | 1 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_CAP_SuperJump1_desc |
| CAPAMERICA_RUNSPEED2 | PRG_CAP_RunSpeed2 | runspeed | 10.25 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_CAP_RunSpeed2_desc |
| CAPAMERICA_HEALTH2 | PRG_CAP_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_CAP_Health2_desc |
| CAPAMERICA_SHIELDRICOCHET1 | PRG_CAP_ShieldRicochet1 | shieldRicochet,ricochetCount | 1,1.0 | 2 | 1 | 0 | 1 |  | PRG_CAP_ShieldRicochet1_Screen | AV_PRG_RangedChargeUp | PRG_CAP_ShieldRicochet1_desc |
| CAPAMERICA_MELEE_DMG1 | PRG_CAP_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_CAP_MeleeDmg1_desc |
| CAPAMERICA_SPECIAL_UNLOCKED | PRG_CAP_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 2 | 1 | 0 | 1 |  | PRG_CAP_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_CAP_SpecialUnlocked_desc |
| CAPAMERICA_GROUNDCOMBO5 | PRG_CAP_GroundCombo5 | groundCombo5 | 1 | 4 | 0 | 0 | 0 |  | PRG_CAP_GroundCombo5_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_CAP_GroundCombo5_desc |
| CAPAMERICA_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_CaptAmerica |  |
| CAPAMERICA_SHIELDRICOCHET4 | PRG_CAP_ShieldRicochet4 | ricochetCount | 5 | 5 | 0 | 0 | 0 |  | PRG_CAP_ShieldRicochet1_Screen | AV_PRG_RangedUpgrade3 | PRG_CAP_ShieldRicochet4_desc |
| CAPAMERICA_SUPER_DMG3 | PRG_CAP_SuperDmg3 | specialDamage | 0.75 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_CAP_SuperDmg3_desc |
| CAPAMERICA_SUPER_DMG2 | PRG_CAP_SuperDmg2 | specialDamage | 0.45 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_CAP_SuperDmg2_desc |
| CAPAMERICA_HEALTH1 | PRG_CAP_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_CAP_Health1_desc |
| CAPAMERICA_GROUNDPOUND2 | PRG_CAP_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 1 | 0 | 0 |  | PRG_CAP_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_CAP_GroundPound2_desc |
| CAPAMERICA_MELEE_DMG3 | PRG_CAP_MeleeDmg3 | meleeDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_CAP_MeleeDmg3_desc |
| CAPAMERICA_SUPER_STRAIGHT_PUNCH | PRG_CAP_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 1 |  | PRG_CAP_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_CAP_SuperStraightPunch_desc |
| CAPAMERICA_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| CAPAMERICA_RUNSPEED1 | PRG_CAP_RunSpeed1 | runspeed | 9.5 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_CAP_RunSpeed1_desc |
| CAPAMERICA_MELEE_DMG2 | PRG_CAP_MeleeDmg2 | meleeDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_CAP_MeleeDmg2_desc |
| CAPAMERICA_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| CAPAMERICA_SUPER_DMG1 | PRG_CAP_SuperDmg1 | specialDamage | 0.3 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_CAP_SuperDmg1_desc |
| CAPAMERICA_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| CAPAMERICA_RANGED_DMG2 | PRG_CAP_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.3,0.15,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_CAP_RangedDmg2_desc |
| CAPAMERICA_RANGED_DMG1 | PRG_CAP_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.2,0.1,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_CAP_RangedDmg1_desc |
| CAPAMERICA_SHIELDRICOCHET3 | PRG_CAP_ShieldRicochet3 | ricochetCount | 4 | 4 | 0 | 0 | 0 |  | PRG_CAP_ShieldRicochet1_Screen | AV_PRG_RangedUpgrade2 | PRG_CAP_ShieldRicochet3_desc |
| CAPAMERICA_SHIELDRICOCHET2 | PRG_CAP_ShieldRicochet2 | ricochetCount | 3 | 3 | 0 | 0 | 0 |  | PRG_CAP_ShieldRicochet1_Screen | AV_PRG_RangedUpgrade1 | PRG_CAP_ShieldRicochet2_desc |
| CAPAMERICA_HEALTH3 | PRG_CAP_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_CAP_Health3_desc |
| CAPAMERICA_RANGED_DMG3 | PRG_CAP_RangedDmg3 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.5,0.25,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_CAP_RangedDmg3_desc |
| CAPAMERICA_RICOCHET_BLOCK | PRG_CAP_RicochetBlock | blockRicochet | 1 | 1 | 0 | 0 | 0 |  | PRG_RicochetBlock_Screen | AV_PRG_RicochetBlock | PRG_CAP_RicochetBlock_desc |
| CAPAMERICA_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| CAPAMERICA_BLOCKBREAKER_COMBO | PRG_CAP_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 0 |  | PRG_CAP_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_CAP_BlockBreakerCombo_desc |
| CAPAMERICA_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
