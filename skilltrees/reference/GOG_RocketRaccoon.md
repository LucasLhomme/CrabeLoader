# GOG_RocketRaccoon

`ProgressionTree = "IN2_GOG_RocketRaccoon"` -- source: `gamedb/core/in2_gog_rocketraccoon.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| ROCKET_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.4 |
| ROCKET_BASERUNSPEED | runspeed | 6.5 |
| ROCKET_BASEHEALTH | health | 150 |
| ROCKET_BASEFIRERATE | cannonFireRate,sharedFireRateMultiplier | 3.5,1.0 |
| ROCKET_BASESUPERJUMP | BM_SuperJumper | 0 |
| ROCKET_BASECLIP | cannonClipSize | 8 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ROCKET_CLIP1 | PRG_RCN_CannonClip1 | cannonClipSize | 16 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_RCN_CannonClip1_desc |
| ROCKET_SUPER_DMG3 | PRG_RCN_SuperDmg3 | specialDamage | 1 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_RCN_SuperDmg3_desc |
| ROCKET_BLOCKBREAKER_COMBO | PRG_RCN_BlockBreakerCombo | blockBreakerCombo | 1 | 4 | 1 | 0 | 1 |  | PRG_RCN_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_RCN_BlockBreakerCombo_desc |
| ROCKET_SPECIAL_UNLOCKED | PRG_RCN_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 2 | 1 | 0 | 1 |  | PRG_RCN_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_RCN_SpecialUnlocked_desc |
| ROCKET_CHARGESPEED1 | PRG_RCN_ChargeSpeed1 | quantumBeamCharge | 1.5 | 5 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_RCN_ChargeSpeed1_desc |
| ROCKET_HEALTH1 | PRG_RCN_Health1 | health | 50 | 2 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RCN_Health1_desc |
| ROCKET_RANGED_DMG3 | PRG_RCN_RangedDmg3 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.125,0.25,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RCN_RangedDmg3_desc |
| ROCKET_DUALWIELD | PRG_RCN_DualWield | cannonDualWield,multiShotDamage | 1, 0.625 | 1 | 1 | 0 | 1 |  | PRG_RCN_DualWield_Screen | AV_PRG_DualWieldUnlock | PRG_RCN_DualWield_desc |
| ROCKET_MELEE_DMG1 | PRG_RCN_MeleeDmg1 | meleeDamage | 0.7 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_RCN_MeleeDmg1_desc |
| ROCKET_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| ROCKET_SUPERJUMP1 | PRG_RCN_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 1 | 0 | 0 | 1 |  | PRG_RCN_SuperJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_RCN_SuperJump1_desc |
| ROCKET_RUNSPEED1 | PRG_RCN_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_RCN_RunSpeed1_desc |
| ROCKET_GROUNDPOUND2 | PRG_RCN_GroundPound2 | groundpound,groundpoundDamage | 1.0,4.0 | 3 | 0 | 0 | 0 |  | PRG_RCN_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_RCN_GroundPound2_desc |
| ROCKET_SUPER_DMG2 | PRG_RCN_SuperDmg2 | specialDamage | 0.65 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_RCN_SuperDmg2_desc |
| ROCKET_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| ROCKET_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_RocketRacoon |  |
| ROCKET_RANGED_DMG2 | PRG_RCN_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.075,0.15,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RCN_RangedDmg2_desc |
| ROCKET_RANGED_DMG1 | PRG_RCN_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.05,0.1,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_RCN_RangedDmg1_desc |
| ROCKET_FIRERATE1 | PRG_RCN_FireRate1 | cannonFireRate,sharedFireRateMultiplier | 4.0,1.5 | 4 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_RCN_FireRate1_desc |
| ROCKET_SUPERJUMP2 | PRG_RCN_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,42.5,10,12 | 5 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_RCN_SuperJump2_desc |
| ROCKET_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| ROCKET_HEALTH2 | PRG_RCN_Health2 | health | 100 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_RCN_Health2_desc |
| ROCKET_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| ROCKET_CLIP2 | PRG_RCN_CannonClip2 | cannonClipSize | -1 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_RCN_CannonClip2_desc |
| ROCKET_SUPER_DMG1 | PRG_RCN_SuperDmg1 | specialDamage | 0.35 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_RCN_SuperDmg1_desc |
| ROCKET_QUANTUMBEAM | PRG_RCN_QuantumBeam | quantumBeam,quantumBeamCharge | 1,2.0 | 4 | 1 | 0 | 1 |  | PRG_RCN_QuantumBeam_Screen | AV_PRG_RangedChargeUp | PRG_RCN_QuantumBeam_desc |
| ROCKET_CHARGESPEED2 | PRG_RCN_ChargeSpeed2 | quantumBeamCharge | 1 | 6 | 0 | 0 | 0 |  | PRG_ChargeSpeed2_Screen | AV_PRG_RangedCharge2 | PRG_RCN_ChargeSpeed2_desc |
| ROCKET_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
