# BHS_Hiro

`ProgressionTree = "IN2_BHS_Hiro"` -- source: `gamedb/core/in2_bhs_hiro.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| HIRO_BASEHEALTH | health | 150 |
| HIRO_SWARMBOTS1 | chargeSwarmBots | 1 |
| HIRO_BASERUNSPEED | runspeed | 6.5 |
| HIRO_BASESUPERJUMP | BM_SuperJumper | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| HIRO_SUPER_DMG3 | PRG_HRR_SuperDmg3 | specialDamage | 1 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_HRR_SuperDmg3_desc |
| HIRO_BLOCKBREAKER_COMBO | PRG_HRR_BlockBreakerCombo | blockBreakerCombo | 1 | 3 | 1 | 0 | 0 |  | PRG_HRR_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_HRR_BlockBreakerCombo_desc |
| HIRO_RANGED_DMG3 | PRG_HRR_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.5,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HRR_RangedDmg3_desc |
| HIRO_HEALTH3 | PRG_HRR_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_HRR_Health3_desc |
| HIRO_MELEE_DMG1 | PRG_HRR_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HRR_MeleeDmg1_desc |
| HIRO_RUNSPEED2 | PRG_HRR_RunSpeed2 | runspeed | 8.5 | 4 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_HRR_RunSpeed2_desc |
| HIRO_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| HIRO_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| HIRO_SWARMBOTS3 | PRG_HRR_SwarmUpgrade3 | swarmBotFinisher | 1 | 5 | 0 | 0 | 0 |  | PRG_HRR_SwarmUpgrade3_Screen | AV_PRG_RangedUpgrade2 | PRG_HRR_SwarmUpgrade3_desc |
| HIRO_GROUNDCOMBO5 | PRG_HRR_GroundCombo5 | groundCombo5 | 1 | 5 | 1 | 0 | 1 |  | PRG_HRR_GroundCombo5_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_HRR_GroundCombo5_desc |
| HIRO_SUPERJUMP2 | PRG_HRR_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 12.4,0.6,44,10,12 | 5 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_HRR_SuperJump2_desc |
| HIRO_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Hiro |  |
| HIRO_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| HIRO_MELEE_DMG2 | PRG_HRR_MeleeDmg2 | meleeDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HRR_MeleeDmg2_desc |
| HIRO_SUPER_DMG1 | PRG_HRR_SuperDmg1 | specialDamage | 0.35 | 3 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_HRR_SuperDmg1_desc |
| HIRO_RANGED_DMG1 | PRG_HRR_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.2,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HRR_RangedDmg1_desc |
| HIRO_RANGED_DMG2 | PRG_HRR_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.3,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HRR_RangedDmg2_desc |
| HIRO_SUPER_DMG2 | PRG_HRR_SuperDmg2 | specialDamage | 0.65 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_HRR_SuperDmg2_desc |
| HIRO_MELEE_DMG3 | PRG_HRR_MeleeDmg3 | meleeDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HRR_MeleeDmg3_desc |
| HIRO_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| HIRO_SUPER_STRAIGHT_PUNCH | PRG_HRR_SuperStraightPunch | superStraightPunch | 1 | 4 | 0 | 0 | 1 |  | PRG_HRR_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_HRR_SuperStraightPunch_desc |
| HIRO_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| HIRO_SUPERJUMP1 | PRG_HRR_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,13.8,0.6,15,8.5,8 | 1 | 0 | 0 | 1 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_HRR_SuperJump1_desc |
| HIRO_RUNSPEED1 | PRG_HRR_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_HRR_RunSpeed1_desc |
| HIRO_SPECIAL_UNLOCKED | PRG_HRR_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,120.0 | 2 | 1 | 0 | 1 |  | PRG_HRR_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_HRR_SpecialUnlocked_desc |
| HIRO_GROUNDPOUND2 | PRG_HRR_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 3 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_HRR_GroundPound2_desc |
| HIRO_SWARMBOTS2 | PRG_HRR_SwarmUpgrade2 | swarmBotSustained | 1 | 3 | 1 | 0 | 1 |  | PRG_HRR_SwarmUpgrade2_Screen | AV_PRG_RangedUpgrade1 | PRG_HRR_SwarmUpgrade2_desc |
| HIRO_HEALTH1 | PRG_HRR_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_HRR_Health1_desc |
| HIRO_HEALTH2 | PRG_HRR_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_HRR_Health2_desc |
