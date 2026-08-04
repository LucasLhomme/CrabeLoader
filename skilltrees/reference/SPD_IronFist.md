# SPD_IronFist

`ProgressionTree = "IN2_SPD_IronFist"` -- source: `gamedb/core/in2_spd_ironfist.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| IRONFIST_BASESUPERJUMP | BM_SuperJumper | 0 |
| IRONFIST_BASERUNSPEED | runspeed | 6.5 |
| IRONFIST_BASEHEALTHREGEN | healthRegen | 0 |
| IRONFIST_BASECHISTATE | chiState | 0 |
| IRONFIST_BASEHEALTH | health | 150 |
| IRONFIST_CHARGED_FIST0 | fistChargeLevels,fistLevelChargeTime | 0,0.0 |
| IRONFIST_BASESTRENGTH | BM_SuperDuperStrong | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| IRONFIST_CHARGED_FIST2 | PRG_IFS_RangedCharge2 | fistChargeLevels,fistLevelChargeTime | 2,0.75 | 4 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_IFS_RangedCharge2_desc |
| IRONFIST_SPECIAL_UNLOCKED | PRG_IFS_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 2 | 1 | 0 | 1 |  | PRG_IFS_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_IFS_SpecialUnlocked_desc |
| IRONFIST_HEALAURA_BLOCK | PRG_IFS_HealAuraBlock | blockHealthRegen,blockHealthDelay,blockHealthAura | 10.0,1.0,1.0 | 4 | 0 | 0 | 1 |  | PRG_IFS_HealAuraBlock_Screen | AV_PRG_HealAuraBlock | PRG_IFS_HealAuraBlock_desc |
| IRONFIST_GROUNDCOMBO6 | PRG_IFS_GroundCombo6 | groundCombo6 | 1 | 4 | 0 | 0 | 0 |  | PRG_IFS_GroundCombo6_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_IFS_GroundCombo6_desc |
| IRONFIST_REPEL | PRG_IFS_Repel | repel,repelSuperBonus | 1,30.0 | 3 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_IFS_Repel_desc |
| IRONFIST_RUNSPEED2 | PRG_IFS_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_IFS_RunSpeed2_desc |
| IRONFIST_RUNSPEED1 | PRG_IFS_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_IFS_RunSpeed1_desc |
| IRONFIST_RANGED_DMG1 | PRG_IFS_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_IFS_RangedDmg1_desc |
| IRONFIST_HEALING_BLOCK | PRG_IFS_HealingBlock | blockHealthRegen,blockHealthDelay | 5.0,1.0 | 2 | 1 | 0 | 0 |  | PRG_IFS_HealingBlock_Screen | AV_PRG_HealingBlock | PRG_IFS_HealingBlock_desc |
| IRONFIST_CHI_STRENGTH | PRG_IFS_ChiStrength | BM_SuperDuperStrong | 1 | 4 | 0 | 0 | 1 |  | PRG_IFS_ChiStrength_Screen | AV_PRG_PickupThrowUpgrade1 | PRG_IFS_ChiStrength_desc |
| IRONFIST_RANGED_DMG3 | PRG_IFS_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.25,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_IFS_RangedDmg3_desc |
| IRONFIST_BLOCKBREAKER_COMBO | PRG_IFS_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 0 |  | PRG_IFS_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_IFS_BlockBreakerCombo_desc |
| IRONFIST_CHI_STATE3 | PRG_IFS_ChiState3 | chiDuration | 20 | 6 | 0 | 0 | 0 |  | PRG_IFS_ChiState1_Screen | AV_PRG_ChiState3 | PRG_IFS_ChiState3_desc |
| IRONFIST_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| IRONFIST_RANGED_DMG2 | PRG_IFS_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_IFS_RangedDmg2_desc |
| IRONFIST_SUPER_DMG1 | PRG_IFS_SuperDmg1 | specialDamage | 0.3 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_IFS_SuperDmg1_desc |
| IRONFIST_CHI_STATE2 | PRG_IFS_ChiState2 | chiDuration | 15 | 5 | 0 | 0 | 0 |  | PRG_IFS_ChiState1_Screen | AV_PRG_ChiState2 | PRG_IFS_ChiState2_desc |
| IRONFIST_CHI_STATE1 | PRG_IFS_ChiState1 | chiDuration | 10 | 2 | 1 | 0 | 1 |  | PRG_IFS_ChiState1_Screen | AV_PRG_ChiState1 | PRG_IFS_ChiState1_desc |
| IRONFIST_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| IRONFIST_HEIGHTENED_SENSE | PRG_IFS_HeightenedSense | heightenedSense,BM_HeightenedSenses | 1.0,1.0 | 5 | 0 | 0 | 1 |  | PRG_IFS_HeightenedSense_Screen | AV_PRG_SpideySenseUnlock | PRG_IFS_HeightenedSense_desc |
| IRONFIST_GROUNDPOUND2 | PRG_IFS_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 3 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_IFS_GroundPound2_desc |
| IRONFIST_SUPER_STRAIGHT_PUNCH | PRG_IFS_SuperStraightPunch | superStraightPunch | 1 | 4 | 0 | 0 | 0 |  | PRG_IFS_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_IFS_SuperStraightPunch_desc |
| IRONFIST_SUPERJUMP | PRG_IFS_SuperJump | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,44,10,12 | 2 | 0 | 0 | 0 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_IFS_SuperJump_desc |
| IRONFIST_CHARGED_FIST1 | PRG_IFS_RangedCharge1 | fistChargeLevels,fistLevelChargeTime | 2,1.25 | 1 | 0 | 0 | 0 |  | PRG_IFS_RangedCharge1_Screen | AV_PRG_RangedChargeUp | PRG_IFS_RangedCharge1_desc |
| IRONFIST_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| IRONFIST_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| IRONFIST_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| IRONFIST_SUPER_DMG3 | PRG_IFS_SuperDmg3 | specialDamage | 0.75 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_IFS_SuperDmg3_desc |
| IRONFIST_SUPER_DMG2 | PRG_IFS_SuperDmg2 | specialDamage | 0.45 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_IFS_SuperDmg2_desc |
| IRONFIST_HEALTH2 | PRG_IFS_Health2 | health | 100 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_IFS_Health2_desc |
| IRONFIST_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_IronFist |  |
| IRONFIST_HEALTH1 | PRG_IFS_Health1 | health | 50 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_IFS_Health1_desc |
