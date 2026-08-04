# AV_McQueen

`ProgressionTree = "IN1_Cars_McQueen"` -- source: `gamedb/core/in1_cars_mcqueen.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| MCQUEEN_BASEJUMP | vehicleJumpHeight | 1 |
| MCQUEEN_BASEHEALTH | health | 150 |
| MCQUEEN_BASETURBOCHARGE | turboChargeMultiplier | 1 |
| MCQUEEN_BASEFIRERATE | sharedFireRateMultiplier | 1 |
| MCQUEEN_BASECLIP | sharedAmmoMultiplier | 1 |
| MCQUEEN_BASETRICKSPEED | trickRotationSpeed | 1 |
| MCQUEEN_BASESHIELD | shield,shieldRegenDelay,shieldRegen | 0.0,4.5,10.0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| MCQUEEN_TURBO2 | PRG_MCQ_Turbo2 | turboChargeMultiplier | 3 | 5 | 1 | 0 | 1 | 1 | PRG_CarvatarTurboUpgrade2_Screen | AV_PRG_CarvatarTurboUpgrade2 | PRG_MCQ_Turbo2_desc |
| MCQUEEN_MAXSHIELD2 | PRG_MCQ_MaxShield1 | shield | 15 | 5 | 0 | 0 | 0 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_MCQ_MaxShield1_desc |
| MCQUEEN_HEALTH4 | PRG_MCQ_Health4 | health | 60 | 6 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MCQ_Health4_desc |
| MCQUEEN_GRID3 | PRG_CAR_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| MCQUEEN_JUMP2 | PRG_MCQ_Jump2 | vehicleJumpHeight | 3 | 5 | 1 | 0 | 1 | 1 | PRG_CarvatarJumpUpgrade2_Screen | AV_PRG_CarvatarJumpUpgrade2 | PRG_MCQ_Jump2_desc |
| MCQUEEN_JUMP1 | PRG_MCQ_Jump1 | vehicleJumpHeight | 2 | 1 | 0 | 0 | 0 | 1 | PRG_CarvatarJumpUpgrade1_Screen | AV_PRG_CarvatarJumpUpgrade1 | PRG_MCQ_Jump1_desc |
| MCQUEEN_RANGED_DMG2 | PRG_MCQ_RangedDmg2 | loadoutDamage | 0.33 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MCQ_RangedDmg2_desc |
| MCQUEEN_RANGED_DMG3 | PRG_MCQ_RangedDmg3 | loadoutDamage | 0.33 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MCQ_RangedDmg3_desc |
| MCQUEEN_HEALTH1 | PRG_MCQ_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MCQ_Health1_desc |
| MCQUEEN_CLIP2 | PRG_MCQ_ClipSize2 | loadoutAmmoMultiplier | 2 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_MCQ_ClipSize2_desc |
| MCQUEEN_TURBO1 | PRG_MCQ_Turbo1 | turboChargeMultiplier | 2 | 1 | 0 | 0 | 0 | 1 | PRG_CarvatarTurboUpgrade1_Screen | AV_PRG_CarvatarTurboUpgrade1 | PRG_MCQ_Turbo1_desc |
| MCQUEEN_FIRERATE1 | PRG_MCQ_FireRate1 | loadoutFireRateMultiplier | 1.5 | 3 | 0 | 0 | 0 |  | PRG_CarvatarFireRateUpgrade1_Screen | AV_PRG_CarvatarFireRateUpgrade1 | PRG_MCQ_FireRate1_desc |
| MCQUEEN_CLIP1 | PRG_MCQ_ClipSize1 | loadoutAmmoMultiplier | 1.5 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_MCQ_ClipSize1_desc |
| MCQUEEN_RANGED_DMG5 | PRG_MCQ_RangedDmg5 | loadoutDamage | 0.5 | 6 | 1 | 0 | 1 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MCQ_RangedDmg5_desc |
| MCQUEEN_RANGED_DMG4 | PRG_MCQ_RangedDmg4 | loadoutDamage | 0.33 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MCQ_RangedDmg4_desc |
| MCQUEEN_MAXSHIELD1 | PRG_MCQ_BaseShield | shield | 15 | 3 | 0 | 0 | 0 | 1 | PRG_MaxShield1_Screen | AV_PRG_ShieldsUnlock | PRG_MCQ_BaseShield_desc |
| MCQUEEN_MAXSHIELD3 | PRG_MCQ_MaxShield2 | shield | 15 | 6 | 0 | 0 | 0 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_MCQ_MaxShield2_desc |
| MCQUEEN_TRICKSPEED1 | PRG_MCQ_TrickSpeed1 | trickRotationSpeed | 2 | 3 | 1 | 0 | 1 | 1 | PRG_CarvatarTrickUpgrade1_Screen | AV_PRG_CarvatarTrickUpgrade1 | PRG_MCQ_TrickSpeed1_desc |
| MCQUEEN_RANGED_DMG1 | PRG_MCQ_RangedDmg1 | loadoutDamage | 0.25 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_MCQ_RangedDmg1_desc |
| MCQUEEN_HEALTH2 | PRG_MCQ_Health2 | health | 30 | 3 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MCQ_Health2_desc |
| MCQUEEN_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Lightning |  |
| MCQUEEN_GRID2 | PRG_CAR_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| MCQUEEN_HEALTH3 | PRG_MCQ_Health3 | health | 30 | 5 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_MCQ_Health3_desc |
| MCQUEEN_GRID4 | PRG_CAR_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| MCQUEEN_SHIELDREGEN1 | PRG_MCQ_ShieldRegen1 | shieldRegenDelay,shieldRegen | 3.5,25.0 | 6 | 0 | 0 | 0 |  | PRG_ShieldRegen1_Screen | AV_PRG_ShieldRegenUpgrade1 | PRG_MCQ_ShieldRegen1_desc |
| MCQUEEN_GRID1 | PRG_CAR_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
