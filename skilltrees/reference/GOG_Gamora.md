# GOG_Gamora

`ProgressionTree = "IN2_GOG_Gamora"` -- source: `gamedb/core/in2_gog_gamora.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| GAMORA_BASERANGED_DMG | rangedDamage | 1 |
| GAMORA_BASEHEALTH | health | 150 |
| GAMORA_BASERUNSPEED | runspeed | 6.5 |
| GAMORA_BASECLIP | pistolClipSize | 8 |
| GAMORA_BASESUPERJUMP | BM_SuperJumper | 0 |
| GAMORA_BASESPECIAL_DMG | specialDamage | 1 |
| GAMORA_BASEMELEE_DMG | meleeDamage | 1 |
| GAMORA_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.6 |
| GAMORA_BASEHEALTHREGEN | healthRegen | 0 |
| GAMORA_LEDGE_OBB | ledgeMinY,ledgeMaxY,ledgeJumpMinY,ledgeJumpMaxY | 1.0,2.4,1.5,2.3 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| GAMORA_SUPER_STRAIGHT_PUNCH | PRG_GMR_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 0 |  | PRG_GMR_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_GMR_SuperStraightPunch_desc |
| GAMORA_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| GAMORA_RANGED_DMG3 | PRG_GMR_RangedDmg3 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.25,0.125,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_GMR_RangedDmg3_desc |
| GAMORA_CLIP2 | PRG_GMR_PistolClip2 | pistolClipSize | -1 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_GMR_PistolClip2_desc |
| GAMORA_SUPER_DMG3 | PRG_GMR_SuperDmg3 | specialDamage | 0.5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_GMR_SuperDmg3_desc |
| GAMORA_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| GAMORA_GROUNDPOUND2 | PRG_GMR_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_GMR_GroundPound2_desc |
| GAMORA_CLIP1 | PRG_GMR_PistolClip1 | pistolClipSize | 12 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_GMR_PistolClip1_desc |
| GAMORA_RUNSPEED2 | PRG_GMR_RunSpeed2 | runspeed | 8.5 | 3 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade2_Screen | AV_PRG_RunSpeedUpgrade2 | PRG_GMR_RunSpeed2_desc |
| GAMORA_SHOCKSHOT | PRG_GMR_ShockShot | numChargedShots | 4 | 3 | 1 | 0 | 0 |  | PRG_GMR_ShockShot_Screen | AV_PRG_ShockShot | PRG_GMR_ShockShot_desc |
| GAMORA_HEALTH1 | PRG_GMR_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_GMR_Health1_desc |
| GAMORA_SPECIAL_UNLOCKED | PRG_GMR_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDuration,poweredUpRunSpeed | 1.0,1.0,120.0,8,13 | 2 | 1 | 0 | 1 |  | PRG_GMR_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_GMR_SpecialUnlocked_desc |
| GAMORA_HEALTH2 | PRG_GMR_Health2 | health | 30 | 2 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_GMR_Health2_desc |
| GAMORA_BLOCKBREAKER_COMBO | PRG_GMR_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 0 | 0 | 0 |  | PRG_GMR_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_GMR_BlockBreakerCombo_desc |
| GAMORA_SUPER_DMG2 | PRG_GMR_SuperDmg2 | specialDamage | 0.3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_GMR_SuperDmg2_desc |
| GAMORA_SUPERJUMP1 | PRG_GMR_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 1 | 0 | 0 | 0 |  | PRG_GMR_SuperJump1_Screen | AV_PRG_SuperJumpUnlock | PRG_GMR_SuperJump1_desc |
| GAMORA_CHARGEDSHOT | PRG_GMR_ChargedShot | pistolChargedShot,numChargedShots | 1,2 | 1 | 0 | 0 | 1 |  | PRG_GMR_ChargedShot_Screen | AV_PRG_RangedChargeUp | PRG_GMR_ChargedShot_desc |
| GAMORA_POWERDURATION2 | PRG_GMR_PowerDuration2 | powerDuration | 15 | 4 | 0 | 0 | 0 |  | PRG_SuperMoveDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_GMR_PowerDuration2_desc |
| GAMORA_HEALTH3 | PRG_GMR_Health3 | health | 30 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_GMR_Health3_desc |
| GAMORA_STRENGTH | PRG_GMR_Strength | BM_SuperDuperStrong | 1 | 3 | 0 | 0 | 1 |  | PRG_StrengthUpgrade_Screen | AV_PRG_PickupThrowUpgrade1 | PRG_GMR_Strength_desc |
| GAMORA_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Gamora |  |
| GAMORA_MELEE_DMG1 | PRG_GMR_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_GMR_MeleeDmg1_desc |
| GAMORA_SUPERJUMP2 | PRG_GMR_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,44,10,12 | 4 | 0 | 0 | 0 |  | PRG_GMR_SuperJump1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_GMR_SuperJump2_desc |
| GAMORA_RUNSPEED1 | PRG_GMR_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_GMR_RunSpeed1_desc |
| GAMORA_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| GAMORA_MELEE_DMG3 | PRG_GMR_MeleeDmg3 | meleeDamage | 0.35 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_GMR_MeleeDmg3_desc |
| GAMORA_HEALTH4 | PRG_GMR_Health4 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_GMR_Health4_desc |
| GAMORA_SUPER_DMG1 | PRG_GMR_SuperDmg1 | specialDamage | 0.2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_GMR_SuperDmg1_desc |
| GAMORA_POWERDURATION1 | PRG_GMR_PowerDuration1 | powerDuration | 10 | 2 | 0 | 0 | 0 |  | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_GMR_PowerDuration1_desc |
| GAMORA_MELEE_DMG2 | PRG_GMR_MeleeDmg2 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_GMR_MeleeDmg2_desc |
| GAMORA_GROUNDCOMBO5 | PRG_GMR_GroundCombo5 | groundCombo5 | 1 | 4 | 1 | 0 | 1 |  | PRG_GMR_GroundCombo5_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_GMR_GroundCombo5_desc |
| GAMORA_POWERFILL1 | PRG_PowerFill1 | powerFill | 90 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| GAMORA_RANGED_DMG1 | PRG_GMR_RangedDmg1 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.1,0.05,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_GMR_RangedDmg1_desc |
| GAMORA_RANGED_DMG2 | PRG_GMR_RangedDmg2 | rangedDamage,chargeRangedDamage,sharedRangedDamage | 0.15,0.075,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_GMR_RangedDmg2_desc |
| GAMORA_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
