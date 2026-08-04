# TRN_Sam

`ProgressionTree = "IN2_TRN_Sam"` -- source: `gamedb/core/in2_trn_sam.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| SAM_BASERANGED_DMG | rangedDamage,sharedRangedDamage | 1.0,1.0 |
| SAM_BASESUPERJUMP | BM_SuperJumper | 0 |
| SAM_BASERUNSPEED | runspeed | 7 |
| SAM_BASEHEALTH | health | 150 |
| SAM_BASEMELEE_DMG | meleeDamage | 1 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| SAM_SHIELDRICOCHET3 | PRG_SAM_ShieldRicochet3 | ricochetCount | 4 | 4 | 0 | 0 | 0 |  | PRG_SAM_ShieldRicochet1_Screen | AV_PRG_RangedUpgrade2 | PRG_SAM_ShieldRicochet3_desc |
| SAM_SUPER_DMG3 | PRG_SAM_SuperDmg3 | specialDamage | 0.7 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_SAM_SuperDmg3_desc |
| SAM_SUPERJUMP2 | PRG_SAM_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,44,10,12 | 4 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_SAM_SuperJump2_desc |
| SAM_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Sam |  |
| SAM_RANGED_DMG2 | PRG_SAM_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SAM_RangedDmg2_desc |
| SAM_SHIELDRICOCHET2 | PRG_SAM_ShieldRicochet2 | ricochetCount | 3 | 2 | 0 | 0 | 0 |  | PRG_SAM_ShieldRicochet1_Screen | AV_PRG_RangedUpgrade1 | PRG_SAM_ShieldRicochet2_desc |
| SAM_SUPER_STRAIGHT_PUNCH | PRG_SAM_SuperStraightPunch | superStraightPunch | 1 | 4 | 0 | 0 | 1 |  | PRG_SAM_SuperPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_SAM_SuperStraightPunch_desc |
| SAM_CHARGESPEED2 | PRG_SAM_ChargeSpeed2 | shieldRicochetCharge | 1 | 4 | 0 | 0 | 0 |  | PRG_ChargeSpeed2_Screen | AV_PRG_RangedCharge2 | PRG_SAM_ChargeSpeed2_desc |
| SAM_HEALTH1 | PRG_SAM_Health1 | health | 50 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SAM_Health1_desc |
| SAM_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| SAM_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| SAM_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| SAM_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| SAM_RUNSPEED2 | PRG_SAM_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_SAM_RunSpeed2_desc |
| SAM_MELEE_DMG1 | PRG_SAM_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SAM_MeleeDmg1_desc |
| SAM_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| SAM_RUNSPEED1 | PRG_SAM_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_SAM_RunSpeed1_desc |
| SAM_RANGED_DMG3 | PRG_SAM_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.25,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SAM_RangedDmg3_desc |
| SAM_BLOCKBREAKER_COMBO | PRG_SAM_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 0 |  | PRG_SAM_BlockBreaker_Screen | AV_PRG_BlockBreaker | PRG_SAM_BlockBreakerCombo_desc |
| SAM_MELEE_DMG3 | PRG_SAM_MeleeDmg3 | meleeDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SAM_MeleeDmg3_desc |
| SAM_SPECIAL_UNLOCKED | PRG_SAM_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 2 | 1 | 0 | 1 |  | PRG_SAM_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_SAM_SpecialUnlocked_desc |
| SAM_CHARGESPEED1 | PRG_SAM_ChargeSpeed1 | shieldRicochetCharge | 1.5 | 2 | 1 | 0 | 0 |  | PRG_SAM_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_SAM_ChargeSpeed1_desc |
| SAM_RANGED_DMG1 | PRG_SAM_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_SAM_RangedDmg1_desc |
| SAM_SUPERJUMP1 | PRG_SAM_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 1 | 0 | 0 | 1 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_SAM_SuperJump1_desc |
| SAM_RICOCHET_BLOCK | PRG_SAM_RicochetBlock | blockRicochet | 1 | 2 | 0 | 0 | 1 |  | PRG_RicochetBlock_Screen | AV_PRG_RicochetBlock | PRG_SAM_RicochetBlock_desc |
| SAM_GROUNDCOMBO6 | PRG_SAM_GroundCombo6 | groundCombo6 | 1 | 3 | 0 | 0 | 0 |  | PRG_SAM_GroundCombo6_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_SAM_GroundCombo6_desc |
| SAM_SHIELDRICOCHET1 | PRG_SAM_ShieldRicochet1 | shieldRicochet,shieldRicochetCharge,ricochetCount | 1,2.0,1.0 | 2 | 0 | 0 | 1 |  | PRG_SAM_ShieldRicochet1_Screen | AV_PRG_RangedChargeUp | PRG_SAM_ShieldRicochet1_desc |
| SAM_SHIELDRICOCHET4 | PRG_SAM_ShieldRicochet4 | ricochetCount | 5 | 6 | 0 | 0 | 0 |  | PRG_SAM_ShieldRicochet1_Screen | AV_PRG_RangedUpgrade3 | PRG_SAM_ShieldRicochet4_desc |
| SAM_GROUNDPOUND2 | PRG_SAM_GroundPound2 | groundpoundDamage,groundpound | 2.0,1.0 | 2 | 1 | 0 | 0 |  | PRG_SAM_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_SAM_GroundPound2_desc |
| SAM_SUPER_DMG1 | PRG_SAM_SuperDmg1 | specialDamage | 0.4 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_SAM_SuperDmg1_desc |
| SAM_SUPER_DMG2 | PRG_SAM_SuperDmg2 | specialDamage | 0.4 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_SAM_SuperDmg2_desc |
| SAM_HEALTH2 | PRG_SAM_Health2 | health | 100 | 2 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_SAM_Health2_desc |
| SAM_MELEE_DMG2 | PRG_SAM_MeleeDmg2 | meleeDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_SAM_MeleeDmg2_desc |
