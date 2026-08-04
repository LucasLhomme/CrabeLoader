# FRO_Olaf

`ProgressionTree = "IN3_FRO_Olaf"` -- source: `gamedb/core/in3_fro_olaf.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| OLAF_BASE_BLOCKBREAKER | blockBreakerCombo,blockBreakerRunTime | 1.0,2.5 |
| OLAF_BASEPARRY | MeleeParry | 0 |
| OLAF_BASE_HEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| OLAF_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| OLAF_PARRY | PRG_OLF_Parry | repel,MeleeParry | 1,1 | 2 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_OLF_Parry_desc |
| OLAF_RANGED_DMG1 | PRG_OLF_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.2,0.1,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_OLF_RangedDmg1_desc |
| OLAF_RANGED_DMG2 | PRG_OLF_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.3,0.15,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_OLF_RangedDmg2_desc |
| OLAF_POWERFILL1 | PRG_PowerFill1 | powerFill | 95 | 2 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| OLAF_HEALTH2 | PRG_OLF_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_OLF_Health2_desc |
| OLAF_BLOCKBREAKER_RUN2 | PRG_OLF_BlockBreakerRun2 | blockBreakerRunTime | 5 | 6 | 0 | 0 | 0 |  | PRG_OlafRun2_Screen | AV_PRG_OlafRun2 | PRG_OLF_BlockBreakerRun2_desc |
| OLAF_BLOCKBREAKER_RUN1 | PRG_OLF_BlockBreakerRun1 | blockBreakerRunTime | 3.5 | 2 | 0 | 0 | 0 |  | PRG_OlafRun1_Screen | AV_PRG_OlafRun1 | PRG_OLF_BlockBreakerRun1_desc |
| OLAF_MELEE_DMG3 | PRG_OLF_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_OLF_MeleeDmg3_desc |
| OLAF_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| OLAF_HEALTH3 | PRG_OLF_Health3 | health | 70 | 5 | 0 |  | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_OLF_Health3_desc |
| OLAF_SPECIAL_AOE2 | PRG_OLF_PowerAOE2 | specialAOE,specialAreaUpgrade | 12,2 | 6 | 0 | 0 | 0 |  | PRG_SuperMoveAOE2_Screen | AV_PRG_SuperMoveAOE2 | PRG_OLF_PowerAOE2_desc |
| OLAF_SPECIAL_DMG2 | PRG_OLF_SuperDmg2 | specialDamage | 0.4 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_OLF_SuperDmg2_desc |
| OLAF_CARROTRICOCHET1 | PRG_OLF_RangedRicochet1 | carrotRicochet,ricochetCount | 1,2.0 | 2 | 0 | 0 | 0 |  | PRG_RangedUpgrade1_Screen | AV_PRG_RangedUpgrade1 | PRG_OLF_RangedRicochet1_desc |
| OLAF_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Jasmine |  |
| OLAF_SPECIAL_DMG3 | PRG_OLF_SuperDmg3 | specialDamage | 0.7 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_OLF_SuperDmg3_desc |
| OLAF_HEALTH1 | PRG_OLF_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_OLF_Health1_desc |
| OLAF_GROUNDPOUND2 | PRG_OLF_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 1 | 0 | 0 |  | PRG_OLF_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_OLF_GroundPound2_desc |
| OLAF_CHARGED_UNLOCK | PRG_OLF_ChargeUnlocked | allowRangedCharge,rangedChargeTime | 1,1.5 | 2 | 1 | 0 | 1 |  | PRG_OLF_ChargeUnlocked_Screen | AV_PRG_RangedChargeUp | PRG_OLF_ChargeUnlocked_desc |
| OLAF_SPECIAL_AOE1 | PRG_OLF_PowerAOE1 | specialAOE,specialAreaUpgrade | 8,1 | 4 | 0 | 0 | 0 |  | PRG_SuperMoveAOE1_Screen | AV_PRG_SuperMoveAOE1 | PRG_OLF_PowerAOE1_desc |
| OLAF_SPECIAL_UNLOCKED | PRG_OLF_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 1 | 1 | 0 | 0 |  | PRG_OLF_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_OLF_SpecialUnlocked_desc |
| OLAF_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| OLAF_PARRY_EFFECT | PRG_OLF_Parry | repelFreezeEffect | 1 | 4 | 0 | 0 | 0 |  | PRG_ParryCounter_Screen | AV_PRG_ParryCounter | PRG_OLF_Parry_desc |
| OLAF_ATHLETICJUMP | PRG_OLF_AthleticJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 2 | 1 | 0 | 0 |  | PRG_OLF_AthleticJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_OLF_AthleticJump1_desc |
| OLAF_MELEE_DMG1 | PRG_OLF_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_OLF_MeleeDmg1_desc |
| OLAF_CHARGESPEED1 | PRG_OLF_ChargeTime1 | rangedChargeTime | 1.15 | 4 | 0 | 0 | 0 |  | PRG_RangedCharge1_Screen | AV_PRG_RangedCharge1 | PRG_OLF_ChargeTime1_desc |
| OLAF_CHARGESPEED2 | PRG_OLF_ChargeTime2 | rangedChargeTime | 0.75 | 6 | 0 | 0 | 0 |  | PRG_RangedCharge2_Screen | AV_PRG_RangedCharge2 | PRG_OLF_ChargeTime2_desc |
| OLAF_SPECIAL_DMG1 | PRG_OLF_SuperDmg1 | specialDamage | 0.4 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_OLF_SuperDmg1_desc |
| OLAF_CARROTRICOCHET2 | PRG_OLF_RangedRicochet2 | ricochetCount | 4 | 4 | 0 | 0 | 0 |  | PRG_RangedUpgrade2_Screen | AV_PRG_RangedUpgrade2 | PRG_OLF_RangedRicochet2_desc |
| OLAF_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| OLAF_RANGED_DMG3 | PRG_OLF_RangedDmg3 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.5,0.25,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_OLF_RangedDmg3_desc |
| OLAF_PAUSECOMBO1 | PRG_OLF_PauseCombo1 | groundCombo4,groundCombo5,groundCombo6 | 1,1,1 | 4 | 0 | 0 | 0 |  | PRG_OLF_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_OLF_PauseCombo1_desc |
| OLAF_MELEE_DMG2 | PRG_OLF_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_OLF_MeleeDmg2_desc |
