# PSX_James

`ProgressionTree = "IN3_PSX_James"` -- source: `gamedb/core/in3_psx_james.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| JAMES_BASERUNSPEED | runspeed | 6.5 |
| JAMES_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.6 |
| JAMES_BASEFIRERATE | fireRate,sharedFireRateMultiplier | 3.0,1.0 |
| JAMES_BASEHEALTH | health | 150 |
| JAMES_GROUND_SLAM | groundSlam | 1 |
| JAMES_BASECLIP | clipSize | 8 |
| JAMES_BASESPECIAL_DMG | specialDamage | 1 |
| JAMES_BASERANGED_DMG | rangedDamage | 1 |
| JAMES_BASEMELEE_DMG | meleeDamage | 1 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| JAMES_MELEE_DMG1 | PRG_JMS_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JMS_MeleeDmg1_desc |
| JAMES_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Special |  |
| JAMES_CLIP1 | PRG_JMS_PistolClip1 | clipSize | 10 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_JMS_PistolClip1_desc |
| JAMES_SPECIAL_DURATION1 | PRG_JMS_PowerDuration1 | powerDuration | 5 | 4 | 0 | 0 | 0 | 1 | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_JMS_PowerDuration1_desc |
| JAMES_PARRY | PRG_JMS_Parry | repel | 1 | 2 | 0 | 0 | 0 |  | PRG_Repel_Screen | AV_PRG_RepelUnlock | PRG_JMS_Parry_desc |
| JAMES_HEALTH2 | PRG_JMS_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_JMS_Health2_desc |
| JAMES_POWERFILL1 | PRG_PowerFill1 | powerFill | 95 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| JAMES_PAUSECOMBO2 | PRG_JMS_PauseCombo2 | PauseComboYY_YY | 1 | 6 | 0 | 0 | 0 |  | PSX_PRG_JMS_PauseCombo2_Screen | AV_PRG_PauseCombo2 | PRG_JMS_PauseCombo2_desc |
| JAMES_POWERBAR3 | PRG_PowerBar3 | powerBar | 1 | 6 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar3_desc |
| JAMES_INSPIRED_LEADERSHIP | PRG_JMS_Leadership | inspiredLeadership | 1 | 6 | 0 | 0 | 0 |  | PRG_InspiredLeadership_Screen | AV_PRG_InspiredLeadership | PRG_JMS_Leadership_desc |
| JAMES_INSTANT_REVIVE | PRG_JMS_InstantRevive | reviveSpeed | 0 | 5 | 0 | 0 | 0 |  | PRG_ReviveSpeedInstant_Screen | AV_PRG_ReviveSpeedInstant | PRG_JMS_InstantRevive_desc |
| JAMES_PAUSECOMBO1 | PRG_JMS_PauseCombo1 | PauseComboY_YY | 1 | 2 | 1 | 0 | 0 |  | PSX_PRG_JMS_PauseCombo1_Screen | AV_PRG_PauseCombo1 | PRG_JMS_PauseCombo1_desc |
| JAMES_AERIALPAUSECOMBO | PRG_JMS_AerialPauseCombo | AerialPauseComboY_YY | 1 | 4 | 1 | 0 | 0 |  | PSX_PRG_JMS_AerialPauseCombo_Screen | AV_PRG_PauseComboAerial | PRG_JMS_AerialPauseCombo_desc |
| JAMES_RANGED_DMG3 | PRG_JMS_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.125,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JMS_RangedDmg3_desc |
| JAMES_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| JAMES_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| JAMES_MELEE_DMG3 | PRG_JMS_MeleeDmg3 | meleeHit,meleeDamage | 1,0.35 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JMS_MeleeDmg3_desc |
| JAMES_COMBO_FINISHER | PRG_JMS_ComboFinisher | finisherCombo,finisherCooldown | 1.0,20.0 | 4 | 0 | 0 | 1 |  | PSX_PRG_JMS_ComboFinisher_Screen | AV_PRG_ComboFinisherUnlock | PRG_JMS_ComboFinisher_desc |
| JAMES_FIRERATE1 | PRG_JMS_FireRate1 | fireRate,sharedFireRateMultiplier | 3.5,1.5 | 2 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_JMS_FireRate1_desc |
| JAMES_HEALTH1 | PRG_JMS_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_JMS_Health1_desc |
| JAMES_HEALTH3 | PRG_JMS_Health2 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_JMS_Health2_desc |
| JAMES_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_NickFury |  |
| JAMES_SPECIAL_DURATION2 | PRG_JMS_PowerDuration2 | powerDuration | 5 | 6 | 0 | 0 | 0 | 1 | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_JMS_PowerDuration2_desc |
| JAMES_ATHLETIC_JUMP | PRG_JMS_AthleticJump | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 2 | 1 | 0 | 0 |  | PSX_PRG_JMS_AthleticJump_Screen | AV_PRG_SuperJumpUnlock | PRG_JMS_AthleticJump_desc |
| JAMES_POWERBAR2 | PRG_PowerBar2 | powerBar | 1 | 5 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar2_desc |
| JAMES_POWERBAR1 | PRG_PowerBar1 | powerBar | 1 | 2 | 0 | 0 | 0 | 1 | PRG_SuperMeterSizeUpgrade2_Screen | AV_PRG_SuperMeterSizeUpgrade2 | PRG_PowerBar1_desc |
| JAMES_RANGED_DMG2 | PRG_JMS_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.075,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JMS_RangedDmg2_desc |
| JAMES_FIRERATE2 | PRG_JMS_FireRate2 | fireRate,sharedFireRateMultiplier | 4.0,2.0 | 5 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_JMS_FireRate2_desc |
| JAMES_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| JAMES_MELEE_DMG2 | PRG_JMS_MeleeDmg2 | meleeHit,meleeDamage | 1,0.2 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_JMS_MeleeDmg2_desc |
| JAMES_CLIP2 | PRG_JMS_PistolClip2 | clipSize | 14 | 5 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_JMS_PistolClip2_desc |
| JAMES_SPECIAL_UNLOCKED | PRG_JMS_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDuration | 1.0,1.0,120.0,20.0 | 1 | 1 | 0 | 1 |  | PSX_PRG_JMS_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_JMS_SpecialUnlocked_desc |
| JAMES_RANGED_DMG1 | PRG_JMS_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.05,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_JMS_RangedDmg1_desc |
| JAMES_MORALE_BOOST | PRG_JMS_MoraleBoost | moraleBoost | 1 | 4 | 0 | 0 | 0 |  | PRG_MoraleBoost_Screen | AV_PRG_MoraleBoost | PRG_JMS_MoraleBoost_desc |
