# SPD_NickFury

`ProgressionTree = "IN2_SPD_NickFury"` -- source: `gamedb/core/in2_spd_nickfury.lua`

Use in a `.patch` file as `ProgressionTable.BaseValues.<Key>.Value = ...` or
`ProgressionTable.Upgrades.<Key>.<Field> = ...` -- see `skilltrees/README.md` and
`skilltrees/HULK_BASEHEALTH.patch` for the mechanism.

## BaseValues

Passive stats, always active (no unlock cost).

| Key | AttributeName | Value |
|---|---|---|
| NICKFURY_BASESHIELD | shield,shieldRegen,shieldRegenDelay | 0.00,10.0,4.5 |
| NICKFURY_BASECLIP | autopistolClipSize | 8 |
| NICKFURY_BASEHEALTH | health | 150 |
| NICKFURY_BASERUNSPEED | runspeed | 6.5 |
| NICKFURY_TARGETMODEBONUSMULTIPLIER | targetModeBonusMultiplier | 1.4 |
| NICKFURY_BASEFIRERATE | autopistolFireRate,sharedFireRateMultiplier | 3.0,1.0 |

## Upgrades

Skill-tree nodes: unlockable in game with skill points, or granted for free by
setting `PCCoreAbility = 1` in a patch (see `skilltrees/HULK_BASEHEALTH.patch`'s
commented-out example).

| Key | UpgradeName | AttributeName | Value | Cost | PCCoreAbility | PrereqLevel | HighValue | additive | SkillScreen | UpgradeIcon | Description |
|---|---|---|---|---|---|---|---|---|---|---|---|
| NICKFURY_GRID4 | PRG_EMP_RangedGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Ranged |  |
| NICKFURY_HEALTH1 | PRG_NFR_Health1 | health | 50 | 2 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_NFR_Health1_desc |
| NICKFURY_MELEE_DMG1 | PRG_NFR_MeleeDmg1 | meleeDamage | 0.15 | 2 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_NFR_MeleeDmg1_desc |
| NICKFURY_DECOY_ATTACK | PRG_NFR_DecoyAttack | decoyAttack | 1 | 6 | 0 | 0 | 1 |  | PRG_NFR_DecoyAttack_Screen | AV_PRG_SuperDecoyAttack | PRG_NFR_DecoyAttack_desc |
| NICKFURY_MELEE_DMG3 | PRG_NFR_MeleeDmg3 | meleeDamage | 0.35 | 6 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_NFR_MeleeDmg3_desc |
| NICKFURY_GROUNDCOMBO5 | PRG_NFR_GroundCombo5 | groundCombo5 | 1 | 4 | 0 | 0 | 0 |  | PRG_NFR_GroundCombo5_Screen | AV_PRG_MeleeComboUpgrade1 | PRG_NFR_GroundCombo5_desc |
| NICKFURY_GRID1 | PRG_EMP_HealthGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Health |  |
| NICKFURY_POWERDURATION2 | PRG_NFR_PowerDuration2 | powerDuration | 20 | 5 | 0 | 0 | 0 |  | PRG_PowerDuration2_Screen | AV_PRG_SuperMoveDuration2 | PRG_NFR_PowerDuration2_desc |
| NICKFURY_SUPER_STRAIGHT_PUNCH | PRG_NFR_SuperStraightPunch | superStraightPunch | 1 | 4 | 1 | 0 | 1 |  | PRG_NFR_SuperStraightPunch_Screen | AV_PRG_SuperPunchUnlock | PRG_NFR_SuperStraightPunch_desc |
| NICKFURY_BLOCKBREAKER_COMBO | PRG_NFR_BlockBreakerCombo | blockBreakerCombo | 1 | 2 | 1 | 0 | 0 |  | PRG_NFR_BlockBreakerCombo_Screen | AV_PRG_BlockBreaker | PRG_NFR_BlockBreakerCombo_desc |
| NICKFURY_POWERDURATION1 | PRG_NFR_PowerDuration1 | powerDuration | 15 | 3 | 0 | 0 | 0 |  | PRG_PowerDuration1_Screen | AV_PRG_SuperMoveDuration1 | PRG_NFR_PowerDuration1_desc |
| NICKFURY_GROUNDPOUND2 | PRG_NFR_GroundPound2 | groundpound,groundpoundDamage | 1.0,2.0 | 2 | 0 | 0 | 0 |  | PRG_GroundPoundUpgrade1_Screen | AV_PRG_GroundPoundUpgrade1 | PRG_NFR_GroundPound2_desc |
| NICKFURY_MAXSHIELD1 | PRG_NFR_MaxShield1 | shield | 15 | 1 | 0 | 0 | 0 | 1 | PRG_MaxShield1_Screen | AV_PRG_ShieldsUnlock | PRG_NFR_MaxShield1_desc |
| NICKFURY_CLIP1 | PRG_NFR_PistolClip1 | autopistolClipSize | 20 | 2 | 0 | 0 | 0 |  | PRG_ClipUpgrade1_Screen | AV_PRG_ClipUpgrade1 | PRG_NFR_PistolClip1_desc |
| NICKFURY_POWERFILL1 | PRG_PowerFill1 | powerFill | 75 | 4 | 0 | 0 | 0 |  | PRG_SuperMeterFillRate1_Screen | AV_PRG_SuperMeterFillRate1 | PRG_PowerFill1_desc |
| NICKFURY_RANGED_DMG2 | PRG_NFR_RangedDmg2 | rangedDamage,sharedRangedDamage | 0.075,0.6 | 4 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_NFR_RangedDmg2_desc |
| NICKFURY_MAXSHIELD2 | PRG_NFR_MaxShield2 | shield | 15 | 3 | 0 | 0 | 0 | 1 | PRG_ShieldUpgrade_A_Screen | AV_PRG_ShieldUpgrade_A | PRG_NFR_MaxShield2_desc |
| NICKFURY_SHIELDREGEN1 | PRG_NFR_ShieldRegen1 | shieldRegen,shieldRegenDelay | 25,3.5 | 5 | 0 | 0 | 0 |  | PRG_ShieldRegen1_Screen | AV_PRG_ShieldRegenUpgrade1 | PRG_NFR_ShieldRegen1_desc |
| NICKFURY_GRID3 | PRG_EMP_MeleeGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Melee |  |
| NICKFURY_RANGED_DMG1 | PRG_NFR_RangedDmg1 | rangedDamage,sharedRangedDamage | 0.05,0.4 | 2 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_NFR_RangedDmg1_desc |
| NICKFURY_FIRERATE2 | PRG_NFR_FireRate2 | autopistolFireRate,sharedFireRateMultiplier | 4.0,2.0 | 5 | 0 | 0 | 0 |  | PRG_RangedFireRate2_Screen | AV_PRG_RangedFireRate2 | PRG_NFR_FireRate2_desc |
| NICKFURY_EXPLOSIVESHELLS | PRG_NFR_ExplosiveShells | explosiveShells | 1 | 5 | 1 | 0 | 0 |  | PRG_NFR_ExplosiveShells_Screen | AV_PRG_RangedUpgrade3 | PRG_NFR_ExplosiveShells_desc |
| NICKFURY_FIRERATE1 | PRG_NFR_FireRate1 | autopistolFireRate,sharedFireRateMultiplier | 3.5,1.5 | 3 | 0 | 0 | 0 |  | PRG_RangedFireRate1_Screen | AV_PRG_RangedFireRate1 | PRG_NFR_FireRate1_desc |
| NICKFURY_HEALTH2 | PRG_NFR_Health2 | health | 100 | 4 | 0 | 0 | 0 | 1 | PRG_HealthUpgrade_A_Screen | AV_PRG_HealthUpgrade_A | PRG_NFR_Health2_desc |
| NICKFURY_GRID2 | PRG_EMP_SpecialGrid |  |  |  | 0 |  |  |  |  | HUD_GridIcons_Force |  |
| NICKFURY_START |  |  |  |  | 0 |  |  |  |  | HUD_PlayerIcons_NickFury |  |
| NICKFURY_CLIP2 | PRG_NFR_PistolClip2 | autopistolClipSize | 28 | 4 | 0 | 0 | 0 |  | PRG_ClipUpgrade2_Screen | AV_PRG_ClipUpgrade2 | PRG_NFR_PistolClip2_desc |
| NICKFURY_SPECIAL_UNLOCKED | PRG_NFR_SpecialUnlocked | specialUnlocked,powerBar,powerFill,powerDuration | 1.0,1.0,90.0,10.0 | 2 | 1 | 0 | 1 |  | PRG_NFR_SpecialUnlocked_Screen | AV_PRG_SuperMoveUnlock | PRG_NFR_SpecialUnlocked_desc |
| NICKFURY_MELEE_DMG2 | PRG_NFR_MeleeDmg2 | meleeDamage | 0.2 | 4 | 0 | 0 | 0 | 1 | PRG_MeleeDamage_A_Screen | AV_PRG_MeleeDamage_A | PRG_NFR_MeleeDmg2_desc |
| NICKFURY_DUALWIELD | PRG_NFR_DualWield | autopistolDual,multiShotDamage,autopistolClipSize | 1.00,0.625,16 | 1 | 0 | 0 | 1 |  | PRG_DualWieldUnlock_Screen | AV_PRG_DualWieldUnlock | PRG_NFR_DualWield_desc |
| NICKFURY_RANGED_DMG3 | PRG_NFR_RangedDmg3 | rangedDamage,sharedRangedDamage | 0.125,0.75 | 6 | 0 | 0 | 0 | 1 | PRG_RangedDamage_A_Screen | AV_PRG_RangedDamage_A | PRG_NFR_RangedDmg3_desc |
| NICKFURY_RUNSPEED1 | PRG_NFR_RunSpeed1 | runspeed | 8 | 1 | 0 | 0 | 0 |  | PRG_RunSpeedUpgrade1_Screen | AV_PRG_RunSpeedUpgrade1 | PRG_NFR_RunSpeed1_desc |
