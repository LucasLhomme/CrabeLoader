# TB_DonaldDuck

`ProgressionTree = "IN2_TB_DonaldDuck"` -- source: `gamedb/core/in2_tb_donaldduck.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| DONALDDUCK_BASEHEALTH | health | 150 |
| DONALDDUCK_BASERUNSPEED | runspeed | 6.5 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| DONALDDUCK_SUPER_DMG1 | PRG_DLD_SuperDmg1 | specialDamage | 0.35 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_DLD_SuperDmg1_desc |
| DONALDDUCK_RUNSPEED2 | PRG_DLD_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_DLD_RunSpeed2_desc |
| DONALDDUCK_SUPERJUMP2 | PRG_DLD_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,44,10,12 | 6 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_DLD_SuperJump2_desc |
| DONALDDUCK_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| DONALDDUCK_HEALTH3 | PRG_DLD_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_DLD_Health3_desc |
| DONALDDUCK_BLOCKBREAKER_COMBO | PRG_DLD_BlockBreakerCombo | blockBreakerCombo | 1 | 3 | 0 | 0 | 0 |  | PRG_DLD_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_DLD_BlockBreakerCombo_desc |
| DONALDDUCK_MELEE_DMG2 | PRG_DLD_MeleeDmg2 | meleeDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DLD_MeleeDmg2_desc |
| DONALDDUCK_RANGED_DMG3 | PRG_DLD_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.5,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DLD_RangedDmg3_desc |
| DONALDDUCK_GROUNDPOUND2 | PRG_DLD_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 3 | 1 | 0 | 0 |  | PRG_DLD_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_DLD_GroundPound2_desc |
| DONALDDUCK_SPECIAL_UNLOCKED | PRG_DLD_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDuration,poweredUpRunSpeed | 1.0,1.0,120.0,8.0,10 | 2 | 1 | 0 | 1 |  | PRG_DLD_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_DLD_SpecialUnlocked_desc |
| DONALDDUCK_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| DONALDDUCK_SUPER_DMG2 | PRG_DLD_SuperDmg2 | specialDamage | 0.65 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_DLD_SuperDmg2_desc |
| DONALDDUCK_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| DONALDDUCK_MELEE_DMG3 | PRG_DLD_MeleeDmg3 | meleeDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DLD_MeleeDmg3_desc |
| DONALDDUCK_RANGED_DMG1 | PRG_DLD_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.2,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DLD_RangedDmg1_desc |
| DONALDDUCK_CHARGEDTHROW | PRG_DLD_ChargedThrow | allowThrowCharge | 1 | 1 | 1 | 0 | 1 |  | PRG_DLD_ChargedThrow_Screen | AV_PRG_RangedChargeUp | PRG_DLD_ChargedThrow_desc |
| DONALDDUCK_SUPER_STRAIGHT_PUNCH | PRG_DLD_SuperStraightPunch | superStraightPunch | 1 | 4 | 0 | 0 | 0 |  | PRG_DLD_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_DLD_SuperStraightPunch_desc |
| DONALDDUCK_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| DONALDDUCK_MELEE_DMG1 | PRG_DLD_MeleeDmg1 | meleeDamage | 0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_DLD_MeleeDmg1_desc |
| DONALDDUCK_CHARGESPEED2 | PRG_DLD_ChargeSpeed2 | throwCharge | 0.8 | 6 | 0 | 0 | 0 |  | PRG_ChargeSpeed2_Screen | AV_PRG_RangedCharge2 | PRG_DLD_ChargeSpeed2_desc |
| DONALDDUCK_CHARGESPEED1 | PRG_DLD_ChargeSpeed1 | throwCharge | 0.9 | 4 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_DLD_ChargeSpeed1_desc |
| DONALDDUCK_HEALTH1 | PRG_DLD_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_DLD_Health1_desc |
| DONALDDUCK_HEALTH2 | PRG_DLD_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_DLD_Health2_desc |
| DONALDDUCK_FIRERATE1 | PRG_DLD_FireRate1 | throwFireRate | 7 | 4 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_DLD_FireRate1_desc |
| DONALDDUCK_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| DONALDDUCK_RANGED_DMG2 | PRG_DLD_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.3,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_DLD_RangedDmg2_desc |
| DONALDDUCK_RUNSPEED1 | PRG_DLD_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_DLD_RunSpeed1_desc |
| DONALDDUCK_SUPERJUMP1 | PRG_DLD_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 2 | 0 | 0 | 0 |  | PRG_DLD_SuperJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_DLD_SuperJump1_desc |
| DONALDDUCK_GROUNDCOMBO4 | PRG_DLD_GroundCombo4 | groundCombo4 | 1 | 4 | 1 | 0 | 1 |  | PRG_DLD_GroundCombo4_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_DLD_GroundCombo4_desc |
| DONALDDUCK_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_DonaldDuck |  |
