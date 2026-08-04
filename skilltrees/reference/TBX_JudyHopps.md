# TBX_JudyHopps

`ProgressionTree = "IN3_TBX_JudyHopps"` -- source: `gamedb/core/in3_tbx_judyhopps.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| JUDYHOPPS_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| JUDYHOPPS_TRANQDART_BASESTUN | webStormStun | 5 |
| JUDYHOPPS_BASEREPEL | repel | 1 |
| JUDYHOPPS_BASE_HEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| JUDYHOPPS_SPECIAL_UNLOCKED | PRG_JDY_SpecialUnlocked | specialUnlocked,powerBar,powerFill | 1.0,1.0,95.0 | 1 | 1 | 0 | 1 |  | PRG_JDY_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_JDY_SpecialUnlocked_desc |
| JUDYHOPPS_SUPER_JUMP3 | PRG_JDY_SuperJump3 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,64,11.3,10 | 6 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade2_Screen | AV_PRG_SuperJumpUpgrade2 | PRG_JDY_SuperJump3_desc |
| JUDYHOPPS_RANGED_DMG2 | PRG_JDY_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.25,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JDY_RangedDmg2_desc |
| JUDYHOPPS_MELEE_DMG1 | PRG_JDY_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JDY_MeleeDmg1_desc |
| JUDYHOPPS_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| JUDYHOPPS_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| JUDYHOPPS_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| JUDYHOPPS_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_JudyHopps |  |
| JUDYHOPPS_SPECIAL_STUNDURATION2 | PRG_JDY_PowerDuration2 | webStormStun | 10 | 6 | 0 | 0 | 0 |  | PRG_SuperMoveDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_JDY_PowerDuration2_desc |
| JUDYHOPPS_SPECIAL_DMG1 | PRG_JDY_SuperDmg1 | specialDamage | 0.2 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_JDY_SuperDmg1_desc |
| JUDYHOPPS_MELEE_DMG3 | PRG_JDY_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 1 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JDY_MeleeDmg3_desc |
| JUDYHOPPS_RANGED_DMG1 | PRG_JDY_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.15,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JDY_RangedDmg1_desc |
| JUDYHOPPS_SUPER_JUMP1 | PRG_JDY_SuperJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,14,8.5,8 | 2 | 1 | 0 | 0 |  | PRG_SuperJumpUnlock_Screen | AV_PRG_SuperJumpUnlock | PRG_JDY_SuperJump1_desc |
| JUDYHOPPS_HEALTH1 | PRG_JDY_Health1 | health | 50 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_JDY_Health1_desc |
| JUDYHOPPS_RANGED_DMG3 | PRG_JDY_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.4,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JDY_RangedDmg3_desc |
| JUDYHOPPS_DASH_ATTACK | PRG_JDY_DashAttack | DashAttack | 1 | 1 | 1 | 0 | 0 |  | PRG_JDY_DashAttack_Screen | AV_PRG_DashAttack | PRG_JDY_DashAttack_desc |
| JUDYHOPPS_PARRY | PRG_JDY_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_JDY_Parry_desc |
| JUDYHOPPS_SPECIAL_DMG2 | PRG_JDY_SuperDmg2 | specialDamage | 0.3 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_JDY_SuperDmg2_desc |
| JUDYHOPPS_SUPER_JUMP2 | PRG_JDY_SuperJump2 | SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 11.4,0.6,44,10,12 | 5 | 0 | 0 | 0 |  | PRG_SuperJumpUpgrade1_Screen | AV_PRG_SuperJumpUpgrade1 | PRG_JDY_SuperJump2_desc |
| JUDYHOPPS_SPECIAL_DMG3 | PRG_JDY_SuperDmg3 | specialDamage | 0.5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_JDY_SuperDmg3_desc |
| JUDYHOPPS_SPECIAL_STUNDURATION1 | PRG_JDY_PowerDuration1 | webStormStun | 7 | 3 | 0 | 0 | 0 |  | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_JDY_PowerDuration1_desc |
| JUDYHOPPS_POWERFILL1 | PRG_PowerFill1 | powerFill | 60 | 2 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| JUDYHOPPS_HEIGHTENED_SENSE | PRG_JDY_HeightenedSense | heightenedSense,BM_HeightenedSenses | 1,1 | 4 | 0 | 0 | 1 |  | PRG_HeightenedSense_Screen | AV_PRG_SpideySenseUnlock | PRG_JDY_HeightenedSense_desc |
| JUDYHOPPS_HEALTH3 | PRG_JDY_Health3 | health | 100 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_JDY_Health3_desc |
| JUDYHOPPS_MELEE_DMG2 | PRG_JDY_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JDY_MeleeDmg2_desc |
| JUDYHOPPS_HEALTH2 | PRG_JDY_Health2 | health | 50 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_JDY_Health2_desc |
| JUDYHOPPS_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
