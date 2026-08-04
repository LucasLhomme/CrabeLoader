# EMP_Leia

`ProgressionTree = "IN3_EMP_Leia"` -- source: `gamedb/core/in3_emp_leia.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| LEIA_BASE_PARRY | MeleeParry | 0 |
| LEIA_LAUNCHER | straightSuperPunch | 1 |
| LEIA_BASE_EWOKDAMAGEMULTIPLIER | ewokDamageMultiplier | 0.3 |
| LEIA_SPECIAL_DECOY | specialUnlocked,powerBar,powerFill,powerDuration,powerRegen | 0.0,4.0,25.0,30.0,1.0 |
| LEIA_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.4 |
| LEIA_BASE_STUNDURATION | stunDuration | 3 |
| LEIA_BASE_FIRERATE | fireRate | 3 |
| LEIA_BASE_CLIP | clipSize | 8 |
| LEIA_BASECHARGESHOTTIME | pulseChargeTime | 1 |
| LEIA_CHARGEATTACK | blockBreakerCombo | 1 |
| LEIA_BASE_FINISHER | finisherCombo,finisherCooldown | 0.0,10.0 |
| LEIA_LEDGE_OBB | ledgeMinY,ledgeMaxY,ledgeJumpMinY,ledgeJumpMaxY | 1.0,2.4,1.5,2.3 |
| LEIA_BASE_SUPERJUMP | BM_SuperJumper | 0 |
| LEIA_BASE_RICOCHET | ricochetCount | 1 |
| LEIA_BASE_HEALTH | health | 150 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| LEIA_CHARGED_RICOCHET1 | PRG_LEI_ChargedRicochet1 | ricochetCount | 3 | 3 | 0 | 0 | 0 |  | PRG_RangedUpgrade1_Screen | AV_PRG_RangedUpgrade1 | PRG_LEI_ChargedRicochet1_desc |
| LEIA_FIRERATE2 | PRG_LEI_FireRate2 | fireRate | 4 | 4 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate2 | PRG_LEI_FireRate2_desc |
| LEIA_ATHLETICJUMP | PRG_LEI_AthleticJump1 | BM_SuperJumper,SJLaunchY,SJTime,SJBoost,SJBaseXZ,SJUltimateXZ | 1,11.4,0.6,15,8.5,8 | 2 | 0 | 0 | 0 |  | PRG_LEI_AthleticJump | AV_PRG_SuperJumpUnlock | PRG_LEI_AthleticJump1_desc |
| LEIA_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_Leia |  |
| LEIA_ATTACK_FINISHER2 | PRG_LEI_AttackFinisher2 | finisherCooldown | 20 | 4 | 0 | 0 | 0 |  | PRG_ComboFinisherUpgrade1_Screen | AV_PRG_ComboFinisherUpgrade1 | PRG_LEI_AttackFinisher2_desc |
| LEIA_MELEE_DMG2 | PRG_LEI_MeleeDmg2 | meleeHit,meleeDamage | 1,0.2 | 3 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LEI_MeleeDmg2_desc |
| LEIA_CLIP1 | PRG_LEI_BlasterClip1 | clipSize | 10 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_LEI_BlasterClip1_desc |
| LEIA_CHARGED_SHOT1 | PRG_LEI_ChargedShot | leiaChargedShot,chargeShotTime,ricochetCount | 1,2.0,2 | 1 | 0 | 0 | 0 |  | PRG_LEI_ChargedShot | AV_PRG_RangedChargeUp | PRG_LEI_ChargedShot_desc |
| LEIA_RANGED_DMG3 | PRG_LEI_RangedDmg3 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.125,0.75 | 5 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LEI_RangedDmg3_desc |
| LEIA_SPECIAL_DMG2 | PRG_LEI_SuperDmg2 | specialDamage | 0.4 | 3 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_LEI_SuperDmg2_desc |
| LEIA_EWOK_DAMAGE_MULTIPLIER | PRG_LEI_AttackFinisher3 | ewokDamageMultiplier | 0.5 | 5 | 0 |  |  |  | PRG_EwokDamage_Screen | AV_PRG_EwokDamage | PRG_LEI_AttackFinisher3_desc |
| LEIA_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| LEIA_PARRY | PRG_LEI_Parry | MeleeParry | 1 | 3 | 1 | 0 | 0 |  | PRG_LEI_Parry | AV_PRG_RepelUnlock | PRG_LEI_Parry_desc |
| LEIA_RANGED_DMG1 | PRG_LEI_RangedDmg1 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.05,0.4 | 1 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LEI_RangedDmg1_desc |
| LEIA_CLIP2 | PRG_LEI_BlasterClip2 | clipSize | 14 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_LEI_BlasterClip2_desc |
| LEIA_FIRERATE1 | PRG_LEI_FireRate1 | fireRate | 3.5 | 2 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_LEI_FireRate1_desc |
| LEIA_RANGEDCOMBO2 | PRG_LEI_RangedCombo2 | RangedComboYYRT | 1 | 4 | 1 | 0 | 0 |  | PRG_LEI_RangedCombo2 | AV_PRG_RangedCombo2 | PRG_LEI_RangedCombo2_desc |
| LEIA_RANGEDCOMBO1 | PRG_LEI_RangedCombo1 | RangedComboYRT | 1 | 2 | 0 | 0 | 0 |  | PRG_LEI_RangedCombo1 | AV_PRG_RangedCombo1 | PRG_LEI_RangedCombo1_desc |
| LEIA_RANGED_DMG2 | PRG_LEI_RangedDmg2 | blasterShot,rangedDamage,sharedRangedDamage | 1,0.075,0.6 | 3 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_LEI_RangedDmg2_desc |
| LEIA_MELEE_DMG1 | PRG_LEI_MeleeDmg1 | meleeHit,meleeDamage | 1,0.15 | 1 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LEI_MeleeDmg1_desc |
| LEIA_MELEE_DMG3 | PRG_LEI_MeleeDmg3 | meleeHit,meleeDamage | 1,0.35 | 5 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_LEI_MeleeDmg3_desc |
| LEIA_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| LEIA_CHARGED_TIME1 | PRG_LEI_ChargedTime1 | chargeShotTime | 1.5 | 3 | 0 | 0 |  |  | PRG_RangedCharge1_Screen | AV_PRG_RangedCharge1 | PRG_LEI_ChargedTime1_desc |
| LEIA_RANGEDCOMBO_AERIAL | PRG_LEI_RangedComboAerial | AerialRangedComboYRT | 1 | 3 | 0 | 0 | 0 |  | PRG_RangedComboAerial_Screen | AV_PRG_RangedComboAerial | PRG_LEI_RangedComboAerial_desc |
| LEIA_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| LEIA_CHARGED_TIME2 | PRG_LEI_ChargedTime2 | chargeShotTime | 0.75 | 5 | 0 | 0 |  |  | PRG_RangedCharge2_Screen | AV_PRG_RangedCharge2 | PRG_LEI_ChargedTime2_desc |
| LEIA_STUN_DURATION | PRG_LEI_SpecialStunDuration | powerStun | 1 | 3 | 0 |  |  |  | PRG_SuperMoveDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_LEI_SpecialStunDuration_desc |
| LEIA_HEALTH3 | PRG_LEI_Health2 | health | 60 | 5 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_LEI_Health2_desc |
| LEIA_SPECIAL_DMG3 | PRG_LEI_SuperDmg3 | specialDamage | 0.7 | 5 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_LEI_SuperDmg3_desc |
| LEIA_HEALTH2 | PRG_LEI_Health2 | health | 60 | 3 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_LEI_Health2_desc |
| LEIA_ATTACK_FINISHER1 | PRG_LEI_AttackFinisher1 | finisherCombo,finisherCooldown | 1.0,25.0 | 2 | 1 | 0 | 1 |  | PRG_LEI_AttackFinisher1 | AV_PRG_ComboFinisherUnlock | PRG_LEI_AttackFinisher1_desc |
| LEIA_HEALTH1 | PRG_LEI_Health1 | health | 30 | 1 | 0 | 0 | 0 | 1 | PRG_Health1_Screen | AV_PRG_HealthUpgrade_A | PRG_LEI_Health1_desc |
| LEIA_CHARGED_RICOCHET2 | PRG_LEI_ChargedRicochet2 | ricochetCount | 4 | 5 | 0 | 0 | 0 |  | PRG_RangedUpgrade2_Screen | AV_PRG_RangedUpgrade2 | PRG_LEI_ChargedRicochet2_desc |
| LEIA_POWERFILL1 | PRG_PowerFill1 | powerFill | 95 | 1 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| LEIA_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| LEIA_SPECIAL_DMG1 | PRG_LEI_SuperDmg1 | specialDamage | 0.4 | 2 | 0 | 0 | 0 | 1 | PRG_SuperDamage_A_Screen | AV_PRG_SuperDamage_A | PRG_LEI_SuperDmg1_desc |
| LEIA_SPECIAL_UNLOCKED | PRG_LEI_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDuration | 1.0,1.0,120.0,30.0 | 1 | 1 | 0 | 1 |  | PRG_LEI_SpecialUnlocked | AV_PRG_SuperMoveUnlock | PRG_LEI_SpecialUnlocked_desc |
