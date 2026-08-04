# AL_Jasmine

`ProgressionTree = "IN2_AL_Jasmine"` -- source: `gamedb/core/in2_al_jasmine.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| JASMINE_BASE_RUNSPEED | runspeed | 6.5 |
| JASMINE_LEDGE_OBB | ledgeMinY,ledgeMaxY,ledgeJumpMinY,ledgeJumpMaxY | 1.0,2.4,1.5,2.3 |
| JASMINE_BASE_GROUNDPOUND | groundpound | 0 |
| JASMINE_BASE_SPINTIME | SpinTime | 30 |
| JASMINE_BASE_HEALTH | health | 150 |
| JASMINE_BASE_SUPERJUMP | BM_SuperJumper | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| JASMINE_HEALTH4 | PRG_JAS_Health4 | health | 60 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_JAS_Health4_desc |
| JASMINE_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| JASMINE_SUPER_STRAIGHT_PUNCH | PRG_JAS_SuperStraightPunch | superStraightPunch | 1 | 4 | 0 | 0 | 0 |  | PRG_JAS_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_JAS_SuperStraightPunch_desc |
| JASMINE_CHARGEDATTACK | PRG_JAS_ChargeUnlocked | tornadoChargedAttack,tornadoChargeTime | 1,2 | 2 | 1 | 0 | 0 |  | PRG_JAS_ChargeUnlocked_Screen | AV_PRG_RangedChargeUp | PRG_JAS_ChargeUnlocked_desc |
| JASMINE_RANGED_LONGSPIN | PRG_JAS_WhipStunTime1 | SpinTime | 60 | 5 | 1 | 0 | 1 |  | PRG_JAS_WhirlwindStun_Screen | AV_PRG_WhirlwindStun | PRG_JAS_WhipStunTime1_desc |
| JASMINE_HEALTH2 | PRG_JAS_Health2 | health | 30 | 2 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_JAS_Health2_desc |
| JASMINE_RUNSPEED2 | PRG_JAS_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_JAS_RunSpeed2_desc |
| JASMINE_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| JASMINE_SUPERJUMP1 | PRG_JAS_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 3 | 0 | 0 | 1 |  | PRG_JAS_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_JAS_SuperJump1_desc |
| JASMINE_RUNSPEED1 | PRG_JAS_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_JAS_RunSpeed1_desc |
| JASMINE_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| JASMINE_MELEE_DMG1 | PRG_JAS_MeleeDmg1 | meleeDamage | 0.3 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JAS_MeleeDmg1_desc |
| JASMINE_BLOCK_METER | PRG_JAS_BuildMeterBlock | repelSuperBonus | 30 | 4 | 0 | 0 | 0 |  | PRG_RepelSuperMeter_Screen | AV_PRG_RepelSuperMeter | PRG_JAS_BuildMeterBlock_desc |
| JASMINE_RANGED_CHARGETIME2 | PRG_JAS_ChargeSpeed2 | tornadoChargeTime | 1.2 | 6 | 0 | 0 | 0 |  | PRG_ChargeSpeed2_Screen | AV_PRG_RangedCharge2 | PRG_JAS_ChargeSpeed2_desc |
| JASMINE_SPECIAL_DMG3 | PRG_JAS_SpecialDmg3 | specialDamage | 0.5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_JAS_SpecialDmg3_desc |
| JASMINE_BLOCKBREAKER_COMBO | PRG_JAS_BlockBreakerCombo | blockBreakerCombo | 1 | 3 | 1 | 0 | 1 |  | PRG_JAS_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_JAS_BlockBreakerCombo_desc |
| JASMINE_HEALTH1 | PRG_JAS_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_JAS_Health1_desc |
| JASMINE_SPECIAL_DMG2 | PRG_JAS_SpecialDmg2 | specialDamage | 0.3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_JAS_SpecialDmg2_desc |
| JASMINE_MELEE_DMG2 | PRG_JAS_MeleeDmg2 | meleeDamage | 0.5 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JAS_MeleeDmg2_desc |
| JASMINE_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Jasmine |  |
| JASMINE_GROUNDPOUND2 | PRG_JAS_GroundPound2 | groundpound, groundPoundDamage | 1.00,2.00 | 2 | 0 | 0 | 0 |  | PRG_JAS_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_JAS_GroundPound2_desc |
| JASMINE_SPECIAL_DMG1 | PRG_JAS_SpecialDmg1 | specialDamage | 0.2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_JAS_SpecialDmg1_desc |
| JASMINE_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| JASMINE_RANGED_DMG1 | PRG_JAS_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.35,0.15,0.75 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JAS_RangedDmg1_desc |
| JASMINE_GROUNDCOMBO5 | PRG_JAS_GroundCombo5 | groundCombo5 | 1 | 4 | 0 | 0 | 0 |  | PRG_JAS_GroundCombo5_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_JAS_GroundCombo5_desc |
| JASMINE_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| JASMINE_SPECIAL_UNLOCK | PRG_JAS_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 2 | 1 | 0 | 1 |  | PRG_JAS_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_JAS_SpecialUnlocked_desc |
| JASMINE_RANGED_CHARGETIME1 | PRG_JAS_ChargeSpeed1 | tornadoChargeTime | 1.7 | 4 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_JAS_ChargeSpeed1_desc |
| JASMINE_RANGED_DMG2 | PRG_JAS_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.65,0.35,1.0 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JAS_RangedDmg2_desc |
| JASMINE_HEALTH3 | PRG_JAS_Health3 | health | 30 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_JAS_Health3_desc |
| JASMINE_SUPERJUMP2 | PRG_JAS_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,44,10,12 | 6 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_JAS_SuperJump2_desc |
