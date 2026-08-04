# GOG_Ronan

`ProgressionTree = "IN2_GOG_Ronan"` -- source: `gamedb/core/in2_gog_ronan.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| RONAN_BASE_GROUNDPOUND | groundpound | 0 |
| RONAN_BASE_POWERDURATION | powerDuration | 12 |
| RONAN_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| RONAN_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.6 |
| RONAN_BASE_HEALTH | health | 150 |
| RONAN_BASE_RUNSPEED | runspeed | 6.5 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| RONAN_RANGED_DMG2 | PRG_RON_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.075,0.15,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RON_RangedDmg2_desc |
| RONAN_RANGED_DMG1 | PRG_RON_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.05,0.1,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RON_RangedDmg1_desc |
| RONAN_BLOCKBREAKER_COMBO | PRG_RON_BlockBreakerCombo | blockBreakerCombo | 1 | 3 | 0 | 0 | 1 |  | PRG_RON_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_RON_BlockBreakerCombo_desc |
| RONAN_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| RONAN_BLOCK_RICOCHET | PRG_RON_RicochetBlock | blockRicochet | 1 | 4 | 0 | 0 | 0 |  | PRG_RicochetBlock_Screen | AV_PRG_RicochetBlock | PRG_RON_RicochetBlock_desc |
| RONAN_RANGED_CHARGETIME2 | PRG_RON_ChargeTime2 | pulseChargeTime | 0.75 | 6 | 0 | 0 | 0 |  | PRG_ChargeSpeed2_Screen | AV_PRG_RangedCharge2 | PRG_RON_ChargeTime2_desc |
| RONAN_SUPER_STRAIGHT_PUNCH | PRG_RON_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 0 |  | PRG_RON_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_RON_SuperStraightPunch_desc |
| RONAN_MELEE_DMG1 | PRG_RON_MeleeDmg1 | meleeDamage | 0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RON_MeleeDmg1_desc |
| RONAN_GROUNDPOUND2 | PRG_RON_GroundPound2 | groundpound,groundpoundDamage | 1.00,2.0 | 3 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_RON_GroundPound2_desc |
| RONAN_GROUNDCOMBO5 | PRG_RON_GroundCombo5 | groundCombo5 | 1 | 4 | 1 | 0 | 0 |  | PRG_RON_GroundCombo5_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_RON_GroundCombo5_desc |
| RONAN_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Ronan |  |
| RONAN_BLOCK_METER | PRG_RON_BuildMeterBlock | repelSuperBonus | 30 | 6 | 0 | 0 | 0 |  | PRG_RepelSuperMeter_Screen | AV_PRG_RepelSuperMeter | PRG_RON_BuildMeterBlock_desc |
| RONAN_HEALTH2 | PRG_RON_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RON_Health2_desc |
| RONAN_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| RONAN_RUNSPEED1 | PRG_RON_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_RON_RunSpeed1_desc |
| RONAN_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| RONAN_SUPERJUMP | PRG_RON_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,44,10,12 | 2 | 0 | 0 | 1 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_RON_SuperJump1_desc |
| RONAN_RANGED_CHARGETIME1 | PRG_RON_ChargeTime1 | pulseChargeTime | 1.5 | 5 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_RON_ChargeTime1_desc |
| RONAN_HEALTH1 | PRG_RON_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RON_Health1_desc |
| RONAN_SUPERSTRENGTH | PRG_RON_SuperStrength | BM_SuperDuperStrong | 1 | 4 | 0 | 0 | 0 |  | PRG_StrengthUpgrade_Screen | AV_PRG_PickupThrowUpgrade1 | PRG_RON_SuperStrength_desc |
| RONAN_POWERDURATION2 | PRG_RON_PowerDuration2 | powerDuration | 30 | 6 | 0 | 0 | 0 |  | PRG_SuperMoveDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_RON_PowerDuration2_desc |
| RONAN_POWERDURATION1 | PRG_RON_PowerDuration1 | powerDuration | 20 | 4 | 0 | 0 | 0 |  | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_RON_PowerDuration1_desc |
| RONAN_CHARGEDATTACK | PRG_RON_ChargeUnlocked | allowPulseCharge,pulseChargeTime | 1, 2 | 3 | 1 | 0 | 1 |  | PRG_RON_ChargeUnlocked_Screen | AV_PRG_RangedChargeUp | PRG_RON_ChargeUnlocked_desc |
| RONAN_SPECIAL_UNLOCK | PRG_RON_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDuration | 1.0,1.0,120.0,10.0 | 2 | 1 | 0 | 1 |  | PRG_RON_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_RON_SpecialUnlocked_desc |
| RONAN_HEALTH3 | PRG_RON_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RON_Health3_desc |
| RONAN_MELEE_DMG3 | PRG_RON_MeleeDmg3 | meleeDamage | 0.35 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RON_MeleeDmg3_desc |
| RONAN_RANGED_FIRERATE1 | PRG_RON_FireRate1 | univWeaponFirerate | 4 | 4 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_RON_FireRate1_desc |
| RONAN_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| RONAN_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| RONAN_RANGED_DMG3 | PRG_RON_RangedDmg3 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.125,0.25,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RON_RangedDmg3_desc |
| RONAN_MELEE_DMG2 | PRG_RON_MeleeDmg2 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RON_MeleeDmg2_desc |
