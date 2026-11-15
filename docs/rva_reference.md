# Engine RVA Reference (`DisneyInfinity3.exe` — Gold Edition Steam)

Verified Relative Virtual Addresses (RVAs) scanned across the entire `d:\dev\Disney_1.06` workspace (C++ plugins, tests, Lua live scripts, and Python tooling; excluding read-only `sources/`, `ava-games-in3-master/`, `build/`, and `.portage_snapshots/`) and validated against `DisneyInfinity3.exe` (`TimeDateStamp = 0x5851C5D1`, ImageBase `0x00400000`).

- **Total RVA usages**: `363` (`271` unique addresses across `27` files)
- **Audit tool**: `python d:\dev\Disney_1.06\research\tools\verify_rvas.py`

---

## `DevToolkitNative/src/engine_defs.hpp` (78 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kAgentSystemEndFrame` | `0x29EF70` | `.text` | 30 |  |
| `kPlayersGetMaxPlayers` | `0xB799F0` | `.text` | 33 |  |
| `kPlayersGetAvatarFromPlayerId` | `0xB79A10` | `.text` | 34 |  |
| `kActorSetPosition` | `0x1076690` | `.text` | 35 |  |
| `kActorTriggerAction` | `0x277AB0` | `.text` | 36 |  |
| `kActorGetName` | `0x344F80` | `.text` | 37 |  |
| `kActorGetCollisionExtends` | `0x344E80` | `.text` | 38 |  |
| `kActorGetCollisionCapsuleExtents` | `0x347F60` | `.text` | 39 |  |
| `kTardisRespawn` | `0x1068D20` | `.text` | 40 |  |
| `kAgentSystemGetData` | `0x29ECD0` | `.text` | 41 |  |
| `kKsdDataType` | `0x1A8E4AC` | `.rdata` | 42 |  |
| `kActorFromHandle` | `0x286780` | `.text` | 43 |  |
| `kActorGetTypeName` | `0x2FD9B0` | `.text` | 46 |  |
| `kActorIsAI` | `0x34FA20` | `.text` | 47 |  |
| `kActorGetSpeed` | `0x34BE60` | `.text` | 48 |  |
| `kActorGetFacingVector` | `0x34C150` | `.text` | 49 |  |
| `kActorGetScale` | `0x34CC80` | `.text` | 50 |  |
| `kActorScale` | `0x34CCC0` | `.text` | 51 |  |
| `kActorResizerSetScale` | `0x345120` | `.text` | 52 |  |
| `kActorResizerRestoreScale` | `0x3451B0` | `.text` | 53 |  |
| `kActorGetRotationX` | `0x34C4B0` | `.text` | 54 |  |
| `kActorGetRotationY` | `0x34C580` | `.text` | 55 |  |
| `kActorGetRotationZ` | `0x34C5B0` | `.text` | 56 |  |
| `kActorSetRotationX` | `0x34C660` | `.text` | 57 |  |
| `kActorSetRotationY` | `0x34C6F0` | `.text` | 58 |  |
| `kActorSetRotationZ` | `0x34C780` | `.text` | 59 |  |
| `kActorRotateToVector` | `0x34C970` | `.text` | 60 |  |
| `kGetHealth` | `0xB79200` | `.text` | 61 |  |
| `kGetMaxHealth` | `0xB792E0` | `.text` | 62 |  |
| `kGetHealthPercentage` | `0xB79250` | `.text` | 63 |  |
| `kIsAlive` | `0xB79170` | `.text` | 64 |  |
| `kSetMyHealth` | `0xB79580` | `.text` | 65 |  |
| `kAddHealth` | `0xB790C0` | `.text` | 66 |  |
| `kResetHealth` | `0xB795E0` | `.text` | 67 |  |
| `kKill` | `0xB75340` | `.text` | 68 |  |
| `kDoDamage` | `0xB79020` | `.text` | 69 |  |
| `kActorMarkForDeathForced` | `0x34F9C0` | `.text` | 70 |  |
| `kActorGetNamedLocatorPositionAndFacing` | `0x359470` | `.text` | 71 |  |
| `kActorParmExists` | `0x2FA4A0` | `.text` | 72 |  |
| `kActorParmGetString` | `0x2FA720` | `.text` | 73 |  |
| `kSkinSwapperStart` | `0xB76D80` | `.text` | 74 |  |
| `kSkinSwapperEnd` | `0xB76DB0` | `.text` | 75 |  |
| `kKillAllFX` | `0x6A81C0` | `.text` | 76 |  |
| `kPlayersGetPlayerIdFromAvatar` | `0xB86510` | `.text` | 77 |  |
| `kGamePlayersSetAvatar` | `0x28D680` | `.text` | 78 |  |
| `kActorGetActiveComponentInClass` | `0x347410` | `.text` | 79 |  |
| `kAiAvatarDriverTriggerLocalAction` | `0x49D510` | `.text` | 80 |  |
| `kStateBitmaskDataType` | `0x1A8E5D0` | `.rdata` | 81 |  |
| `kBitMaskManagerInstance` | `0x1A039C8` | `.rdata` | 82 |  |
| `kCustomBitMaskGetBitMask` | `0x30A180` | `.text` | 83 |  |
| `kCustomBitMaskTestState` | `0x2FA390` | `.text` | 84 |  |
| `kPlayersInstance` | `0x1A5F540` | `.rdata` | 87 |  |
| `kPlayersGetCameraScene` | `0xB849C0` | `.text` | 88 |  |
| `kCameraSceneCurrentCamera` | `0x10ADCF0` | `.text` | 89 |  |
| `kCameraGetFov` | `0x10B9B50` | `.text` | 90 |  |
| `kCollisionDataType` | `0x1A8E338` | `.rdata` | 93 |  |
| `kCollisionDataGetCollisionObject` | `0x10627B0` | `.text` | 94 |  |
| `kLineCollisionObjectCreate` | `0x23A570` | `.text` | 95 |  |
| `kCollisionQueryCreate` | `0x8E720` | `.text` | 96 |  |
| `kCollisionQueryAddIgnoreObject` | `0x21F480` | `.text` | 97 |  |
| `kCollisionQueryExecute` | `0x8E760` | `.text` | 98 |  |
| `kSmartPtrRelease` | `0x23DA40` | `.text` | 99 |  |
| `kGetFirstValidHandle` | `0x8CC10` | `.text` | 100 |  |
| `kGetItemPointer` | `0x2183A0` | `.text` | 101 |  |
| `kCollisionCatQuery` | `0x1A044B8` | `.rdata` | 104 |  |
| `kCollisionCatStatic` | `0x1A044A0` | `.rdata` | 105 |  |
| `kCollisionCatWater` | `0x1A04500` | `.rdata` | 106 |  |
| `kCollisionCatRigidBody` | `0x1A04500` | `.rdata` | 107 |  |
| `kCollisionCatAllActors` | `0x1A04818` | `.rdata` | 108 |  |
| `kCollisionCatTrigger` | `0x1A04648` | `.rdata` | 109 |  |
| `kCollisionCatPickup` | `0x1A045D0` | `.rdata` | 110 |  |
| `kCollisionCatPenetrable` | `0x1A04630` | `.rdata` | 111 |  |
| `kCurveHandleManagerInstance` | `0x1A03C18` | `.rdata` | 114 |  |
| `kCurveHandleManagerRetrieveValue` | `0x34A500` | `.text` | 115 |  |
| `kCurveBSplineGetCurveLength` | `0x3165A0` | `.text` | 116 |  |
| `kCurveBSplineNearestDistance` | `0x3165C0` | `.text` | 117 |  |
| `kCurveBSplinePointFromDistance` | `0x3178E0` | `.text` | 118 |  |
| `kCurveBSplineTangentFromDistance` | `0x317B00` | `.text` | 119 |  |

## `Update106Native/src/core/dev_overlay.cpp` (1 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kOpenGenieFileRva` | `0x1CB10` | `.text` | 33 |  |

## `Update106Native/src/core/diagnostics.cpp` (4 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kFlashLogRva` | `0x3FA50` | `.text` | 84 |  |
| `kLuaPcallRva` | `0xB0DF60` | `.text` | 89 |  |
| `kLuaLoadBufferRva` | `0xB0EDE0` | `.text` | 90 |  |
| `kLuaTolStringRva` | `0xB0D260` | `.text` | 91 |  |

## `Update106Native/src/core/engine.hpp` (125 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kRegisterOctSerializers` | `0x2CAF10` | `.text` | 21 |  |
| `kObjectLoaderRegisterCreator` | `0x2BC220` | `.text` | 22 |  |
| `kReadSerializerCreators` | `0x17FDD74` | `.rdata` | 23 |  |
| `kCreateTheNodeMaster2` | `0x2C8BE0` | `.text` | 24 |  |
| `kBooleanNotNodeCreate` | `0x2DF880` | `.text` | 25 |  |
| `kDefinitionCreators` | `0x1A02C80` | `.rdata` | 26 |  |
| `kDefinitionCreatorsLookup` | `0xBCC1F0` | `.text` | 27 |  |
| `kDefinitionCreatorsSetAt` | `0x2EAA00` | `.text` | 28 |  |
| `kOperatorNew` | `0x6E80` | `.text` | 29 |  |
| `kNodeDefinitionCtor` | `0x2AE840` | `.text` | 30 |  |
| `kConnectionsSetAtGrow` | `0xD2FA50` | `.text` | 31 |  |
| `kReadSerializerRegisterCreator` | `0x84420` | `.text` | 32 |  |
| `kReadSerializerReadString` | `0x81380` | `.text` | 33 |  |
| `kNodeDefinitionVtable` | `0x133FE28` | `.text` | 34 |  |
| `kImpulseHandlerCtor` | `0x2A7C60` | `.text` | 35 |  |
| `kImpulseEventCtor` | `0x2A7D20` | `.text` | 36 |  |
| `kImpulseHandlerInitialize` | `0x2C7EF0` | `.text` | 37 |  |
| `kImpulseEventInitialize` | `0x2C7FF0` | `.text` | 38 |  |
| `kImpulseEventFireImpulse` | `0x2B4720` | `.text` | 39 |  |
| `kNodeStateGetNodeMaster` | `0x2ABCA0` | `.text` | 40 |  |
| `kDataFetchEventCtor` | `0x2A8D90` | `.text` | 43 |  |
| `kDataFetchHandlerCtor` | `0x2A7CA0` | `.text` | 44 |  |
| `kDataFetchEventInitialize` | `0x2C7F80` | `.text` | 45 |  |
| `kDataFetchHandlerInitialize` | `0x2C7EF0` | `.text` | 46 |  |
| `kDataFetchEventFetchDataCurveHandle` | `0x6F2070` | `.text` | 47 |  |
| `kCurveHandleManagerInstance` | `0x1A03C18` | `.rdata` | 48 |  |
| `kCurveHandleManagerRetrieveValue` | `0x34A500` | `.text` | 49 |  |
| `kCurveBSplineGetCurveLength` | `0x3165A0` | `.text` | 50 |  |
| `kCurveBSplineNearestDistance` | `0x3165C0` | `.text` | 51 |  |
| `kCurveBSplinePointFromDistance` | `0x3178E0` | `.text` | 52 |  |
| `kCurveBSplineTangentFromDistance` | `0x317B00` | `.text` | 53 |  |
| `kCurveBSplineTangentFromDistanceClamped` | `0x3174E0` | `.text` | 54 |  |
| `kCurveBSplineDistanceFromLineOfSight` | `0x317F00` | `.text` | 55 |  |
| `kDataElementFloatVtable` | `0x133F880` | `.text` | 56 |  |
| `kDataElementFloatClassType` | `0x193E84C` | `.rdata` | 57 |  |
| `kFetchDataListNoCast` | `0x2B4390` | `.text` | 60 |  |
| `kElementArrayFree` | `0x2A9F50` | `.text` | 61 |  |
| `kCurrentGraphMaster` | `0x2AAE40` | `.text` | 62 |  |
| `kNodeDefinitionCompare` | `0x2AB890` | `.text` | 63 |  |
| `kReadSerializerReadU32` | `0x80990` | `.text` | 64 |  |
| `kReadSerializerReadBinary` | `0x81680` | `.text` | 65 |  |
| `kElementBoolVtable` | `0x133F8B8` | `.text` | 67 |  |
| `kElementIntVtable` | `0x133F848` | `.text` | 68 |  |
| `kElementAgentVtable` | `0x133F8F0` | `.text` | 69 |  |
| `kElementVectorVtable` | `0x133F928` | `.text` | 70 |  |
| `kElementBoolClassType` | `0x193E82C` | `.rdata` | 71 |  |
| `kElementIntClassType` | `0x193E86C` | `.rdata` | 72 |  |
| `kElementAgentClassType` | `0x193E76C` | `.rdata` | 73 |  |
| `kElementVectorClassType` | `0x193E42C` | `.rdata` | 74 |  |
| `kRegisterGadgets` | `0x8CE460` | `.text` | 85 |  |
| `kGetNextAgentDataType` | `0x29EC10` | `.text` | 86 |  |
| `kRegisterDataType` | `0x2A5D20` | `.text` | 87 |  |
| `kInstallEntryPoint` | `0x29EE10` | `.text` | 88 |  |
| `kAgentSystemGetData` | `0x29ECD0` | `.text` | 89 |  |
| `kGravityDataType` | `0x1A8E41C` | `.rdata` | 90 |  |
| `kAgentDataTypeCounter` | `0x1A02A40` | `.rdata` | 91 |  |
| `kAllDataTypes` | `0x1A02930` | `.rdata` | 92 |  |
| `kEntryPoints` | `0x1A02934` | `.rdata` | 93 |  |
| `kGetCameraIdFromElement` | `0xF49970` | `.text` | 96 |  |
| `kCameraPropertySetFind` | `0xECA6E0` | `.text` | 97 |  |
| `kCameraSceneGetAvatarTransform` | `0xEE80B0` | `.text` | 98 |  |
| `kCameraSceneCurrentCamera` | `0x10ADCF0` | `.text` | 99 |  |
| `kCameraSceneGetAvatar` | `0x10ADCF0` | `.text` | 100 | Note: returns Camera*, real Avatar is at offset 0x158 |
| `kPlayersGetCameraScene` | `0xB849C0` | `.text` | 101 | Players::vtable[11](playerId, 0, flag) -> CameraScene* |
| `kCameraFOV` | `0x10B9B50` | `.text` | 102 | Camera::GetFOV() (vtable slot 6; evaluates TypeProperty(16), DO NOT call inside FOV callback!) |
| `kCameraActiveProperty` | `0x10B7810` | `.text` | 103 | Camera::GetActiveCameraProperty(uint32_t) |
| `kCameraToScreen5` | `0x10B02A0` | `.text` | 104 | Camera::ToScreen(world, outScr, outW, outZ, useAspect) |
| `kRendererDefaultTarget` | `0x4C290` | `.text` | 105 | Renderer::r_defaultTarget() |
| `kPlayersInstance` | `0x1A5F540` | `.rdata` | 108 |  |
| `kNumLocalPlayers` | `0x1A8E8F0` | `.rdata` | 109 |  |
| `kPlayersGetPlayerIdFromAvatar` | `0xB84A50` | `.text` | 110 |  |
| `kAiInteractionManagerInstance` | `0x1A89E1C` | `.rdata` | 111 |  |
| `kAiInteractionManagerGetPartner` | `0xD300A0` | `.text` | 112 |  |
| `kStateBitmaskDataType` | `0x1A8E5D0` | `.rdata` | 113 |  |
| `kCustomBitMaskManagerInstance` | `0x1A039C8` | `.rdata` | 114 |  |
| `kCustomBitMaskManagerGetBitMask` | `0x30F900` | `.text` | 115 | CustomBitMaskManager::GetBitMask(cat, name) __thiscall |
| `kStateBitmaskTestState` | `0x2F9AB0` | `.text` | 116 |  |
| `kActorFromHandle` | `0x34B200` | `.text` | 117 |  |
| `kActorGetComponent` | `0x344290` | `.text` | 118 |  |
| `kAiAvatarCtrlBotName` | `0x14D6438` | `.text` | 119 |  |
| `kControlMapperDataType` | `0x1A60040` | `.rdata` | 120 |  |
| `kPlayersAllControlLockHead` | `0x1A5F530` | `.rdata` | 122 | Player::s_AllPlayerControlLock (0x1E5F518) + m_head (0x18) |
| `kPlayersMaxPlayers` | `0xB77280` | `.text` | 123 |  |
| `kExpendablesDataType` | `0x1A89AF8` | `.rdata` | 128 |  |
| `kGameSettingsInstance` | `0x1A8AA24` | `.rdata` | 129 |  |
| `kCameraSceneCastCommon` | `0xECB340` | `.text` | 130 | CommonCameraScene*(CameraScene*) |
| `kToyBoxCameraSceneVtable` | `0x15342EC` | `.text` | 131 | ToyBoxCameraScene RTTI vtable |
| `kAgentDataFromHandle` | `0x2A1C50` | `.text` | 132 | AgentData*(AgentDataHandle) |
| `kTimerPauseContainer` | `0x1940194` | `.rdata` | 134 |  |
| `kCollisionDataType` | `0x1A8E338` | `.rdata` | 135 |  |
| `kCollisionDataGetCollisionObject` | `0x10627B0` | `.text` | 136 |  |
| `kCameraInitializeCollisionObject` | `0x10AEB40` | `.text` | 137 |  |
| `kCameraInitPenetrationCollisionObject` | `0x10AEBA0` | `.text` | 138 |  |
| `kLineCollisionObjectCreate` | `0x23A570` | `.text` | 140 |  |
| `kSphereCollisionObjectCreate` | `0x392600` | `.text` | 141 |  |
| `kCollisionQueryCreate` | `0x8E720` | `.text` | 142 |  |
| `kCollisionQueryExecute` | `0x8E760` | `.text` | 143 |  |
| `kCollisionQueryAddIgnoreObject` | `0x21F480` | `.text` | 144 |  |
| `kMatrix4x4Copy` | `0x45930` | `.text` | 145 |  |
| `kCollisionQueryCategoryField` | `0x1A044B8` | `.rdata` | 147 |  |
| `kCollisionStaticCategoryField` | `0x1A044A0` | `.rdata` | 148 |  |
| `kCollisionWaterCategoryField` | `0x1A04500` | `.rdata` | 149 |  |
| `kCollisionSubsonicCategoryField` | `0x1A04540` | `.rdata` | 150 |  |
| `kCollisionPickupCategoryField` | `0x1A045D0` | `.rdata` | 151 |  |
| `kCollisionPenetrableCategoryField` | `0x1A04630` | `.rdata` | 152 |  |
| `kCollisionTriggerCategoryField` | `0x1A04648` | `.rdata` | 153 |  |
| `kHashCalcChecksum` | `0xB900` | `.text` | 388 |  |
| `kScriptSystemRegisterFunction` | `0x3318F0` | `.text` | 389 |  |
| `kScriptSystemFindFunction` | `0x331A80` | `.text` | 390 |  |
| `kScriptSystemInitialize` | `0x336DA0` | `.text` | 391 |  |
| `kScriptExecutableInit` | `0x3366F0` | `.text` | 392 |  |
| `kGScriptSystem` | `0x1A040D4` | `.rdata` | 393 |  |
| `kScriptVMPushBool` | `0x331D90` | `.text` | 395 |  |
| `kScriptVMPushInt32` | `0x331DC0` | `.text` | 396 |  |
| `kScriptVMPushFloat` | `0x331DF0` | `.text` | 397 |  |
| `kScriptVMPushString` | `0x331E20` | `.text` | 398 |  |
| `kScriptVMPushPointer` | `0x331E50` | `.text` | 399 |  |
| `kScriptVMPushVector3` | `0x331E80` | `.text` | 400 |  |
| `kScriptVMPopFloat` | `0x332040` | `.text` | 401 |  |
| `kScriptVMPopVector3` | `0x3320B0` | `.text` | 402 |  |
| `kScriptVMPopString` | `0x332140` | `.text` | 403 |  |
| `kScriptVMPopUint32` | `0x331FD0` | `.text` | 404 |  |
| `kScriptVMPopActorChecked` | `0x3321B0` | `.text` | 405 |  |
| `kScriptVMPopActorHandle` | `0x332200` | `.text` | 406 |  |
| `kScriptTableCountOffset` | `0x68CC` | `.text` | 412 |  |

## `Update106Native/src/core/script_call.hpp` (5 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kZoneIsToyBox` | `0x28AB60` | `.text` | 23 |  |
| `kPlayersGetMaxPlayers` | `0xB799F0` | `.text` | 24 |  |
| `kPlayersGetAvatarFromPlayerId` | `0xB79A10` | `.text` | 25 |  |
| `kAvatarInteractionHudAddUIText` | `0xB7E190` | `.text` | 26 |  |
| `kAvatarInteractionHudRemoveUIText` | `0xB7E240` | `.text` | 27 |  |

## `Update106Native/src/gameplay/camera_property.hpp` (2 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `at<...>()` | `0x1A018F0` | `.rdata` | 160 |  |
| `at<...>()` | `0x1A8E4AC` | `.rdata` | 174 |  |

## `Update106Native/src/gameplay/collision_helper.hpp` (3 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kSmartPtrRelease` | `0x23DA40` | `.text` | 24 |  |
| `kGetFirstValidHandle` | `0x8CC10` | `.text` | 25 |  |
| `kGetItemPointer` | `0x2183A0` | `.text` | 26 |  |

## `Update106Native/src/gameplay/submersion_manager.cpp` (54 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `at<...>()` | `0x1A5F7E0` | `.rdata` | 136 |  |
| `at<...>()` | `0x1A028D0` | `.rdata` | 207 |  |
| `at<...>()` | `0x28B020` | `.text` | 210 |  |
| `at<...>()` | `0x1A8E4AC` | `.rdata` | 254 |  |
| `at<...>()` | `0x105FDD0` | `.text` | 258 | type 1, heuristic 0 = OVERRIDE |
| `at<...>()` | `0x1A039C8` | `.rdata` | 269 |  |
| `at<...>()` | `0x30F900` | `.text` | 273 |  |
| `at<...>()` | `0x1A8E5D0` | `.rdata` | 276 |  |
| `at<...>()` | `0x1A039C8` | `.rdata` | 292 |  |
| `at<...>()` | `0x30F900` | `.text` | 295 |  |
| `at<...>()` | `0x1A8E5D0` | `.rdata` | 298 |  |
| `at<...>()` | `0x1061140` | `.text` | 302 |  |
| `at<...>()` | `0x1A8E4B8` | `.rdata` | 319 |  |
| `at<...>()` | `0x34B200` | `.text` | 330 |  |
| `at<...>()` | `0x344290` | `.text` | 334 |  |
| `at<...>()` | `0x14D6438` | `.text` | 334 |  |
| `at<...>()` | `0x1A039C8` | `.rdata` | 344 |  |
| `at<...>()` | `0x1A8E5D0` | `.rdata` | 345 |  |
| `at<...>()` | `0x30F900` | `.text` | 348 |  |
| `at<...>()` | `0x1061110` | `.text` | 349 |  |
| `at<...>()` | `0x30F900` | `.text` | 350 |  |
| `at<...>()` | `0x1061140` | `.text` | 351 |  |
| `at<...>()` | `0x30F900` | `.text` | 352 |  |
| `at<...>()` | `0x1061140` | `.text` | 353 |  |
| `at<...>()` | `0x344290` | `.text` | 359 |  |
| `at<...>()` | `0x3554F0` | `.text` | 364 |  |
| `at<...>()` | `0x3473E0` | `.text` | 365 |  |
| `at<...>()` | `0x344290` | `.text` | 366 |  |
| `at<...>()` | `0x14D6438` | `.text` | 366 |  |
| `at<...>()` | `0x492A70` | `.text` | 371 | AllowRetriggerOfBeginMove |
| `at<...>()` | `0x1A60040` | `.rdata` | 375 |  |
| `at<...>()` | `0xBB28D0` | `.text` | 385 |  |
| `at<...>()` | `0x1A5F540` | `.rdata` | 391 |  |
| `at<...>()` | `0xB84A50` | `.text` | 394 |  |
| `at<...>()` | `0xBBE450` | `.text` | 399 |  |
| `at<...>()` | `0xC10490` | `.text` | 403 |  |
| `at<...>()` | `0xC10490` | `.text` | 404 |  |
| `at<...>()` | `0xC10490` | `.text` | 405 |  |
| `at<...>()` | `0x1074F50` | `.text` | 413 |  |
| `at<...>()` | `0x107E350` | `.text` | 415 |  |
| `at<...>()` | `0x1A039C8` | `.rdata` | 431 |  |
| `at<...>()` | `0x30F900` | `.text` | 434 |  |
| `at<...>()` | `0x1A5F7E0` | `.rdata` | 437 |  |
| `at<...>()` | `0xC2CC20` | `.text` | 457 |  |
| `at<...>()` | `0x1A039C8` | `.rdata` | 473 |  |
| `at<...>()` | `0x30F900` | `.text` | 476 |  |
| `at<...>()` | `0x1A5F7E0` | `.rdata` | 485 |  |
| `at<...>()` | `0xC2CC20` | `.text` | 488 |  |
| `at<...>()` | `0x1A8E4AC` | `.rdata` | 496 |  |
| `at<...>()` | `0x1A8E4AC` | `.rdata` | 617 |  |
| `at<...>()` | `0x107A7F0` | `.text` | 690 |  |
| `at<...>()` | `0x1A899D8` | `.rdata` | 691 |  |
| `at<...>()` | `0xCE6F10` | `.text` | 694 |  |
| `at<...>()` | `0x1A8E4B8` | `.rdata` | 730 |  |

## `Update106Native/src/gameplay/swim_mode.cpp` (16 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kSwimmingVtable` | `0x156256C` | `.text` | 37 |  |
| `kVelocityBeginFrame` | `0x10A4170` | `.text` | 38 |  |
| `kModeExit` | `0x10601E0` | `.text` | 39 |  |
| `kFullKsdDataType` | `0x1A8E4AC` | `.rdata` | 40 |  |
| `kLocomotionDataType` | `0x1A8E4B8` | `.rdata` | 41 |  |
| `kMovementDataType` | `0x1A8E540` | `.rdata` | 42 |  |
| `kKsdRefresh` | `0x1087040` | `.text` | 43 |  |
| `kKsdProposeOrientation` | `0x106E5B0` | `.text` | 44 |  |
| `kKsdProposeVelocity` | `0x1074720` | `.text` | 45 |  |
| `kTimer` | `0x1A018F0` | `.rdata` | 46 |  |
| `kTimerOwner` | `0x2795E0` | `.text` | 47 |  |
| `kTimerFrameSec` | `0x279730` | `.text` | 48 |  |
| `kGravityManager` | `0x1A028D0` | `.rdata` | 49 |  |
| `kGravityDirection` | `0x28AED0` | `.text` | 50 |  |
| `kSendAnimTreeEventInternal` | `0xC44350` | `.text` | 192 |  |
| `kScriptSetClipParameterValue` | `0x300B20` | `.text` | 193 |  |

## `Update106Native/src/gameplay/swimming_locomotion.cpp` (4 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kGetActualVelocityNative` | `0x108AAB0` | `.text` | 24 |  |
| `kGetActualOrientationNative` | `0x108AB50` | `.text` | 25 |  |
| `at<...>()` | `0x1A8E4AC` | `.rdata` | 178 |  |
| `at<...>()` | `0x1A8E4AC` | `.rdata` | 194 |  |

## `Update106Native/src/gameplay/water_current.cpp` (1 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `at<...>()` | `0x34B200` | `.text` | 116 |  |

## `Update106Native/src/gameplay/water_height.cpp` (5 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kWaterCategoryField` | `0x1A04500` | `.rdata` | 30 |  |
| `kZoneManager` | `0x1A5F548` | `.rdata` | 33 |  |
| `kGetZoneName` | `0xB91040` | `.text` | 34 |  |
| `kGetZoneDataBool` | `0xB9DE80` | `.text` | 35 |  |
| `kGetZoneDataFloat` | `0xB9DFF0` | `.text` | 36 |  |

## `Update106Native/src/hooks/avatar_choice_trace.cpp` (7 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kDetermineAvatarCreationKey` | `0xBA4250` | `.text` | 32 |  |
| `kSecondPicker` | `0x457F70` | `.text` | 33 |  |
| `kGetZoneDataString` | `0xB9E0B0` | `.text` | 34 |  |
| `kHaveParamsFromCreationKey` | `0x351750` | `.text` | 35 |  |
| `kGetACDataFromCreationKey` | `0x3518D0` | `.text` | 36 |  |
| `kAvatarSuffixText` | `0x14FCA3C` | `.text` | 41 |  |
| `kFallbackAvatarText` | `0x15AA29C` | `.text` | 42 |  |

## `Update106Native/src/hooks/avatar_validity_watch.cpp` (2 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kIsAvatarValidInPlayset` | `0x5DAE30` | `.text` | 27 |  |
| `kGatewayAddTag` | `0x3A320` | `.text` | 28 |  |

## `Update106Native/src/hooks/cinematic_skip.cpp` (5 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kChoreographyManagerSingleton` | `0x1A89324` | `.rdata` | 29 |  |
| `kChoreographyManagerKillAll` | `0xC4BF50` | `.text` | 30 |  |
| `kFadeSystemInstances` | `0x1A012C0` | `.rdata` | 33 |  |
| `kFadeSystemUpdate` | `0x26D3D0` | `.text` | 35 |  |
| `at<...>()` | `0x1A8E4AC` | `.rdata` | 75 |  |

## `Update106Native/src/hooks/dna_hooks.cpp` (9 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kTwoDimensionalModeDataInitRva` | `0xCED5D0` | `.text` | 25 |  |
| `kActorPlacedHandleMessageRva` | `0xCF8690` | `.text` | 26 |  |
| `kBipedCOMDataInitRva` | `0x417B60` | `.text` | 27 |  |
| `kLuaObjectIndex` | `0x510B0` | `.text` | 31 |  |
| `kLuaObjectType` | `0xB0D2D0` | `.text` | 32 |  |
| `kLuaObjectRelease` | `0xB0E4D0` | `.text` | 33 |  |
| `kLuaObjectGetBoolean` | `0x23B7A0` | `.text` | 34 |  |
| `kBipedCOMDataName` | `0x185ECCC` | `.rdata` | 35 |  |
| `at<...>()` | `0x1A8E4AC` | `.rdata` | 123 |  |

## `Update106Native/src/hooks/dory_nodes.cpp` (2 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kZoneManager` | `0x1A5F548` | `.rdata` | 28 |  |
| `kZoneManagerIsToyBoxZone` | `0xB9E6E0` | `.text` | 29 |  |

## `Update106Native/src/hooks/fade_watch.cpp` (3 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kStringLockAcquire` | `0x186A0` | `.text` | 31 |  |
| `kStringLockRelease` | `0x186C0` | `.text` | 32 |  |
| `kFadeSystemInstances` | `0x1A012C0` | `.rdata` | 33 |  |

## `Update106Native/src/hooks/playset_filter.cpp` (1 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kIsCharacterAllowedInPlayset` | `0x37F910` | `.text` | 27 |  |

## `Update106Native/src/hooks/purecall_watch.cpp` (1 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kPureCallRva` | `0xAB9B85` | `.text` | 28 |  |

## `Update106Native/src/render/early_vfs.cpp` (2 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kIatCreateFileA_Rva` | `0x12E321C` | `.text` | 33 |  |
| `kIatCreateFileW_Rva` | `0x12E31A8` | `.text` | 34 |  |

## `Update106Native/src/render/render_trace.cpp` (15 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kGeometryNodeCreate` | `0x74700` | `.text` | 34 |  |
| `kCellCreate` | `0x8A4B0` | `.text` | 35 |  |
| `kCellAssignmentCreate` | `0x88C90` | `.text` | 36 |  |
| `kMasterAssetCreateFromOct` | `0x385CB0` | `.text` | 37 |  |
| `kModelPieceSetSceneNode` | `0x375FE0` | `.text` | 38 |  |
| `kModelPieceInstanceGeometry` | `0x3760C0` | `.text` | 39 |  |
| `kModelPieceActivate` | `0x379960` | `.text` | 40 |  |
| `kModelPieceDeactivate` | `0x3761C0` | `.text` | 41 |  |
| `kUpdateCellVisibility` | `0x897A0` | `.text` | 42 |  |
| `kEnableVisRegionCulling` | `0x8CAD30` | `.text` | 43 |  |
| `kForceAllCellsVisible` | `0x19AECB9` | `.rdata` | 44 |  |
| `kPushGeometryCreator` | `0x7F814` | `.text` | 45 |  |
| `kPushCellCreator` | `0x7F850` | `.text` | 46 |  |
| `kPushCellAssignmentCreator` | `0x7F862` | `.text` | 47 |  |
| `kNoNameText` | `0x1329270` | `.text` | 48 |  |

## `Update106Native/src/render/scaleform_texture_fix.cpp` (4 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kTextureDestructor` | `0x2276B0` | `.text` | 39 | D3D1x::Texture::~Texture body, thiscall |
| `kTextureInitFromResource` | `0x225AD0` | `.text` | 40 | bool Texture::Initialize(ID3D11Texture2D*) |
| `kMutexLock` | `0x1ADA30` | `.text` | 41 |  |
| `kMutexUnlock` | `0x1AE6B0` | `.text` | 42 |  |

## `Update106Native/tests/test_parity_1to1.cpp` (2 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `at<...>()` | `0x10261F0` | `.text` | 141 |  |
| `at<...>()` | `0xB900` | `.text` | 191 |  |

## `research/tools/patch_swim_mode.py` (6 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `kGravityDirection` | `0x28AED0` | `.text` | 10 |  |
| `kGravityDirection` | `0x28AED0` | `.text` | 11 |  |
| `at<...>()` | `0x34B200` | `.text` | 16 |  |
| `at<...>()` | `0x34B200` | `.text` | 17 |  |
| `at<...>()` | `0x344290` | `.text` | 20 |  |
| `at<...>()` | `0x344290` | `.text` | 21 |  |

## `research/tools/update_106/live/p_lock.lua` (2 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `Crabe.Memory/Hook` | `0x1E5F518` | `.data` | 12 |  |
| `Crabe.Memory/Hook` | `0x1E5F540` | `.data` | 14 |  |

## `scratch/patch_swim_mode_locomotion.py` (4 RVAs)

| Symbol / Callsite | RVA | Section | Line | Notes |
| :--- | :--- | :--- | :---: | :--- |
| `at<...>()` | `0x34B200` | `.text` | 58 |  |
| `at<...>()` | `0x344290` | `.text` | 59 |  |
| `at<...>()` | `0x34B200` | `.text` | 186 |  |
| `at<...>()` | `0x344290` | `.text` | 187 |  |
