# SPD_GreenGoblin

`ProgressionTree = "IN2_SPD_GreenGoblin"` -- source: `gamedb/core/in2_spd_greengoblin.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| GOBLIN_BASE_RUNSPEED | runSpeed | 8 |
| GOBLIN_BASE_HEALTHREGEN | healthRegen | 0 |
| GOBLIN_BASECHARGETIME | bolaChargeTime | 2 |
| GOBLIN_BASESUPERJUMP | BM_SuperJumper | 0 |
| GOBLIN_BASE_HEALTH | health | 150 |
| GOBLIN_BASE_GROUNDPOUND | groundpound | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| GOBLIN_SPECIAL_DMG3 | PRG_GOB_SpecialDmg3 | specialDamage | 1 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_GOB_SpecialDmg3_desc |
| GOBLIN_POWERFILL1 | PRG_PowerFill1 | powerFill | 75 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| GOBLIN_SPECIAL_UNLOCK | PRG_GOB_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,90.0 | 2 | 1 | 0 | 1 |  | PRG_GOB_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_GOB_SpecialUnlocked_desc |
| GOBLIN_HEALTH3 | PRG_GOB_Health3 | health | 70 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_GOB_Health3_desc |
| GOBLIN_SPECIAL_DMG2 | PRG_GOB_SpecialDmg2 | specialDamage | 0.65 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_GOB_SpecialDmg2_desc |
| GOBLIN_RANGED_DMG3 | PRG_GOB_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.25,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_GOB_RangedDmg3_desc |
| GOBLIN_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| GOBLIN_RUNSPEED2 | PRG_GOB_RunSpeed2 | runspeed | 10.25 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_GOB_RunSpeed2_desc |
| GOBLIN_MELEE_DMG3 | PRG_GOB_MeleeDmg3 | meleeDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_GOB_MeleeDmg3_desc |
| GOBLIN_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| GOBLIN_HEALTH1 | PRG_GOB_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_GOB_Health1_desc |
| GOBLIN_SPECIAL_DMG1 | PRG_GOB_SpecialDmg1 | specialDamage | 0.35 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_GOB_SpecialDmg1_desc |
| GOBLIN_HEALTHREGEN2 | PRG_GOB_HealthRegen2 | healthRegen | 5 | 6 | 0 | 0 | 0 |  | PRG_HealthRegen2_Screen | AV_PRG_RegenUpgrade1 | PRG_GOB_HealthRegen2_desc |
| GOBLIN_BOLA_CHARGETIME2 | PRG_GOB_BolaChargeTime2 | bolaChargeTime | 0.75 | 6 | 0 | 0 | 0 |  | PRG_ChargeSpeed2_Screen | AV_PRG_RangedCharge2 | PRG_GOB_BolaChargeTime2_desc |
| GOBLIN_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_GreenGoblin |  |
| GOBLIN_GROUNDPOUND2 | PRG_GOB_GroundPound2 | groundpound,groundpoundDamage | 1.00,2.0 | 3 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_GOB_GroundPound2_desc |
| GOBLIN_BOLA_UNLOCK | PRG_GOB_BolaUnlocked | bolaUnlocked,bolaChargeTime | 1,2 | 3 | 1 | 0 | 1 |  | PRG_GOB_BolaUnlocked_Screen | AV_PRG_RangedChargeUp | PRG_GOB_BolaUnlocked_desc |
| GOBLIN_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| GOBLIN_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| GOBLIN_RANGED_DMG2 | PRG_GOB_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.15,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_GOB_RangedDmg2_desc |
| GOBLIN_MELEE_DMG1 | PRG_GOB_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_GOB_MeleeDmg1_desc |
| GOBLIN_HEALTHREGEN1 | PRG_GOB_HealthRegen1 | healthRegen | 2 | 3 | 0 | 0 | 1 |  | PRG_HealthRegen1_Screen | AV_PRG_RegenUnlock | PRG_GOB_HealthRegen1_desc |
| GOBLIN_MELEE_DMG2 | PRG_GOB_MeleeDmg2 | meleeDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_GOB_MeleeDmg2_desc |
| GOBLIN_RUNSPEED1 | PRG_GOB_RunSpeed1 | runspeed | 9.5 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_GOB_RunSpeed1_desc |
| GOBLIN_BOLA_CHARGETIME1 | PRG_GOB_BolaChargeTime1 | bolaChargeTime | 1.5 | 5 | 0 | 0 | 0 |  | PRG_ChargeSpeed1_Screen | AV_PRG_RangedCharge1 | PRG_GOB_BolaChargeTime1_desc |
| GOBLIN_SUPERJUMP1 | PRG_GOB_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,42.5,10,12 | 2 | 0 | 0 | 0 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_GOB_SuperJump1_desc |
| GOBLIN_SUPERJUMP2 | PRG_GOB_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,64,11.3,10 | 5 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_GOB_SuperJump2_desc |
| GOBLIN_HEALTH2 | PRG_GOB_Health2 | health | 40 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_GOB_Health2_desc |
| GOBLIN_RANGED_DMG1 | PRG_GOB_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.1,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_GOB_RangedDmg1_desc |
| GOBLIN_BLOCKBREAKER_COMBO | PRG_GOB_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 0 |  | PRG_GOB_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_GOB_BlockBreakerCombo_desc |
| GOBLIN_FIRERATE1 | PRG_GOB_FireRate1 | pumpkinFireRate | 1.5 | 4 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_GOB_FireRate1_desc |
| GOBLIN_GROUNDCOMBO5 | PRG_GOB_GroundCombo5 | groundCombo5 | 1 | 4 | 1 | 0 | 0 |  | PRG_GOB_GroundCombo5_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_GOB_GroundCombo5_desc |
| GOBLIN_SUPER_STRAIGHT_PUNCH | PRG_GOB_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 1 |  | PRG_GOB_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_GOB_SuperStraightPunch_desc |
