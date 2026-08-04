# AVG_HulkBuster

`ProgressionTree = "IN3_AVG_HulkBuster"` -- source: `gamedb/core/in3_avg_hulkbuster.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| HULKBUSTER_FINISHER_COOLDOWN | finisherCooldown | 20 |
| HULKBUSTER_BASEFIRERATE | pulseFireRate | 2.5 |
| HULKBUSTER_BASE_HEALTH | health | 150 |
| HULKBUSTER_BASEPARRY | MeleeParry | 0 |
| HULKBUSTER_BASESHIELD | shield,shieldRegenDelay,shieldRegen | 0.0,10.0,5.0 |
| HULKBUSTER_LAUNCHER | superStraightPunch | 1 |
| HULKBUSTER_CHARGEATTACK | blockBreakerCombo | 1 |
| HULKBUSTER_BASE_FLIGHT | BM_Flier,flightSpeed,maxFlightSpeed,hoverSpeed | 1.0,24.00,31.00,10.00 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| HULKBUSTER_SHIELD_UNLOCK | PRG_HBS_BaseShield | shield | 15 | 1 | 0 | 0 | 1 | 1 | PRG_ShieldsUnlock_Screen | AV_PRG_ShieldsUnlock | PRG_HBS_BaseShield_desc |
| HULKBUSTER_MELEE_DMG3 | PRG_HBS_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HBS_MeleeDmg3_desc |
| HULKBUSTER_FIRERATE | PRG_HBS_FireRate1 | pulseFireRate | 3 | 3 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_HBS_FireRate1_desc |
| HULKBUSTER_TRACTOR_BEAM1 | PRG_HBS_AttackFinisher1 | finisherCombo,finisherCooldown | 1.0,25.0 | 2 | 0 | 0 | 1 |  | PRG_ComboFinisherUnlock_Screen | AV_PRG_ComboFinisherUnlock | PRG_HBS_AttackFinisher1_desc |
| HULKBUSTER_SPECIAL_DMG3 | PRG_HBS_SuperDmg3 | specialDamage | 0.12 | 5 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_HBS_SuperDmg3_desc |
| HULKBUSTER_RANGED_DMG1 | PRG_HBS_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.15,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HBS_RangedDmg1_desc |
| HULKBUSTER_MAXSHIELD2 | PRG_HBS_MaxShield2 | shield | 15 | 3 | 0 | 0 | 0 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_HBS_MaxShield2_desc |
| HULKBUSTER_POWERFILL1 | PRG_PowerFill1 | powerFill | 95 | 3 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| HULKBUSTER_SPECIAL_UNLOCK | PRG_HBS_SpecialUnlocked | specialUnlocked,powerBar,powerFill,specialFireRate | 1.0,3.0,120.0,1.0 | 1 | 1 | 0 | 1 |  | PRG_HBS_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_HBS_SpecialUnlocked_desc |
| HULKBUSTER_TRACTOR_BEAM_UNLOCK | PRG_HBS_TractorBeam | forcePull | 1 | 1 | 1 | 0 | 1 |  | PRG_HBS_TractorBeam_Screen | AV_PRG_ForcePull | PRG_HBS_TractorBeam_desc |
| HULKBUSTER_TRACTOR_BEAM2 | PRG_HBS_AttackFinisher2 | finisherCooldown | 20 | 4 | 0 | 0 | 0 |  | PRG_ComboFinisherUpgrade1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_HBS_AttackFinisher2_desc |
| HULKBUSTER_SPECIAL_DMG2 | PRG_HBS_SuperDmg2 | specialDamage | 0.08 | 3 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_HBS_SuperDmg2_desc |
| HULKBUSTER_DUALPULSE | PRG_HBS_DualPulse | pulseDual,multiShotDamage | 1,0.65 | 2 | 0 | 0 | 0 |  | PRG_DualBlastUpgrade_Screen | AV_PRG_DualBlastUpgrade | PRG_HBS_DualPulse_desc |
| HULKBUSTER_HEALTH1 | PRG_HBS_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_HBS_Health1_desc |
| HULKBUSTER_SPECIAL_UPG_SHOULDERS | PRG_HBS_SpecialUpgShoulders | specialUpgrade1 | 1 | 2 | 0 | 0 | 1 |  | PRG_SuperWeaponUpgrade_Screen | AV_PRG_SuperWeaponUpgrade | PRG_HBS_SpecialUpgShoulders_desc |
| HULKBUSTER_AERIALPAUSECOMBO | PRG_HBS_AerialPauseCombo | AerialPauseComboY_YY | 1 | 4 | 0 | 0 | 0 |  | PRG_HBS_PauseCombo3_Screen | AV_PRG_PauseComboAerial | PRG_HBS_AerialPauseCombo_desc |
| HULKBUSTER_POWERBAR1 | PRG_PowerBar1 | powerBar | 6 | 2 | 0 | 0 | 0 |  | PRG_SuperMeterSizeUpgrade4_Screen | AV_PRG_SuperMeterSizeUpgrade4 | PRG_PowerBar1_desc |
| HULKBUSTER_POWERBAR2 | PRG_PowerBar2 | powerBar | 9 | 5 | 0 | 0 | 0 |  | PRG_SuperMeterSizeUpgrade4_Screen | AV_PRG_SuperMeterSizeUpgrade4 | PRG_PowerBar2_desc |
| HULKBUSTER_HEALTH3 | PRG_HBS_Health3 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_HBS_Health3_desc |
| HULKBUSTER_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| HULKBUSTER_CHARGEDBOLT | PRG_HBS_ChargedBolt | allowPulseCharge,pulseChargeTime | 1,0.9 | 4 | 0 | 0 | 1 |  | PRG_RangedChargeUp_Screen | AV_PRG_RangedChargeUp | PRG_HBS_ChargedBolt_desc |
| HULKBUSTER_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| HULKBUSTER_RANGED_DMG3 | PRG_HBS_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.4,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HBS_RangedDmg3_desc |
| HULKBUSTER_SPECIAL_DMG1 | PRG_HBS_SuperDmg1 | specialDamage | 0.05 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_HBS_SuperDmg1_desc |
| HULKBUSTER_SHIELDREGEN1 | PRG_HBS_ShieldRegen1 | shieldRegenDelay,shieldRegen | 6,10.0 | 5 | 0 | 0 | 0 |  | PRG_ShieldRegen1_Screen | AV_PRG_ShieldRegenUpgrade1 | PRG_HBS_ShieldRegen1_desc |
| HULKBUSTER_RANGED_DMG2 | PRG_HBS_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.25,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_HBS_RangedDmg2_desc |
| HULKBUSTER_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_HulkBuster |  |
| HULKBUSTER_DASH_ATTACK | PRG_HBS_DashAttack | DashAttack | 1 | 1 | 1 | 0 | 0 |  | PRG_HBS_DashAttack_Screen | AV_PRG_DashAttack | PRG_HBS_DashAttack_desc |
| HULKBUSTER_PAUSECOMBO2 | PRG_HBS_PauseCombo2 | PauseComboYY_YY | 1 | 5 | 0 | 0 | 0 |  | PRG_HBS_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_HBS_PauseCombo2_desc |
| HULKBUSTER_PAUSECOMBO1 | PRG_HBS_PauseCombo1 | PauseComboY_YY | 1 | 2 | 1 | 0 | 0 |  | PRG_HBS_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_HBS_PauseCombo1_desc |
| HULKBUSTER_MELEE_DMG2 | PRG_HBS_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HBS_MeleeDmg2_desc |
| HULKBUSTER_MELEE_DMG1 | PRG_HBS_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_HBS_MeleeDmg1_desc |
| HULKBUSTER_PARRY | PRG_HBS_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_HBS_Parry_desc |
| HULKBUSTER_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| HULKBUSTER_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| HULKBUSTER_SPECIAL_UPG_ARMS | PRG_HBS_SpecialUpgArms | specialUpgrade2 | 1 | 4 | 0 | 0 | 1 |  | PRG_SuperWeaponUpgrade_Screen | AV_PRG_SuperWeaponUpgrade | PRG_HBS_SpecialUpgArms_desc |
| HULKBUSTER_HEALTH4 | PRG_HBS_Health4 | health | 60 | 6 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_HBS_Health4_desc |
| HULKBUSTER_HEALTH2 | PRG_HBS_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_HBS_Health2_desc |
| HULKBUSTER_FLIGHT | PRG_HBS_FlightSpeed1 | BM_Flier,flightSpeed,maxFlightSpeed,hoverSpeed | 1.0,24.00,31.00,10.00 | 2 | 0 | 0 | 0 |  | PRG_FlightSpeedUpgrade1_Screen | AV_PRG_FlightSpeedUpgrade1 | PRG_HBS_FlightSpeed1_desc |
| HULKBUSTER_PARRY_COUNTER | PRG_HBS_ParryCounter | repelAttack | 1 | 4 | 0 | 0 | 0 |  | PRG_ParryCounter_Screen | AV_PRG_ParryCounter | PRG_HBS_ParryCounter_desc |
