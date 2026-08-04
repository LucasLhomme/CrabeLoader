# AL_Aladdin

`ProgressionTree = "IN2_AL_Aladdin"` -- source: `gamedb/core/in2_al_aladdin.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| ALADDIN_BASEHEALTH | health | 150 |
| ALADDIN_SWORDCHARGE | swordChargeTime | 1 |
| ALADDIN_BASESUPERJUMP | BM_SuperJumper | 0 |
| ALADDIN_BASERUNSPEED | runspeed | 6.5 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ALADDIN_HEALTH1 | PRG_ALD_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_ALD_Health1_desc |
| ALADDIN_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| ALADDIN_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| ALADDIN_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Aladdin |  |
| ALADDIN_RUNSPEED2 | PRG_ALD_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_ALD_RunSpeed2_desc |
| ALADDIN_SUPERJUMP1 | PRG_ALD_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 2 | 0 | 0 | 1 |  | PRG_ALD_SuperJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_ALD_SuperJump1_desc |
| ALADDIN_HEALTH2 | PRG_ALD_Health2 | health | 30 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_ALD_Health2_desc |
| ALADDIN_HEALTH3 | PRG_ALD_Health3 | health | 30 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_ALD_Health3_desc |
| ALADDIN_RANGED_DMG1 | PRG_ALD_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.2,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ALD_RangedDmg1_desc |
| ALADDIN_MELEE_DMG3 | PRG_ALD_MeleeDmg3 | meleeDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ALD_MeleeDmg3_desc |
| ALADDIN_GROUNDCOMBO6 | PRG_ALD_GroundCombo6 | groundCombo6 | 1 | 4 | 0 | 0 | 0 |  | PRG_ALD_GroundCombo6_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_ALD_GroundCombo6_desc |
| ALADDIN_GROUNDPOUND2 | PRG_ALD_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 1 | 1 | 0 | 0 |  | PRG_ALD_GroundPound2_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_ALD_GroundPound2_desc |
| ALADDIN_BLOCKBREAKER_COMBO | PRG_ALD_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 0 |  | PRG_ALD_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_ALD_BlockBreakerCombo_desc |
| ALADDIN_RUNSPEED1 | PRG_ALD_RunSpeed1 | runspeed | 7.5 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_ALD_RunSpeed1_desc |
| ALADDIN_SUPERJUMP2 | PRG_ALD_SuperJump2 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,44,10,12 | 5 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_ALD_SuperJump2_desc |
| ALADDIN_RICOCHET_BLOCK | PRG_ALD_RicochetBlock | blockRicochet | 1 | 5 | 1 | 0 | 1 |  | PRG_ALD_RicochetBlock_Screen | AV_PRG_RicochetBlock | PRG_ALD_RicochetBlock_desc |
| ALADDIN_POWERAREA1 | PRG_ALD_PowerArea1 | powerArea | 12 | 4 | 0 | 0 | 0 |  | PRG_SuperMoveUpgrade1_Screen | AV_PRG_SuperMoveUpgrade1 | PRG_ALD_PowerArea1_desc |
| ALADDIN_POWERFILL1 | PRG_PowerFill1 | powerFill | 115 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| ALADDIN_SPECIAL_UNLOCKED | PRG_ALD_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,150.0 | 2 | 1 | 0 | 1 |  | PRG_ALD_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_ALD_SpecialUnlocked_desc |
| ALADDIN_RANGED_DMG3 | PRG_ALD_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.5,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ALD_RangedDmg3_desc |
| ALADDIN_RANGED_DMG2 | PRG_ALD_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.3,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ALD_RangedDmg2_desc |
| ALADDIN_CHARGESPEED2 | PRG_ALD_ChargeSpeed2 | swordCharge | 0.75 | 6 | 0 | 0 | 0 |  | PRG_ChargeSpeed2_Screen | AV_PRG_RangedCharge2 | PRG_ALD_ChargeSpeed2_desc |
| ALADDIN_HEALTH4 | PRG_ALD_Health4 | health | 60 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_ALD_Health4_desc |
| ALADDIN_CHARGESPEED1 | PRG_ALD_ChargeSpeed1 | swordCharge | 1.75 | 4 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_ALD_ChargeSpeed1_desc |
| ALADDIN_SUPER_STRAIGHT_PUNCH | PRG_ALD_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 1 |  | PRG_ALD_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_ALD_SuperStraightPunch_desc |
| ALADDIN_SWORDCHARGE1 | PRG_ALD_SwordCharge1 | swordCharge | 2.5 | 2 | 0 | 0 | 1 |  | PRG_ALD_SwordCharge1_Screen | AV_PRG_RangedChargeUp | PRG_ALD_SwordCharge1_desc |
| ALADDIN_MELEE_DMG1 | PRG_ALD_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ALD_MeleeDmg1_desc |
| ALADDIN_POWERAREA2 | PRG_ALD_PowerArea2 | powerArea | 15 | 6 | 0 | 0 | 0 |  | PRG_SuperMoveUpgrade2_Screen | AV_PRG_SuperMoveUpgrade2 | PRG_ALD_PowerArea2_desc |
| ALADDIN_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| ALADDIN_MELEE_DMG2 | PRG_ALD_MeleeDmg2 | meleeDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ALD_MeleeDmg2_desc |
| ALADDIN_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
