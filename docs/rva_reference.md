# Engine RVA Reference (`DisneyInfinity3.exe`)

> **Intended Audience:** Standard Lua mods should use the high-level `Crabe.*` and `Game.*` APIs documented in [NativeDB](nativedb.md). This low-level reference is intended for **native `.asi` plugin authors** (`CrabePluginApi` / `HookRegistry`), **`Crabe.Hooks` / `Crabe.Memory`** power users, and **CrabeLoader contributors**.

Community reference of verified Relative Virtual Addresses (RVAs) for **Disney Infinity 3.0: Gold Edition (Steam)** (`TimeDateStamp = 0x5851C5D1`, ImageBase `0x00400000`), organized by engine subsystem.

- **Unique verified RVAs**: `271`
- **Target binary**: `DisneyInfinity3.exe` (Win32 x86)

---

## Script VM, Lua & EvalTree (27)

| Symbol | RVA | Section | Notes |
| :--- | :--- | :--- | :--- |
| `kScriptTableCountOffset` | `0x68CC` | `.text` |  |
| `kLuaObjectIndex` | `0x510B0` | `.text` |  |
| `kLuaObjectGetBoolean` | `0x23B7A0` | `.text` |  |
| `kScriptSetClipParameterValue` | `0x300B20` | `.text` |  |
| `kScriptSystemRegisterFunction` | `0x3318F0` | `.text` |  |
| `kScriptSystemFindFunction` | `0x331A80` | `.text` |  |
| `kScriptVMPushBool` | `0x331D90` | `.text` |  |
| `kScriptVMPushInt32` | `0x331DC0` | `.text` |  |
| `kScriptVMPushFloat` | `0x331DF0` | `.text` |  |
| `kScriptVMPushString` | `0x331E20` | `.text` |  |
| `kScriptVMPushPointer` | `0x331E50` | `.text` |  |
| `kScriptVMPushVector3` | `0x331E80` | `.text` |  |
| `kScriptVMPopUint32` | `0x331FD0` | `.text` |  |
| `kScriptVMPopFloat` | `0x332040` | `.text` |  |
| `kScriptVMPopVector3` | `0x3320B0` | `.text` |  |
| `kScriptVMPopString` | `0x332140` | `.text` |  |
| `kScriptVMPopActorChecked` | `0x3321B0` | `.text` |  |
| `kScriptVMPopActorHandle` | `0x332200` | `.text` |  |
| `kScriptExecutableInit` | `0x3366F0` | `.text` |  |
| `kScriptSystemInitialize` | `0x336DA0` | `.text` |  |
| `kCurveHandleManagerRetrieveValue` | `0x34A500` | `.text` |  |
| `kLuaTolStringRva` | `0xB0D260` | `.text` |  |
| `kLuaObjectType` | `0xB0D2D0` | `.text` |  |
| `kLuaPcallRva` | `0xB0DF60` | `.text` |  |
| `kLuaObjectRelease` | `0xB0E4D0` | `.text` |  |
| `kLuaLoadBufferRva` | `0xB0EDE0` | `.text` |  |
| `kGScriptSystem` | `0x1A040D4` | `.rdata` |  |

## Actors, Players & Locomotion (67)

| Symbol | RVA | Section | Notes |
| :--- | :--- | :--- | :--- |
| `kActorFromHandle` | `0x286780` | `.text` |  |
| `kGamePlayersSetAvatar` | `0x28D680` | `.text` |  |
| `kAgentSystemGetData` | `0x29ECD0` | `.text` |  |
| `kAgentSystemEndFrame` | `0x29EF70` | `.text` |  |
| `kNodeStateGetNodeMaster` | `0x2ABCA0` | `.text` |  |
| `kStateBitmaskTestState` | `0x2F9AB0` | `.text` |  |
| `kCustomBitMaskTestState` | `0x2FA390` | `.text` |  |
| `kActorParmExists` | `0x2FA4A0` | `.text` |  |
| `kActorParmGetString` | `0x2FA720` | `.text` |  |
| `kActorGetTypeName` | `0x2FD9B0` | `.text` |  |
| `kCustomBitMaskGetBitMask` | `0x30A180` | `.text` |  |
| `kCustomBitMaskManagerGetBitMask` | `0x30F900` | `.text` | CustomBitMaskManager::GetBitMask(cat, name) __thiscall |
| `kActorGetComponent` | `0x344290` | `.text` |  |
| `kActorGetName` | `0x344F80` | `.text` |  |
| `kActorResizerSetScale` | `0x345120` | `.text` |  |
| `kActorResizerRestoreScale` | `0x3451B0` | `.text` |  |
| `kActorGetActiveComponentInClass` | `0x347410` | `.text` |  |
| `kActorFromHandle` | `0x34B200` | `.text` |  |
| `kActorGetSpeed` | `0x34BE60` | `.text` |  |
| `kActorGetFacingVector` | `0x34C150` | `.text` |  |
| `kActorGetRotationX` | `0x34C4B0` | `.text` |  |
| `kActorGetRotationY` | `0x34C580` | `.text` |  |
| `kActorGetRotationZ` | `0x34C5B0` | `.text` |  |
| `kActorSetRotationX` | `0x34C660` | `.text` |  |
| `kActorSetRotationY` | `0x34C6F0` | `.text` |  |
| `kActorSetRotationZ` | `0x34C780` | `.text` |  |
| `kActorRotateToVector` | `0x34C970` | `.text` |  |
| `kActorGetScale` | `0x34CC80` | `.text` |  |
| `kActorScale` | `0x34CCC0` | `.text` |  |
| `kActorMarkForDeathForced` | `0x34F9C0` | `.text` |  |
| `kActorIsAI` | `0x34FA20` | `.text` |  |
| `kActorGetNamedLocatorPositionAndFacing` | `0x359470` | `.text` |  |
| `kIsAvatarValidInPlayset` | `0x5DAE30` | `.text` |  |
| `kKillAllFX` | `0x6A81C0` | `.text` |  |
| `kKill` | `0xB75340` | `.text` |  |
| `kPlayersMaxPlayers` | `0xB77280` | `.text` |  |
| `kDoDamage` | `0xB79020` | `.text` |  |
| `kAddHealth` | `0xB790C0` | `.text` |  |
| `kGetHealth` | `0xB79200` | `.text` |  |
| `kGetHealthPercentage` | `0xB79250` | `.text` |  |
| `kGetMaxHealth` | `0xB792E0` | `.text` |  |
| `kSetMyHealth` | `0xB79580` | `.text` |  |
| `kResetHealth` | `0xB795E0` | `.text` |  |
| `kPlayersGetMaxPlayers` | `0xB799F0` | `.text` |  |
| `kPlayersGetAvatarFromPlayerId` | `0xB79A10` | `.text` |  |
| `kAvatarInteractionHudAddUIText` | `0xB7E190` | `.text` |  |
| `kAvatarInteractionHudRemoveUIText` | `0xB7E240` | `.text` |  |
| `kPlayersGetPlayerIdFromAvatar` | `0xB84A50` | `.text` |  |
| `kPlayersGetPlayerIdFromAvatar` | `0xB86510` | `.text` |  |
| `kDetermineAvatarCreationKey` | `0xBA4250` | `.text` |  |
| `kChoreographyManagerKillAll` | `0xC4BF50` | `.text` |  |
| `kActorPlacedHandleMessageRva` | `0xCF8690` | `.text` |  |
| `kTardisRespawn` | `0x1068D20` | `.text` |  |
| `kKsdProposeOrientation` | `0x106E5B0` | `.text` |  |
| `kKsdProposeVelocity` | `0x1074720` | `.text` |  |
| `kActorSetPosition` | `0x1076690` | `.text` |  |
| `kKsdRefresh` | `0x1087040` | `.text` |  |
| `kAiAvatarCtrlBotName` | `0x14D6438` | `.text` |  |
| `kAvatarSuffixText` | `0x14FCA3C` | `.text` |  |
| `kSwimmingVtable` | `0x156256C` | `.text` |  |
| `kFallbackAvatarText` | `0x15AA29C` | `.text` |  |
| `kBitMaskManagerInstance` | `0x1A039C8` | `.rdata` |  |
| `kPlayersAllControlLockHead` | `0x1A5F530` | `.rdata` | Player::s_AllPlayerControlLock (0x1E5F518) + m_head (0x18) |
| `kPlayersInstance` | `0x1A5F540` | `.rdata` |  |
| `kKsdDataType` | `0x1A8E4AC` | `.rdata` |  |
| `kStateBitmaskDataType` | `0x1A8E5D0` | `.rdata` |  |
| `kNumLocalPlayers` | `0x1A8E8F0` | `.rdata` |  |

## Camera & Splines (20)

| Symbol | RVA | Section | Notes |
| :--- | :--- | :--- | :--- |
| `kCurveBSplineGetCurveLength` | `0x3165A0` | `.text` |  |
| `kCurveBSplineNearestDistance` | `0x3165C0` | `.text` |  |
| `kCurveBSplineTangentFromDistanceClamped` | `0x3174E0` | `.text` |  |
| `kCurveBSplinePointFromDistance` | `0x3178E0` | `.text` |  |
| `kCurveBSplineTangentFromDistance` | `0x317B00` | `.text` |  |
| `kCurveBSplineDistanceFromLineOfSight` | `0x317F00` | `.text` |  |
| `kDataFetchEventFetchDataCurveHandle` | `0x6F2070` | `.text` |  |
| `kPlayersGetCameraScene` | `0xB849C0` | `.text` |  |
| `kCameraPropertySetFind` | `0xECA6E0` | `.text` |  |
| `kCameraSceneCastCommon` | `0xECB340` | `.text` | CommonCameraScene*(CameraScene*) |
| `kCameraSceneGetAvatarTransform` | `0xEE80B0` | `.text` |  |
| `kGetCameraIdFromElement` | `0xF49970` | `.text` |  |
| `kCameraSceneCurrentCamera` | `0x10ADCF0` | `.text` |  |
| `kCameraInitializeCollisionObject` | `0x10AEB40` | `.text` |  |
| `kCameraInitPenetrationCollisionObject` | `0x10AEBA0` | `.text` |  |
| `kCameraToScreen5` | `0x10B02A0` | `.text` | Camera::ToScreen(world, outScr, outW, outZ, useAspect) |
| `kCameraActiveProperty` | `0x10B7810` | `.text` | Camera::GetActiveCameraProperty(uint32_t) |
| `kCameraGetFov` | `0x10B9B50` | `.text` |  |
| `kToyBoxCameraSceneVtable` | `0x15342EC` | `.text` | ToyBoxCameraScene RTTI vtable |
| `kCurveHandleManagerInstance` | `0x1A03C18` | `.rdata` |  |

## Collision & Physics (24)

| Symbol | RVA | Section | Notes |
| :--- | :--- | :--- | :--- |
| `kGetFirstValidHandle` | `0x8CC10` | `.text` |  |
| `kCollisionQueryCreate` | `0x8E720` | `.text` |  |
| `kCollisionQueryExecute` | `0x8E760` | `.text` |  |
| `kGetItemPointer` | `0x2183A0` | `.text` |  |
| `kCollisionQueryAddIgnoreObject` | `0x21F480` | `.text` |  |
| `kLineCollisionObjectCreate` | `0x23A570` | `.text` |  |
| `kSmartPtrRelease` | `0x23DA40` | `.text` |  |
| `kActorTriggerAction` | `0x277AB0` | `.text` |  |
| `kElementArrayFree` | `0x2A9F50` | `.text` |  |
| `kActorGetCollisionExtends` | `0x344E80` | `.text` |  |
| `kActorGetCollisionCapsuleExtents` | `0x347F60` | `.text` |  |
| `kSphereCollisionObjectCreate` | `0x392600` | `.text` |  |
| `at<...>()` | `0x492A70` | `.text` | AllowRetriggerOfBeginMove |
| `kAiAvatarDriverTriggerLocalAction` | `0x49D510` | `.text` |  |
| `kCollisionDataGetCollisionObject` | `0x10627B0` | `.text` |  |
| `kCollisionCatStatic` | `0x1A044A0` | `.rdata` |  |
| `kCollisionCatQuery` | `0x1A044B8` | `.rdata` |  |
| `kCollisionCatWater` | `0x1A04500` | `.rdata` |  |
| `kCollisionSubsonicCategoryField` | `0x1A04540` | `.rdata` |  |
| `kCollisionCatPickup` | `0x1A045D0` | `.rdata` |  |
| `kCollisionCatPenetrable` | `0x1A04630` | `.rdata` |  |
| `kCollisionCatTrigger` | `0x1A04648` | `.rdata` |  |
| `kCollisionCatAllActors` | `0x1A04818` | `.rdata` |  |
| `kCollisionDataType` | `0x1A8E338` | `.rdata` |  |

## VFS, Serialization & Object Graph (21)

| Symbol | RVA | Section | Notes |
| :--- | :--- | :--- | :--- |
| `kHashCalcChecksum` | `0xB900` | `.text` |  |
| `kOpenGenieFileRva` | `0x1CB10` | `.text` |  |
| `kGeometryNodeCreate` | `0x74700` | `.text` |  |
| `kReadSerializerReadU32` | `0x80990` | `.text` |  |
| `kReadSerializerReadString` | `0x81380` | `.text` |  |
| `kReadSerializerReadBinary` | `0x81680` | `.text` |  |
| `kReadSerializerRegisterCreator` | `0x84420` | `.text` |  |
| `kNodeDefinitionCompare` | `0x2AB890` | `.text` |  |
| `kNodeDefinitionCtor` | `0x2AE840` | `.text` |  |
| `kObjectLoaderRegisterCreator` | `0x2BC220` | `.text` |  |
| `kCreateTheNodeMaster2` | `0x2C8BE0` | `.text` |  |
| `kRegisterOctSerializers` | `0x2CAF10` | `.text` |  |
| `kBooleanNotNodeCreate` | `0x2DF880` | `.text` |  |
| `kDefinitionCreatorsSetAt` | `0x2EAA00` | `.text` |  |
| `kModelPieceSetSceneNode` | `0x375FE0` | `.text` |  |
| `kDefinitionCreatorsLookup` | `0xBCC1F0` | `.text` |  |
| `kIatCreateFileW_Rva` | `0x12E31A8` | `.text` |  |
| `kIatCreateFileA_Rva` | `0x12E321C` | `.text` |  |
| `kNodeDefinitionVtable` | `0x133FE28` | `.text` |  |
| `kReadSerializerCreators` | `0x17FDD74` | `.rdata` |  |
| `kDefinitionCreators` | `0x1A02C80` | `.rdata` |  |

## Rendering, Shaders, UI & Cinematics (7)

| Symbol | RVA | Section | Notes |
| :--- | :--- | :--- | :--- |
| `kFlashLogRva` | `0x3FA50` | `.text` |  |
| `kRendererDefaultTarget` | `0x4C290` | `.text` | Renderer::r_defaultTarget() |
| `kTextureInitFromResource` | `0x225AD0` | `.text` | bool Texture::Initialize(ID3D11Texture2D*) |
| `kTextureDestructor` | `0x2276B0` | `.text` | D3D1x::Texture::~Texture body, thiscall |
| `kFadeSystemUpdate` | `0x26D3D0` | `.text` |  |
| `kFadeSystemInstances` | `0x1A012C0` | `.rdata` |  |
| `kChoreographyManagerSingleton` | `0x1A89324` | `.rdata` |  |

## Audio & VFX (2)

| Symbol | RVA | Section | Notes |
| :--- | :--- | :--- | :--- |
| `kSkinSwapperStart` | `0xB76D80` | `.text` |  |
| `kSkinSwapperEnd` | `0xB76DB0` | `.text` |  |

## Core Engine & Memory (103)

| Symbol | RVA | Section | Notes |
| :--- | :--- | :--- | :--- |
| `kOperatorNew` | `0x6E80` | `.text` |  |
| `kStringLockAcquire` | `0x186A0` | `.text` |  |
| `kStringLockRelease` | `0x186C0` | `.text` |  |
| `kGatewayAddTag` | `0x3A320` | `.text` |  |
| `kMatrix4x4Copy` | `0x45930` | `.text` |  |
| `kPushGeometryCreator` | `0x7F814` | `.text` |  |
| `kPushCellCreator` | `0x7F850` | `.text` |  |
| `kPushCellAssignmentCreator` | `0x7F862` | `.text` |  |
| `kCellAssignmentCreate` | `0x88C90` | `.text` |  |
| `kUpdateCellVisibility` | `0x897A0` | `.text` |  |
| `kCellCreate` | `0x8A4B0` | `.text` |  |
| `kMutexLock` | `0x1ADA30` | `.text` |  |
| `kMutexUnlock` | `0x1AE6B0` | `.text` |  |
| `kTimerOwner` | `0x2795E0` | `.text` |  |
| `kTimerFrameSec` | `0x279730` | `.text` |  |
| `kZoneIsToyBox` | `0x28AB60` | `.text` |  |
| `kGravityDirection` | `0x28AED0` | `.text` |  |
| `at<...>()` | `0x28B020` | `.text` |  |
| `kGetNextAgentDataType` | `0x29EC10` | `.text` |  |
| `kInstallEntryPoint` | `0x29EE10` | `.text` |  |
| `kAgentDataFromHandle` | `0x2A1C50` | `.text` | AgentData*(AgentDataHandle) |
| `kRegisterDataType` | `0x2A5D20` | `.text` |  |
| `kImpulseHandlerCtor` | `0x2A7C60` | `.text` |  |
| `kDataFetchHandlerCtor` | `0x2A7CA0` | `.text` |  |
| `kImpulseEventCtor` | `0x2A7D20` | `.text` |  |
| `kDataFetchEventCtor` | `0x2A8D90` | `.text` |  |
| `kCurrentGraphMaster` | `0x2AAE40` | `.text` |  |
| `kFetchDataListNoCast` | `0x2B4390` | `.text` |  |
| `kImpulseEventFireImpulse` | `0x2B4720` | `.text` |  |
| `kImpulseHandlerInitialize` | `0x2C7EF0` | `.text` |  |
| `kDataFetchEventInitialize` | `0x2C7F80` | `.text` |  |
| `kImpulseEventInitialize` | `0x2C7FF0` | `.text` |  |
| `at<...>()` | `0x3473E0` | `.text` |  |
| `kHaveParamsFromCreationKey` | `0x351750` | `.text` |  |
| `kGetACDataFromCreationKey` | `0x3518D0` | `.text` |  |
| `at<...>()` | `0x3554F0` | `.text` |  |
| `kModelPieceInstanceGeometry` | `0x3760C0` | `.text` |  |
| `kModelPieceDeactivate` | `0x3761C0` | `.text` |  |
| `kModelPieceActivate` | `0x379960` | `.text` |  |
| `kIsCharacterAllowedInPlayset` | `0x37F910` | `.text` |  |
| `kMasterAssetCreateFromOct` | `0x385CB0` | `.text` |  |
| `kBipedCOMDataInitRva` | `0x417B60` | `.text` |  |
| `kSecondPicker` | `0x457F70` | `.text` |  |
| `kEnableVisRegionCulling` | `0x8CAD30` | `.text` |  |
| `kRegisterGadgets` | `0x8CE460` | `.text` |  |
| `kPureCallRva` | `0xAB9B85` | `.text` |  |
| `kIsAlive` | `0xB79170` | `.text` |  |
| `kGetZoneName` | `0xB91040` | `.text` |  |
| `kGetZoneDataBool` | `0xB9DE80` | `.text` |  |
| `kGetZoneDataFloat` | `0xB9DFF0` | `.text` |  |
| `kGetZoneDataString` | `0xB9E0B0` | `.text` |  |
| `kZoneManagerIsToyBoxZone` | `0xB9E6E0` | `.text` |  |
| `at<...>()` | `0xBB28D0` | `.text` |  |
| `at<...>()` | `0xBBE450` | `.text` |  |
| `at<...>()` | `0xC10490` | `.text` |  |
| `at<...>()` | `0xC2CC20` | `.text` |  |
| `kSendAnimTreeEventInternal` | `0xC44350` | `.text` |  |
| `at<...>()` | `0xCE6F10` | `.text` |  |
| `kTwoDimensionalModeDataInitRva` | `0xCED5D0` | `.text` |  |
| `kConnectionsSetAtGrow` | `0xD2FA50` | `.text` |  |
| `kAiInteractionManagerGetPartner` | `0xD300A0` | `.text` |  |
| `at<...>()` | `0x10261F0` | `.text` |  |
| `at<...>()` | `0x105FDD0` | `.text` | type 1, heuristic 0 = OVERRIDE |
| `kModeExit` | `0x10601E0` | `.text` |  |
| `at<...>()` | `0x1061110` | `.text` |  |
| `at<...>()` | `0x1061140` | `.text` |  |
| `at<...>()` | `0x1074F50` | `.text` |  |
| `at<...>()` | `0x107A7F0` | `.text` |  |
| `at<...>()` | `0x107E350` | `.text` |  |
| `kGetActualVelocityNative` | `0x108AAB0` | `.text` |  |
| `kGetActualOrientationNative` | `0x108AB50` | `.text` |  |
| `kVelocityBeginFrame` | `0x10A4170` | `.text` |  |
| `kNoNameText` | `0x1329270` | `.text` |  |
| `kElementIntVtable` | `0x133F848` | `.text` |  |
| `kDataElementFloatVtable` | `0x133F880` | `.text` |  |
| `kElementBoolVtable` | `0x133F8B8` | `.text` |  |
| `kElementAgentVtable` | `0x133F8F0` | `.text` |  |
| `kElementVectorVtable` | `0x133F928` | `.text` |  |
| `kBipedCOMDataName` | `0x185ECCC` | `.rdata` |  |
| `kElementVectorClassType` | `0x193E42C` | `.rdata` |  |
| `kElementAgentClassType` | `0x193E76C` | `.rdata` |  |
| `kElementBoolClassType` | `0x193E82C` | `.rdata` |  |
| `kDataElementFloatClassType` | `0x193E84C` | `.rdata` |  |
| `kElementIntClassType` | `0x193E86C` | `.rdata` |  |
| `kTimerPauseContainer` | `0x1940194` | `.rdata` |  |
| `kForceAllCellsVisible` | `0x19AECB9` | `.rdata` |  |
| `kTimer` | `0x1A018F0` | `.rdata` |  |
| `kGravityManager` | `0x1A028D0` | `.rdata` |  |
| `kAllDataTypes` | `0x1A02930` | `.rdata` |  |
| `kEntryPoints` | `0x1A02934` | `.rdata` |  |
| `kAgentDataTypeCounter` | `0x1A02A40` | `.rdata` |  |
| `kZoneManager` | `0x1A5F548` | `.rdata` |  |
| `at<...>()` | `0x1A5F7E0` | `.rdata` |  |
| `kControlMapperDataType` | `0x1A60040` | `.rdata` |  |
| `at<...>()` | `0x1A899D8` | `.rdata` |  |
| `kExpendablesDataType` | `0x1A89AF8` | `.rdata` |  |
| `kAiInteractionManagerInstance` | `0x1A89E1C` | `.rdata` |  |
| `kGameSettingsInstance` | `0x1A8AA24` | `.rdata` |  |
| `kGravityDataType` | `0x1A8E41C` | `.rdata` |  |
| `kLocomotionDataType` | `0x1A8E4B8` | `.rdata` |  |
| `kMovementDataType` | `0x1A8E540` | `.rdata` |  |
| `Crabe.Memory/Hook` | `0x1E5F518` | `.data` |  |
| `Crabe.Memory/Hook` | `0x1E5F540` | `.data` |  |
