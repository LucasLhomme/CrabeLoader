# TBX_NickWilde

`ProgressionTree = "IN3_TBX_NickWilde"` -- source: `gamedb/core/in3_tbx_nickwilde.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| NICKWILDE_BASE_HEALTH | health | 150 |
| NICKWILDE_BASE_SUPERJUMP | BM_SuperJumper | 0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| NICKWILDE_RANGED_AOE2 | PRG_WLD_RangedAOE2 | chargedMunitionUpgrade | 2 | 6 | 0 | 0 | 0 |  | PRG_RangedAOEBlast2_Screen | AV_PRG_RangedAOEBlast2 | PRG_WLD_RangedAOE2_desc |
| NICKWILDE_RANGED_DMG2 | PRG_WLD_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.15,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_WLD_RangedDmg2_desc |
| NICKWILDE_CHARGED_UNLOCK | PRG_WLD_ChargeUnlocked | allowThrowCharge,rangedChargeTime | 1,2.0 | 2 | 1 | 0 | 1 |  | PRG_WLD_ChargeUnlocked_Screen | AV_PRG_RangedChargeUp | PRG_WLD_ChargeUnlocked_desc |
| NICKWILDE_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| NICKWILDE_SPECIAL_DMG1 | PRG_WLD_SuperDmg1 | specialDamage,blckHdSpecialDamage | 0.4,1 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_WLD_SuperDmg1_desc |
| NICKWILDE_RANGED_DMG1 | PRG_WLD_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.1,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_WLD_RangedDmg1_desc |
| NICKWILDE_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| NICKWILDE_SPECIAL_DMG2 | PRG_WLD_SuperDmg2 | specialDamage,blckHdSpecialDamage | 0.4,1 | 4 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_WLD_SuperDmg2_desc |
| NICKWILDE_RANGED_DMG3 | PRG_WLD_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.25,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_WLD_RangedDmg3_desc |
| NICKWILDE_SPECIAL_DMG3 | PRG_WLD_SuperDmg3 | specialDamage,blckHdSpecialDamage | 0.7,1 | 6 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_WLD_SuperDmg3_desc |
| NICKWILDE_HEALTH3 | PRG_WLD_Health3 | health | 100 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_WLD_Health3_desc |
| NICKWILDE_PARRY | PRG_WLD_Parry | MeleeParry | 1 | 2 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_WLD_Parry_desc |
| NICKWILDE_HEALTH1 | PRG_WLD_Health1 | health | 50 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_WLD_Health1_desc |
| NICKWILDE_RANGED_AOE1 | PRG_WLD_RangedAOE1 | chargedMunitionUpgrade | 1 | 3 | 0 | 0 | 0 |  | PRG_RangedAOEBlast_Screen | AV_PRG_RangedAOEBlast | PRG_WLD_RangedAOE1_desc |
| NICKWILDE_CHARGESPEED2 | PRG_WLD_ChargeTime2 | rangedChargeTime | 0.75 | 5 | 0 | 0 | 0 |  | PRG_RangedCharge2_Screen | AV_PRG_RangedCharge2 | PRG_WLD_ChargeTime2_desc |
| NICKWILDE_SUPER_JUMP | PRG_WLD_AthleticJump | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1.0,11.4,0.6,44,10,12 | 2 | 1 | 0 | 0 |  | PRG_WLD_AthleticJump_Screen | AV_PRG_SuperJumpUnlock | PRG_WLD_AthleticJump_desc |
| NICKWILDE_MELEE_DMG1 | PRG_WLD_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_WLD_MeleeDmg1_desc |
| NICKWILDE_CHARGESPEED1 | PRG_WLD_ChargeTime1 | rangedChargeTime | 1.5 | 3 | 0 | 0 | 0 |  | PRG_RangedCharge1_Screen | AV_PRG_RangedCharge1 | PRG_WLD_ChargeTime1_desc |
| NICKWILDE_POWERFILL1 | PRG_PowerFill1 | powerFill | 60 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| NICKWILDE_MELEE_DMG3 | PRG_WLD_MeleeDmg3 | meleeHit,meleeDamage | 1,0.4 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_WLD_MeleeDmg3_desc |
| NICKWILDE_SPECIAL_DURATION1 | PRG_WLD_PowerDuration1 | powerDuration | 30 | 4 | 0 | 0 | 0 |  | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_WLD_PowerDuration1_desc |
| NICKWILDE_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_NickWilde |  |
| NICKWILDE_SPECIAL_UNLOCKED | PRG_WLD_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDuration,blckHdSpecialDamage | 1.0,1.0,95.0,25.0,0 | 1 | 1 | 0 | 1 |  | PRG_WLD_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_WLD_SpecialUnlocked_desc |
| NICKWILDE_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| NICKWILDE_PAUSECOMBO | PRG_WLD_PauseCombo1 | PauseComboY_YY | 1 | 3 | 1 | 0 | 0 |  | PRG_WLD_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_WLD_PauseCombo1_desc |
| NICKWILDE_PARRY_COUNTER | PRG_WLD_ParryCounter | repelAttack | 1 | 4 | 0 | 0 | 0 |  | PRG_WLD_ParryCounter_Screen | AV_PRG_ParryCounter | PRG_WLD_ParryCounter_desc |
| NICKWILDE_SPECIAL_DURATION2 | PRG_WLD_PowerDuration2 | powerDuration | 38 | 6 | 0 | 0 | 0 |  | PRG_SuperMoveDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_WLD_PowerDuration2_desc |
| NICKWILDE_HEALTH2 | PRG_WLD_Health2 | health | 50 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_WLD_Health2_desc |
| NICKWILDE_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| NICKWILDE_MELEE_DMG2 | PRG_WLD_MeleeDmg2 | meleeHit,meleeDamage | 1,0.25 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_WLD_MeleeDmg2_desc |
