# AVG_Ultron

`ProgressionTree = "IN3_AVG_Ultron"` -- source: `gamedb/core/in3_avg_ultron.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| ULTRON_MAGNETIC_PULL | forcePull | 0 |
| ULTRON_CHARGEATTACK | blockBreakerCombo | 1 |
| ULTRON_BASEFIRERATE | pulseFireRate | 2.25 |
| ULTRON_BASE_HEALTH | health | 150 |
| ULTRON_BASE_HEALTHREGEN | healthRegen | 0 |
| ULTRON_FINISHER_COOLDOWN | finisherCooldown | 30 |
| ULTRON_BASE_FLIGHT | BM_Flier,flightSpeed,maxFlightSpeed,hoverSpeed | 1.0,24.00,31.00,10.00 |
| ULTRON_POWERUP_ULTRONBOTS | numMinions,minGroundRad, maxGroundRad, minAirRad, maxAirRad, maxAngleRand,collisionCheckRad,minGap,maxHeight,maxSpawnAbove,baseAngleOffset | 2,2.0,3.0,2.5,3.5,0.1,1.2,1.5,30.0,6.0,-35.0 |
| ULTRON_BASEPARRY | MeleeParry | 0 |
| ULTRON_LAUNCHER | superStraightPunch | 1 |
| ULTRON_BASEPULSECHARGETIME | pulseChargeTime | 1 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ULTRON_SPECIAL_DMG2 | PRG_ULT_SuperDmg2 | specialDamage | 0.4 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_ULT_SuperDmg2_desc |
| ULTRON_STRENGTH | PRG_ULT_Strength | BM_SuperDuperStrong | 1 | 3 | 0 | 0 | 0 |  | PRG_StrengthUpgrade_Screen | AV_PRG_PickupThrowUpgrade1 | PRG_ULT_Strength_desc |
| ULTRON_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| ULTRON_MELEE_DMG2 | PRG_ULT_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ULT_MeleeDmg2_desc |
| ULTRON_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| ULTRON_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Ultron |  |
| ULTRON_HEALTH3 | PRG_ULT_Health3 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_ULT_Health3_desc |
| ULTRON_SPECIAL_UPGRADE2 | PRG_ULT_SpecialSummon2 | numMinions | 4 | 5 | 0 | 0 | 0 |  | PRG_SuperMinionUpgrade_Screen | AV_PRG_SuperMinionUpgrade | PRG_ULT_SpecialSummon2_desc |
| ULTRON_PARRY | PRG_ULT_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_ULT_Parry_desc |
| ULTRON_PAUSECOMBO1 | PRG_ULT_PauseCombo1 | PauseComboY_YY | 1 | 2 | 0 | 0 | 0 |  | PRG_ULT_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_ULT_PauseCombo1_desc |
| ULTRON_RANGED_DMG1 | PRG_ULT_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.2,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ULT_RangedDmg1_desc |
| ULTRON_SPECIAL_DMG1 | PRG_ULT_SuperDmg1 | specialDamage | 0.4 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_ULT_SuperDmg1_desc |
| ULTRON_MELEE_DMG3 | PRG_ULT_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ULT_MeleeDmg3_desc |
| ULTRON_DIVEROLL_COUNTER | PRG_ULT_DodgeCounter | dodgeAttack | 1 | 6 | 0 | 0 | 0 |  | PRG_DodgeCounter_Screen | AV_PRG_DodgeCounter | PRG_ULT_DodgeCounter_desc |
| ULTRON_HEALTH1 | PRG_ULT_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_ULT_Health1_desc |
| ULTRON_RANGED_DMG2 | PRG_ULT_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.3,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ULT_RangedDmg2_desc |
| ULTRON_MAGNETIC_PULL | PRG_ULT_MagneticPull | forcePull | 1 | 2 | 0 | 0 | 1 |  | PRG_ForcePull_Screen | AV_PRG_ForcePull | PRG_ULT_MagneticPull_desc |
| ULTRON_PAUSECOMBO2 | PRG_ULT_PauseCombo2 | PauseComboYY_YY | 1 | 4 | 1 | 0 | 0 |  | PRG_ULT_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_ULT_PauseCombo2_desc |
| ULTRON_SPECIAL_UNLOCK | PRG_ULT_SpecialUnlocked | specialUnlocked,powerBar,powerFill,numMinions | 1.0,1.0,120.0,2.0 | 1 | 1 | 0 | 1 |  | PRG_ULT_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_ULT_SpecialUnlocked_desc |
| ULTRON_MELEE_DMG1 | PRG_ULT_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_ULT_MeleeDmg1_desc |
| ULTRON_COMBO_FINISHER1 | PRG_ULT_ComboFinisher1 | finisherCombo,finisherCooldown | 1.0,25.0 | 4 | 1 | 0 | 1 |  | PRG_ULT_ComboFinisher1_Screen | AV_PRG_ComboFinisherUnlock | PRG_ULT_ComboFinisher1_desc |
| ULTRON_PARRY_COUNTER | PRG_ULT_ParryCounter | repelAttack | 1 | 4 | 1 | 0 | 0 |  | PRG_ULT_ParryCounter_Screen | AV_PRG_ParryCounter | PRG_ULT_ParryCounter_desc |
| ULTRON_HEALTHREGEN2 | PRG_ULT_HealthRegen2 | healthRegen | 5 | 6 | 0 | 0 | 0 |  | AV_PRG_RegenUpgrade1_Screen | AV_PRG_RegenUpgrade1 | PRG_ULT_HealthRegen2_desc |
| ULTRON_SPECIAL_DMG3 | PRG_ULT_SuperDmg3 | specialDamage | 0.7 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_ULT_SuperDmg3_desc |
| ULTRON_POWERFILL1 | PRG_PowerFill1 | powerFill | 95 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| ULTRON_SPECIAL_UPGRADE1 | PRG_ULT_SpecialSummon1 | numMinions | 3 | 3 | 0 | 0 | 0 |  | PRG_SuperMinionUpgrade_Screen | AV_PRG_SuperMinionUpgrade | PRG_ULT_SpecialSummon1_desc |
| ULTRON_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| ULTRON_RANGED_DMG3 | PRG_ULT_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.5,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_ULT_RangedDmg3_desc |
| ULTRON_DUALPULSE | PRG_ULT_DualPulse | dualBeam | 1 | 3 | 0 | 0 | 0 |  | PRG_DualBlastUpgrade_Screen | AV_PRG_DualBlastUpgrade | PRG_ULT_DualPulse_desc |
| ULTRON_COMBO_FINISHER2 | PRG_ULT_ComboFinisher2 | finisherCooldown | 20 | 6 | 0 | 0 | 1 |  | PRG_ComboFinisherUpgrade1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_ULT_ComboFinisher2_desc |
| ULTRON_HEALTH2 | PRG_ULT_Health2 | health | 30 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_ULT_Health2_desc |
| ULTRON_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| ULTRON_HEALTHREGEN1 | PRG_ULT_HealthRegen1 | healthRegen | 2 | 3 | 0 | 0 | 1 |  | AV_PRG_RegenUnlock_Screen | AV_PRG_RegenUnlock | PRG_ULT_HealthRegen1_desc |
