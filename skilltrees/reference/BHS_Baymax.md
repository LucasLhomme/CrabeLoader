# BHS_Baymax

`ProgressionTree = "IN2_BHS_Baymax"` -- source: `gamedb/core/in2_bhs_baymax.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| BAYMAX_BASEFLIGHT | BM_Flier,flight,flightSpeed,maxFlightSpeed,hoverSpeed | 1.0,1.0,24.00,31.00,10.00 |
| BAYMAX_BASEHEALTH | health | 150 |
| BAYMAX_BASESHIELD | shield,shieldRegenDelay,shieldRegen | 0.0,4.5,10.0 |
| BAYMAX_BASERUNSPEED | runspeed | 6.5 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| BAYMAX_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| BAYMAX_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| BAYMAX_MAXSHIELD1 | PRG_BMX_BaseShield | shield | 15 | 2 | 0 | 0 | 0 | 1 | PRG_MaxShield1_Screen | AV_PRG_ShieldsUnlock | PRG_BMX_BaseShield_desc |
| BAYMAX_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| BAYMAX_RANGED_DMG3 | PRG_BMX_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.5,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BMX_RangedDmg3_desc |
| BAYMAX_RANGED_DMG2 | PRG_BMX_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.3,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BMX_RangedDmg2_desc |
| BAYMAX_RUNSPEED1 | PRG_BMX_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_BMX_RunSpeed1_desc |
| BAYMAX_GROUNDCOMBO4 | PRG_BMX_GroundCombo4 | groundCombo4 | 1 | 4 | 1 | 0 | 0 |  | PRG_BMX_GroundCombo4_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_BMX_GroundCombo4_desc |
| BAYMAX_HEALTH2 | PRG_BMX_Health2 | health | 40 | 2 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BMX_Health2_desc |
| BAYMAX_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Baymax |  |
| BAYMAX_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| BAYMAX_RANGED_DMG1 | PRG_BMX_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.2,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_BMX_RangedDmg1_desc |
| BAYMAX_RUNSPEED2 | PRG_BMX_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_BMX_RunSpeed2_desc |
| BAYMAX_HEALTH3 | PRG_BMX_Health3 | health | 70 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BMX_Health3_desc |
| BAYMAX_GROUNDPOUND2 | PRG_BMX_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_BMX_GroundPound2_desc |
| BAYMAX_RICOCHETFIST1 | PRG_BMX_RicochetFist1 | rocketFistRicochet | 3 | 2 | 0 | 0 | 0 |  | PRG_RangedUpgrade1_Screen | AV_PRG_RangedUpgrade1 | PRG_BMX_RicochetFist1_desc |
| BAYMAX_SUPER_DMG1 | PRG_BMX_SuperDmg1 | specialDamage | 0.2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_BMX_SuperDmg1_desc |
| BAYMAX_MELEE_DMG2 | PRG_BMX_MeleeDmg2 | meleeDamage | 0.25 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BMX_MeleeDmg2_desc |
| BAYMAX_HEALCARRY | PRG_BMX_HealCarry | healCarry | 10 | 1 | 1 | 0 | 1 |  | PRG_BMX_HealCarry_Screen | AV_PRG_HealCarry | PRG_BMX_HealCarry_desc |
| BAYMAX_MELEE_DMG1 | PRG_BMX_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BMX_MeleeDmg1_desc |
| BAYMAX_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| BAYMAX_RICOCHETFIST3 | PRG_BMX_RicochetFist3 | rocketFistRicochet | 5 | 5 | 0 | 0 | 0 |  | PRG_RangedUpgrade3_Screen | AV_PRG_RangedUpgrade3 | PRG_BMX_RicochetFist3_desc |
| BAYMAX_CHARGEDSHOT | PRG_BMX_ChargedShot1 | allowRocketFistCharge,rocketFistCharge,rocketFistRicochet | 1.0,1.0,2.0 | 1 | 0 | 0 | 0 |  | PRG_BMX_ChargedShot1_Screen | AV_PRG_RangedChargeUp | PRG_BMX_ChargedShot1_desc |
| BAYMAX_MAXSHIELD2 | PRG_BMX_MaxShield1 | shield | 15 | 4 | 0 | 0 | 0 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_BMX_MaxShield1_desc |
| BAYMAX_SPECIAL_UNLOCKED | PRG_BMX_SpecialUnlocked | specialUnlocked,powerBar,powerFill,superHealRate | 1.0,1.0,120.0,2.0 | 2 | 1 | 0 | 1 |  | PRG_BMX_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_BMX_SpecialUnlocked_desc |
| BAYMAX_MELEE_DMG3 | PRG_BMX_MeleeDmg3 | meleeDamage | 0.4 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_BMX_MeleeDmg3_desc |
| BAYMAX_SUPER_STRAIGHT_PUNCH | PRG_BMX_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 0 |  | PRG_BMX_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_BMX_SuperStraightPunch_desc |
| BAYMAX_SUPER_DMG2 | PRG_BMX_SuperDmg2 | specialDamage | 0.3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_BMX_SuperDmg2_desc |
| BAYMAX_RICOCHETFIST2 | PRG_BMX_RicochetFist2 | rocketFistRicochet | 4 | 3 | 0 | 0 | 0 |  | PRG_RangedUpgrade2_Screen | AV_PRG_RangedUpgrade2 | PRG_BMX_RicochetFist2_desc |
| BAYMAX_SUPER_DMG3 | PRG_BMX_SuperDmg3 | specialDamage | 0.5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_BMX_SuperDmg3_desc |
| BAYMAX_SHIELDREGEN1 | PRG_BMX_ShieldRegen1 | shieldRegen,shieldRegenDelay | 15.0,4.5 | 5 | 0 | 0 | 0 |  | PRG_ShieldRegen1_Screen | AV_PRG_ShieldRegenUpgrade1 | PRG_BMX_ShieldRegen1_desc |
| BAYMAX_BLOCKBREAKER_COMBO | PRG_BMX_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 0 |  | PRG_BMX_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_BMX_BlockBreakerCombo_desc |
| BAYMAX_HEALTH1 | PRG_BMX_Health1 | health | 40 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_BMX_Health1_desc |
| BAYMAX_FLIGHT1 | PRG_BMX_FlightSpeed1 | flightSpeed,maxFlightSpeed,hoverSpeed | 31.00,38.00,12.00 | 4 | 0 | 0 | 0 |  | PRG_FlightSpeedUpgrade1_Screen | AV_PRG_FlightSpeedUpgrade1 | PRG_BMX_FlightSpeed1_desc |
