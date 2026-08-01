# NativeDB — Disney Infinity 3.0

What the engine exposes to the game's Lua. This is documentation, not code: nothing here is loaded in-game. The API in `src/api/10_game.lua` is written from this list.

Cyberpunk ships a full RTTI database, which is what lets CET resolve any native by name at runtime. Disney Infinity has nothing of the sort — the natives are plain C functions registered as globals in the `lua_State`. The reflection database here is literally `_G`, hence the dump.

## Regenerating

In-game (not in a menu — the engine's tables are not built yet), in the overlay console (`Insert`):

```lua
=Crabe.dump.all()          -- full report      -> crabe_dump.txt
=Crabe.dump.natives()      -- C natives only   -> crabe_natives.txt
=Crabe.dump.find("spark")  -- search, printed to the console
```

Files land in the game folder, next to `loader.log`.

`crabe_dump.txt` has three sections:

| Section            | Contents                                                                                                       |
| ------------------ | -------------------------------------------------------------------------------------------------------------- |
| `GLOBALS (_G)`   | everything a mod can reach;`function C` = engine native, `function Lua` = game script (with `file:line`) |
| `package.loaded` | the libraries the engine opened                                                                                |
| `registry`       | userdata metatables — the closest thing to a class list                                                       |

Constants matter as much as functions: screen names, editor states and enum-ish numbers are what the natives take as arguments.

### One dump is enough

The native surface does not change with the game mode. Dumps taken in Toy Box and in Rise of the Resistance both list **902 global C natives, with identical names**.

What differs is game-side Lua (890 vs 622 functions) and which **UI screen objects** happen to be instantiated — `ScreenSpace_Text`, `VirtualReaderPC_*` and `Storefront` in Toy Box, `Container`, `ButtonLegend` and `HUD_PowerDiskMeter` in Rise. Each carries the same 54 methods: it is one UI class instantiated under different global names, not extra API. Dumping in more places only widens the catalogue of screen names.

## `Crabe.*` vs game natives

Everything below is a native the *game engine* registered in `_G` — CrabeLoader only wraps it. `Crabe.*` is different: it is CrabeLoader's own API, and most of it (`Crabe.dump.*`, `Crabe.log`, …) is pure Lua with no engine involvement at all.

One exception: `Crabe.SetWindowMode`/`Crabe.GetWindowMode` (`src/api/05_window.lua`) wrap `Crabe._setWindowModeNative`, a real C++ function registered with `lua_pushcclosure` (`LuaRuntime::registerNatives`, `src/lua_runtime.cpp`) — not a wrapper around a game native, because no native anywhere in this dump exposes window/fullscreen/display state (exhaustive search: zero hits for `window`/`fullscreen`/`display`/`borderless`). It exists because Win32 window styling can only happen from C++, where `RenderHook` already holds the real `HWND`. Any future loader-side (as opposed to game-side) capability follows the same `_underscorePrefixed` native + ergonomic Lua wrapper pattern.

## Confirmed natives

Verified in-game. The rest of the dump is triaged as it gets used.

### Currency and UI

| Native                                | Signature                                                         | Notes                                                                                    |
| ------------------------------------- | ----------------------------------------------------------------- | ---------------------------------------------------------------------------------------- |
| `UI_GetSparks`                      | `() -> number`                                                  | Sparks = the Toy Box currency                                                            |
| `UI_IncrementSparks`                | `(amount)`                                                      | ⚠️**not** `Players_*RoundCoin*`, which is physical Power Discs — false friend |
| `UI_DisplayTextBox`                 | `(text, body, showLegend, num, playerID, screenType, priority)` | non-modal bubble;`screenType` tested: `"HelpBubble"`                                 |
| `UI_HideCursor` / `UI_ShowCursor` | `()`                                                            |                                                                                          |

### Players

| Native                      | Signature                              | Notes                                                                                                                                                            |
| --------------------------- | -------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `Players_GetHostPlayerID` | `() -> id`                           | argument to most`Place_*`                                                                                                                                      |
| `Players_IsCharacterDead` | `(playerID, characterIndex) -> bool` | takes**2** args, not 1 — `Game.IsCharacterDead` used to error with "bad argument #2 ... got no value" until fixed; `characterIndex` tested with `0` |

### Object placement

Generic: covers the ~4337 Toy Box objects. Order matters — without `Place_SetEditorState`, `Place_CalculateSpawnPosition` yields nothing and the ghost is created but never placed.

| Native                           | Signature                                                                                               |
| -------------------------------- | ------------------------------------------------------------------------------------------------------- |
| `Place_SetEditorState`         | `(playerID, state)` — tested with `"Editor::ObjectMode"`                                           |
| `Place_CalculateSpawnPosition` | `(playerID)`                                                                                          |
| `Place_CreateGhost`            | `(playerID, 0, rrofile) -> ghost` — `rrofile` = `INV_*` name without the `INV_` prefix         |
| `Place_PlaceObject`            | `(playerID, ghost)`                                                                                   |
| `Place_StopPlaceMode`          | `(playerID, 0, false)` — **must** run even after a failure, or the player's input stays locked |

### Catalog and misc UI

Called with zero or `(playerID)` and returned without error — tested from the console, one at a time, no crash.

| Native                      | Signature                | Notes                                                                                                        |
| --------------------------- | ------------------------ | ------------------------------------------------------------------------------------------------------------ |
| `UI_CurrentWorldName`     | `() -> string`         | tested:`"TBX_Onboard"`                                                                                     |
| `UI_GetPlayerZoneName`    | `(playerID) -> string` | tested:`"TBX_Onboard"`, same as the world name at the hub                                                  |
| `UI_GetInventoryToyCount` | `(playerID) -> number` | tested:`0` (no toys owned in this save)                                                                    |
| `UI_GetGamerTag`          | `(playerID) -> string` | tested: empty string — no error, plausibly not signed into a platform identity                              |
| `Catalog_GetList`         | `() -> string`         | tested: empty string — no error, plausibly nothing pending in this save                                     |
| `Catalog_GetItemInfo`     | `(itemName) -> string` | tested with`"Test"` (not a real item): empty string, no error — takes a string here, unlike the two below |

## To verify — wired into Game.*

Named clearly enough to wire up, still guessed. `Game.*` calls them through a `pcall` — move a row up once the console confirms its behavior.

| Native                         | Guessed signature                | Notes                                                                                                                                                                                                                                                                |
| ------------------------------ | -------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `Catalog_UnlockCatalogItem`  | `(playerID, itemId: number)`   | unlocks a catalog entry (menus/Toy Box list) rather than spawning it.**Confirmed arg #2 is a number, not the item's string name** — first guess errored with "bad argument #2 ... number expected, got string". The numeric id scheme itself is still unknown |
| `Catalog_AwardInventoryItem` | `(playerID, itemId: number)`   | grants possession directly, as opposed to`UnlockCatalogItem`; same numeric-id correction                                                                                                                                                                           |
| `UI_ListInventoryToys`       | `(playerID, category: number)` | **confirmed 2 args, not 1** — errored with "bad argument #2 ... number expected, got no value". What the second number selects is unknown                                                                                                                     |
| `UI_QuitGame`                | `()`                           | untested — exits the process                                                                                                                                                                                                                                        |
| `UI_ReturnToHub`             | `()`                           | untested — leaves the current screen                                                                                                                                                                                                                                |
| `Players_GetRoundCoin`       | `(playerID) -> number`         | Round Coins = physical Power Disc currency, distinct from Sparks                                                                                                                                                                                                     |
| `Players_SetRoundCoins`      | `(playerID, amount)`           |                                                                                                                                                                                                                                                                      |
| `Players_GetAvatarLevel`     | `(playerID) -> number`         |                                                                                                                                                                                                                                                                      |
| `Players_AvatarLevelUp`      | `(playerID)`                   |                                                                                                                                                                                                                                                                      |
| `Players_NumLocalPlayers`    | `() -> number`                 |                                                                                                                                                                                                                                                                      |
| `Players_NumPlayers`         | `() -> number`                 |                                                                                                                                                                                                                                                                      |
| `Players_IsValid`            | `(playerID) -> bool`           |                                                                                                                                                                                                                                                                      |
| `Catalog_BuyItem`            | `(playerID, itemId: number)`   | same numeric-id guess as`Catalog_UnlockCatalogItem`/`Catalog_AwardInventoryItem`                                                                                                                                                                                 |
| `UI_GetEarnedStarCount`      | `(playerID) -> number`         |                                                                                                                                                                                                                                                                      |
| `UI_GetTotalStarCount`       | `(playerID) -> number`         |                                                                                                                                                                                                                                                                      |
| `UI_IsHost`                  | `() -> bool`                   |                                                                                                                                                                                                                                                                      |
| `UI_GetCurrentLanguage`      | `() -> string`                 |                                                                                                                                                                                                                                                                      |
| `UI_PlayAudio`               | `(handle)`                     | argument shape is a pure guess — could be a sound bank name rather than a handle                                                                                                                                                                                    |
| `UI_KillAudio`               | `(handle)`                     | same guess as above                                                                                                                                                                                                                                                  |

Console testing is one command at a time (`=Game.X(...)`), the same way the dump commands are run — not batched in a mod. A wrong argument to an unverified native can crash the process outright (`pcall` only catches genuine Lua errors, not that), so batching them together just makes the crash harder to attribute to a specific call.

## To verify — full catalog

Everything else the dump found: no guessed signature, no `Game.*` wrapper, not called even once. Grouped by prefix, largest first. If you test one, move it into the tables above with what you found (signature, return value, or "crashes with X args") — same one-command-at-a-time method as above, since a wrong argument can crash the process outright.

A native's own name is usually the best hint for its arguments — `Set*`/`Get*` pairs, `*ById`/`*ByName` variants, and the family it belongs to (its neighbors in the same prefix tend to share a calling convention, as seen with `Catalog_*` taking a numeric id).

<details><summary><strong>UI_* (316)</strong></summary>

`UI_ActivateSpeedwayHubMission`, `UI_ActivateToyMaker`, `UI_AddCloudSaveDeleteAllOption`, `UI_AddDebugMenuOption`, `UI_AddEnemyToList`, `UI_AddFriend`, `UI_AddSideKickToSideKickToyRoster`, `UI_AddToyToInventoryToy`, `UI_AddToyToLoot`, `UI_AdvanceOnboardTutorialState`, `UI_AllToysUnlocked`, `UI_AllowArcadeMenuItem`, `UI_AllowTestUI`, `UI_ApplyLanguageChange`, `UI_ApplyResolutionChange`, `UI_ButtonQueue`, `UI_CanContinuePlayset`, `UI_CanEditTextToy`, `UI_CanItemBePurchased`, `UI_CanSelectValidSelection`, `UI_CanTransitionToLevel`, `UI_CarryItem`, `UI_ChangePelletCount`, `UI_CheckCCSignIn`, `UI_CheckForAgeBandResult`, `UI_CheckForGeoIPResult`, `UI_CheckForLoginResult`, `UI_CheckFreeSpaceForPackage`, `UI_CheckSignIn`, `UI_CombatTutorialActive`, `UI_ContinuePlayset`, `UI_DeleteLink`, `UI_DisneyID_Cancel`, `UI_DisneyID_CheckUpdate`, `UI_DisneyID_Create`, `UI_DisneyID_GetAccountType`, `UI_DisneyID_GetChildPrivacyPolicyBody`, `UI_DisneyID_GetChildPrivacyPolicyFlag`, `UI_DisneyID_GetCompanyOverviewBody`, `UI_DisneyID_GetCompanyOverviewFlag`, `UI_DisneyID_GetCookiesPolicy`, `UI_DisneyID_GetCookiesPolicyBody`, `UI_DisneyID_GetCookiesToggleText`, `UI_DisneyID_GetMarketingDefault`, `UI_DisneyID_GetMarketingToggleText`, `UI_DisneyID_GetMoreInfoText`, `UI_DisneyID_GetPrefillData`, `UI_DisneyID_GetPrivacyPolicyBody`, `UI_DisneyID_GetSEAPolicyBody`, `UI_DisneyID_GetSEAPolicyFlag`, `UI_DisneyID_GetSEAToggleText`, `UI_DisneyID_GetTermsOfUseBody`, `UI_DisneyID_GetToSToggleText`, `UI_DisneyID_GetUpdateFields`, `UI_DisneyID_GotoCreate`, `UI_DisneyID_GotoLegal`, `UI_DisneyID_GotoSignInSignUp`, `UI_DisneyID_Login`, `UI_DisneyID_NeedsPartialUpdate`, `UI_DisneyID_RequestAgeBand`, `UI_DisneyID_ResetPassword`, `UI_DisneyID_Update`, `UI_DisplayDatePicker`, `UI_DisplayEnemyList`, `UI_DisplayEntireInventory`, `UI_DisplayKeyboard`, `UI_DisplayLootList`, `UI_DisplayScrollList`, `UI_DisplaySideKickInventory`, `UI_DoesOnboardTutorialNeedContinueInput`, `UI_EditLink`, `UI_FinishedRaceResults`, `UI_ForceRumpuseUnlockLoadData`, `UI_ForceUnpauseAllLocalPlayers`, `UI_GameIsLocked`, `UI_GetAvatarNameForIGPPopup`, `UI_GetAvatarSKU`, `UI_GetAvatarTypeGridData`, `UI_GetAvatarVersion`, `UI_GetCDNError`, `UI_GetChunkDownloadPercent`, `UI_GetCollectablesData`, `UI_GetCrossoverData`, `UI_GetCurrentResolution`, `UI_GetCustomData`, `UI_GetCustomDataTable`, `UI_GetDateFormat`, `UI_GetDefaultToyBoxLevels`, `UI_GetDestinationZone`, `UI_GetDiskCollectionData`, `UI_GetEconomyDOD`, `UI_GetFilteredZoneList`, `UI_GetGameVersion`, `UI_GetGrandPrixMissionName`, `UI_GetGrandPrixRaceResults_PointsAwarded`, `UI_GetGrandPrixRaceResults_Standings`, `UI_GetHealthPercentageOfCore`, `UI_GetInventoryFilter`, `UI_GetIsChallengeMissionBehavior`, `UI_GetItemArgumentsForPlayer`, `UI_GetItemArgumentsForPlayerTable`, `UI_GetItemDetails`, `UI_GetItemDetailsForPlayer`, `UI_GetItemDetailsTable`, `UI_GetItemParentHeirarchy`, `UI_GetList`, `UI_GetListAndLocks`, `UI_GetListAndLocksForPlayer`, `UI_GetListForActor`, `UI_GetListForPlayer`, `UI_GetListForRumpusObject`, `UI_GetListPlayerIndx`, `UI_GetNumSupportedLanguages`, `UI_GetNumSupportedResolutions`, `UI_GetNumberOfCores`, `UI_GetNumberOfScoreboards`, `UI_GetOnboardTutorialListIndex`, `UI_GetOnboardTutorialStateString`, `UI_GetPackageDownloadPercent`, `UI_GetPartyEmpty`, `UI_GetPlayerIcon`, `UI_GetPlayerUIDString`, `UI_GetPlaysetCurrency`, `UI_GetPlaysetIcon`, `UI_GetRaceHubData_EngineSizeData`, `UI_GetRaceHubData_GrandPrixSelectData`, `UI_GetRaceHubData_LevelSelectData`, `UI_GetRaceHubData_RaceModeData_BattleRace`, `UI_GetRaceHubData_RaceModeData_Race`, `UI_GetReaderAvatarCount`, `UI_GetRibbonName`, `UI_GetScaledIconText`, `UI_GetScoreboardValue`, `UI_GetSkillTreeGridData`, `UI_GetSpeedwayData`, `UI_GetSpinCount`, `UI_GetStarMapPositions`, `UI_GetSupportedLanguage`, `UI_GetSupportedResolution`, `UI_GetTimeFormat`, `UI_GetToyTreeGridData`, `UI_GetTrackProgressData`, `UI_GetTrophyData_GrandPrix`, `UI_GetTrophyData_Race`, `UI_GetTutorialCombatData`, `UI_GetUsernames`, `UI_GetValidScreenSelection`, `UI_GetZoneMgrBool`, `UI_GetZoneMgrInt`, `UI_GetZoneMgrPlaysetBool`, `UI_GetZoneMgrString`, `UI_GetiOSPackageName`, `UI_HasEnoughFreeSpace`, `UI_HasGlobalURLs`, `UI_HasPSXAccess`, `UI_HideNonModalAlerts`, `UI_HostingNetworkedGame`, `UI_InBattleArenaCampaign`, `UI_InReplayMission`, `UI_InfiniteData`, `UI_InitiateDisneyID`, `UI_InviteToParty`, `UI_IsAnyPlayerOnline`, `UI_IsApplicationDownloaded`, `UI_IsAudioHandleValid`, `UI_IsAvatarAlive`, `UI_IsAvatarOnReader`, `UI_IsCloudMigrationComplete`, `UI_IsCloudMigrationEnabled`, `UI_IsCombatTutorialAllowed`, `UI_IsCurrentWorldADungeon`, `UI_IsCurrentWorldATransition`, `UI_IsCurrentWorldATutorial`, `UI_IsCurrentWorldAnIntro`, `UI_IsCurrentWorldDefense`, `UI_IsDevice16_10`, `UI_IsDevice16_9`, `UI_IsDevice3_2`, `UI_IsDevice4_3`, `UI_IsDevice5_3`, `UI_IsDialoguePlaying`, `UI_IsGameConnectedToWiFi`, `UI_IsGameInSpace`, `UI_IsGamePlayCombinedScreen`, `UI_IsGamePlayHorizontalSplit`, `UI_IsGamePlayVerticalSplit`, `UI_IsGameSplineFlying`, `UI_IsGlobalSet`, `UI_IsInInsideOutMiniGame`, `UI_IsInSpaceportOrStarmap`, `UI_IsLoggedIn`, `UI_IsMetalEnabled`, `UI_IsOnboardTutorialActive`, `UI_IsPRDemo`, `UI_IsPhone`, `UI_IsPlalaGame`, `UI_IsReaderLockedForPlayerID`, `UI_IsSampleAvatar`, `UI_IsScreenActive`, `UI_IsSelectionAWorld`, `UI_IsSinglePlayerPauseOnly`, `UI_IsSteamGame`, `UI_IsSystemUIVisible`, `UI_IsTOGOOfflineGame`, `UI_IsTablet`, `UI_IsWorldAPlayset`, `UI_IsWorldAUserToyBox`, `UI_KeyboardUsedLast`, `UI_KickPlayer`, `UI_KillCombatTutorial`, `UI_KillDialogue`, `UI_KillLogicToyActionMode`, `UI_LaunchDefaultLevel`, `UI_LaunchDisneyStore`, `UI_LaunchIntro`, `UI_LaunchIntroLevel`, `UI_LaunchLevel`, `UI_LaunchMainMenu`, `UI_LaunchOnboardIntroLevel`, `UI_LaunchXB1Help`, `UI_LoadPlaysetDictionary`, `UI_LoadProgressData`, `UI_LoadSocialHubNews`, `UI_LoadUnlockedProgressData`, `UI_LobbyToyAllIn`, `UI_LocTestPlayDialog`, `UI_LockGame`, `UI_LogicOptionCategoryPicked`, `UI_MakeOnlineInvite`, `UI_MissionInputEnabled`, `UI_MoreDisney`, `UI_MoveToLink`, `UI_OtherPlayerHasActiveScreen`, `UI_PerformResetAvatar`, `UI_PlayDialogue`, `UI_PlayerInRace`, `UI_ProcessPurchasedSkills`, `UI_PurchaseToyFromVendor`, `UI_RaceHubData_Reset`, `UI_RegisterSkillTreeShortcut`, `UI_RemoveBuddy`, `UI_RemoveEnemyFromLoot`, `UI_RemoveScreensForZoneUnload`, `UI_RemoveToyFromInventoryToy`, `UI_RemoveToyFromLoot`, `UI_ResetPlayset`, `UI_ResetToyBox`, `UI_ScreenAlreadyAdded`, `UI_ScreenAspectRatio`, `UI_ScreenHeight`, `UI_ScreenSplitDirection`, `UI_ScreenViewports`, `UI_ScreenWidth`, `UI_SelectActions`, `UI_SelectAttributeOption`, `UI_SelectAttributes`, `UI_SelectBehavior`, `UI_SelectLink`, `UI_SelectingValidScreenSelection`, `UI_SendMessageString`, `UI_SetAttributeValue`, `UI_SetContext`, `UI_SetCustomizationCategory`, `UI_SetOnboardTutorialCompleted`, `UI_SetOnboardTutorialState`, `UI_SetPackageToTopPriority`, `UI_SetRaceHubData_AiDifficulty`, `UI_SetRaceHubData_RaceCC`, `UI_SetRaceHubData_RaceRuleSet`, `UI_SetRaceHubData_RaceTrack`, `UI_SetRaceHubData_RaceWeaponsAllowed`, `UI_SetToyValue`, `UI_SetupSpeedwayRace`, `UI_ShouldOnboardTutorialRestrictInput`, `UI_ShouldShowIntroPrompt`, `UI_ShowCharacterSampleScreen`, `UI_ShowFeats`, `UI_ShowFriendGamerCard`, `UI_ShowGamerCard`, `UI_ShowOnboardTutorialOverlay`, `UI_SideKickCustomize`, `UI_SignOut`, `UI_SpawnSideKick`, `UI_SpawnToyFromKiosk`, `UI_SpawnToyFromTree`, `UI_SuppressHud`, `UI_TakeOwnership`, `UI_ToggleButton`, `UI_ToyMakerExiting`, `UI_TriggerEmpireStorefront`, `UI_TriggerEvent`, `UI_TriggerHitchingPost`, `UI_TriggerInventorySelection`, `UI_TriggerLootChestSelection`, `UI_TriggerLootToySelection`, `UI_TriggerParticleEffect`, `UI_TriggerSkillTree`, `UI_TriggerStorefrontSelection`, `UI_TriggerTestUI`, `UI_TriggerToolPackSelection`, `UI_TriggerToyTree`, `UI_TriggerTutorialCombat`, `UI_TriggerWaveEnemySelection`, `UI_UnlinkDisneyID`, `UI_UnloadPlaysetDictionary`, `UI_UnlockGame`, `UI_UpdateFriendList`, `UI_UpdateLogicConnectionCounts`, `UI_UpdateLogicSelection`, `UI_UpdateToyChance`, `UI_UseBigDisneyLogo`, `UI_UseVirtualReader`, `UI_ViewFriends`, `UI_ViewInvitations`, `UI_ViewOnlineCommunity`, `UI_WhiteWashText`

</details>

<details><summary><strong>Misc / no prefix (120)</strong></summary>

`AbortActiveMission`, `AbortActivity`, `AbortMission`, `AddMusicCue`, `BlockFriend`, `BuddyApplyItem`, `BuddyGetData`, `BuddyTriggerUI`, `BuddyUILocked`, `CabinetToySelected`, `CanPlayBinkMovie`, `ChatWithFriend`, `CheckCleanspeakTransactionStatus`, `ClearNewsPaper`, `ClearNewsSection`, `ControlContextActive`, `CreateRibbonScreen`, `DeleteAllSavegames`, `DeleteFriend`, `DeleteInboxMessage`, `DisableIGP`, `DisableReader`, `DisableSubtitle`, `EnableSubtitle`, `GameSaveLoadUGCScreenExit`, `GatewayConnected`, `GatewayHasRestrictedContent`, `GatewayPlayset`, `GatewayPlaysetAndFile`, `GetActiveTools`, `GetAgeBand`, `GetBlockList`, `GetControllerSetting`, `GetDisneyID`, `GetFriendsList`, `GetGamepadEnabled`, `GetLockedControllerIndex`, `GetMouseSensitivity`, `GetMyUserName`, `GetOptionsVolume`, `GetPlayerAvatarData`, `GetPlayerNumber`, `GetPlayerNumberFromViewport`, `GetQuickSelectTool`, `GetToolNameFromAutoEquipName`, `GetToolSelectDisplayType`, `GetUnlockedController`, `GetUnlockedControllerDeviceName`, `GetUpsellAcceptPercentageForType`, `GetUpsellCountForType`, `GetViewportCount`, `GetZoneFromInvitation`, `HideNewsfeed`, `InitiateHorcruxDownload`, `InviteFriend`, `IsControllerLocked`, `IsIntroChildWorld`, `IsInviteAllowed`, `IsMultiplayerAllowed`, `IsOnline`, `IsOnlineContentAllowed`, `IsPlayerInVehicle`, `IsPlayerSpectator`, `IsRibbonFilterRestricted`, `IsSaveToyBoxGameAvailable`, `IsSignedIntoDisneyID`, `IsSignedIntoPlatform`, `IsToolLocked`, `IsWaitingForSession`, `IsZoneLoaded`, `JoinFriend`, `LockOtherPlayerControls`, `LockPlayerToController`, `LogAction`, `LogUIScreenView`, `LogUIScreenViewStepTime`, `LuaDumpObject`, `Mute`, `PlayAwardCho`, `PlayBinkMovie`, `PlayFMV`, `PlaySfx`, `PlayStingerCue`, `PlayerInActiveMission`, `PlayerInActivity`, `PlayerInExclusiveChallengeMission`, `PlayerInIntro`, `PressStartScreenOnEnter`, `RemovePlayer`, `RequestInboxImage`, `RequestInboxMessageCount`, `RequestInboxMessageDetails`, `RequestInboxMessages`, `RequestLoadingScreenNews`, `RequestNews`, `SendMessageToHelpManager`, `SetActiveTool`, `SetControlContext`, `SetControllerSetting`, `SetMouseSensitivity`, `SetOptionsVolume`, `SetQuickSelectTool`, `SetUGCIDForRumpusToy`, `ShowDebugInfoPopup`, `ShowNewsfeed`, `StopMusic`, `ToyIsGuest`, `ToyboxTVIsAvailable`, `TrackUpsellResult`, `TransmitChatText`, `UnMute`, `UnblockFriend`, `UninviteFriend`, `UnloadScreenShot`, `UnlockAllControllers`, `UnlockPlayerFromController`, `UnpauseAllLocalGuests`, `UpdateNewsNotConnected`, `UploadAllSavegames`, `UploadAllSavegamesInitial`

</details>

<details><summary><strong>Place_* (68)</strong></summary>

`Place_ActivateGroupSelect`, `Place_AddFavorite`, `Place_AddPerfMeter`, `Place_AddSplinePoint`, `Place_BranchSpline`, `Place_CanPlaceRumpusObject`, `Place_CanSummonVehicle`, `Place_CancelEditorIdle`, `Place_ClearEditorObjects`, `Place_ClearStartupData`, `Place_CollectionGetInvItems`, `Place_ConfirmDeleteObject`, `Place_ConnectSplineBranch`, `Place_ConnectSplineLoop`, `Place_CreateGhostAtLocation`, `Place_DeactivateGroupSelect`, `Place_DeletePickedObject`, `Place_DeletePreviewObject`, `Place_DoSparkSelection`, `Place_EditRumpusObjectFromSpark`, `Place_ExitLogicIfEnabled`, `Place_FocusGained`, `Place_FocusLost`, `Place_GetAvatarHandle`, `Place_GetInvItems`, `Place_GetInventoryFast`, `Place_GetInventoryFast2`, `Place_GetObjectIntersect`, `Place_GetObjectName`, `Place_GetRCCardItems`, `Place_GetSparkSelectionRodbName`, `Place_GetStartupData`, `Place_GroupSelect`, `Place_IncrementObjectsUsedCount`, `Place_IsFavorite`, `Place_IsNew`, `Place_IsUserCategoryAvailable`, `Place_LimitManagerCanPlace`, `Place_LimitManagerTotalNetworkValue`, `Place_LimitManagerTotalValue`, `Place_LoadFinished`, `Place_LogicRumpusObject`, `Place_PassThroughMouseValues`, `Place_PlaceObjectNearAvatar`, `Place_PlaceObjectOnExit`, `Place_PlayCommercialForLock`, `Place_RemoveEditorObjects`, `Place_RemoveFavorite`, `Place_RemovePerfMeter`, `Place_ReturnFromSpline`, `Place_ReturnToSparkIfNeeded`, `Place_RibbonEnded`, `Place_RibbonStarted`, `Place_SavePreviewTransform`, `Place_SelectObject`, `Place_SetActiveVirtualController`, `Place_SetCurrentCategory`, `Place_SetCurrentItem`, `Place_SetInitialSelection`, `Place_SetShouldReturnToSpark`, `Place_ShouldReturnToSpark`, `Place_ShowConfirmDeleteObject`, `Place_SummonVehicle`, `Place_TellTutorialUserScrolledCategory`, `Place_TellTutorialUserScrolledObject`, `Place_ToggleCameraRotateDisabled`, `Place_ToggleMouseActive`, `Place_UpdateConfirmDeleteObjectStatus`

</details>

<details><summary><strong>VirtualReaderPC_* (46)</strong></summary>

`VirtualReaderPC_ActivateChanges`, `VirtualReaderPC_AreAllItemsLocked`, `VirtualReaderPC_AvatarCheckpointRespawn`, `VirtualReaderPC_AvatarLevelUp`, `VirtualReaderPC_GetAvatarAbilitiesByName`, `VirtualReaderPC_GetAvatarEntitlements`, `VirtualReaderPC_GetBrandFromCurrentPlaySet`, `VirtualReaderPC_GetBrandFromName`, `VirtualReaderPC_GetCurrentCharacter`, `VirtualReaderPC_GetCurrentHexCoins`, `VirtualReaderPC_GetCurrentRoundCoins`, `VirtualReaderPC_GetCurrentSortMethod`, `VirtualReaderPC_GetFilterButtonData`, `VirtualReaderPC_GetFilterButtonStrings`, `VirtualReaderPC_GetItemByName`, `VirtualReaderPC_GetItemByPage`, `VirtualReaderPC_GetMaxNumCoins`, `VirtualReaderPC_GetNumberOfTrialSlots`, `VirtualReaderPC_GetSearchSuggestion`, `VirtualReaderPC_GetTrialCharacters`, `VirtualReaderPC_GetUpsellBundles`, `VirtualReaderPC_InitWinRTStore`, `VirtualReaderPC_IsInsideOutPlayset`, `VirtualReaderPC_IsItemAvatar`, `VirtualReaderPC_IsItemAvatarBundle`, `VirtualReaderPC_IsItemHexCoin`, `VirtualReaderPC_IsItemHexCoinBundle`, `VirtualReaderPC_IsItemLocked`, `VirtualReaderPC_IsItemMiscLock`, `VirtualReaderPC_IsItemPlayset`, `VirtualReaderPC_IsItemSparkPack`, `VirtualReaderPC_IsItemStarterPack`, `VirtualReaderPC_IsItemToyBoxGame`, `VirtualReaderPC_OpenSteamStoreOverlay`, `VirtualReaderPC_PlaysetCheckInGame`, `VirtualReaderPC_Purchase`, `VirtualReaderPC_RemoveHexCoin`, `VirtualReaderPC_RemoveRoundCoin`, `VirtualReaderPC_SetCurrentCharacter`, `VirtualReaderPC_SetCurrentHexCoin`, `VirtualReaderPC_SetCurrentRoundCoin`, `VirtualReaderPC_SetCurrentSortMethod`, `VirtualReaderPC_SetData`, `VirtualReaderPC_ShouldShowTrialCharacters`, `VirtualReaderPC_ShowTrialCharactersScreen`, `VirtualReaderPC_ValidateCurrentCharacter`

</details>

<details><summary><strong>UGC_* (38)</strong></summary>

`UGC_CanDelete`, `UGC_CanSaveNewLocal`, `UGC_CanUploadToCloud`, `UGC_CloudSaveByDefault`, `UGC_CloudSaveDeleteAll`, `UGC_DeleteContent`, `UGC_GetClientPageSize`, `UGC_GetDownloadSize`, `UGC_GetFilters`, `UGC_GetFiltersChoices`, `UGC_GetSearchCategories`, `UGC_GetSearchCategoryString`, `UGC_GetSearchTerm`, `UGC_GetSelectedCloudID`, `UGC_GetUGCTarget`, `UGC_IsLegalContentRequest`, `UGC_IsLegalParentalCheck`, `UGC_IsLevelEditable`, `UGC_IsPlayingUGCLevel`, `UGC_IsPlaysetUGC`, `UGC_IsPublic`, `UGC_IsUGCAllowed`, `UGC_LaunchLevel`, `UGC_LikeContent`, `UGC_ReportContent`, `UGC_RequestAllFriendsToyBoxes`, `UGC_RequestContent`, `UGC_RequestContentItemCount`, `UGC_RequestFriends`, `UGC_RequestList`, `UGC_RequestScreenShot`, `UGC_SetCreatorChoice`, `UGC_SetFilter`, `UGC_SetFiltersByIndex`, `UGC_SetSearchCategory`, `UGC_SetSearchTerms`, `UGC_SetTarget`, `UGC_UploadContent`

</details>

<details><summary><strong>Customize_* (30)</strong></summary>

`Customize_ChangeSlider`, `Customize_ClearActorBeingCustomizedFlag`, `Customize_ClearNewFlag`, `Customize_ClearStartupData`, `Customize_EnterPickModeRumpus`, `Customize_FinalizeActor`, `Customize_GetActorName`, `Customize_GetGenderCategory`, `Customize_GetSliderInfo`, `Customize_GetStartupData`, `Customize_InformCustomizeBegin`, `Customize_InformCustomizeEnd`, `Customize_InteriorThemeAll`, `Customize_IsLocked`, `Customize_IsNew`, `Customize_ItemApplied`, `Customize_LoadItem`, `Customize_ModelHasLocators`, `Customize_MoveCamera`, `Customize_NextTrainSlot`, `Customize_SetActorBeingCustomizedFlag`, `Customize_SetCurrentAsDefault`, `Customize_SetTheme`, `Customize_StartCustomizeCamera`, `Customize_StopCustomizeCamera`, `Customize_StoreInitialValues`, `Customize_ThemeAll`, `Customize_Undo`, `Customize_UpdateBoatCustomizationCameraFocus`, `Customize_YieldExtraFiberTime`

</details>

<details><summary><strong>Settings_* (29)</strong></summary>

`Settings_ApplyVideoSettings`, `Settings_GetBloomEnabled`, `Settings_GetDepthOfFieldEnabled`, `Settings_GetDifficulty`, `Settings_GetDynamicResolutionEnabled`, `Settings_GetFXAAEnabled`, `Settings_GetGamma`, `Settings_GetHelpEnable`, `Settings_GetMotionBlurEnabled`, `Settings_GetNumResolutions`, `Settings_GetResolutionHeight`, `Settings_GetResolutionWidth`, `Settings_GetRetailTheme`, `Settings_GetSSAOEnabled`, `Settings_GetSubtitle`, `Settings_GetTouchEnabled`, `Settings_Save`, `Settings_SetBloomEnabled`, `Settings_SetDepthOfFieldEnabled`, `Settings_SetDifficulty`, `Settings_SetDynamicResolutionEnabled`, `Settings_SetFXAAEnabled`, `Settings_SetGamma`, `Settings_SetHelpEnable`, `Settings_SetMotionBlurEnabled`, `Settings_SetResolution`, `Settings_SetRetailTheme`, `Settings_SetSSAOEnabled`, `Settings_SetSubtitle`

</details>

<details><summary><strong>SaveLoad_* (29)</strong></summary>

`SaveLoad_AllowUpdate`, `SaveLoad_AutoSaveProfile`, `SaveLoad_AutoSaveWorld`, `SaveLoad_CanSave`, `SaveLoad_CloudSaveInteriorAndIntro`, `SaveLoad_CopySave`, `SaveLoad_CreateEmptySave`, `SaveLoad_Delete`, `SaveLoad_FindFirstAvailableSet`, `SaveLoad_FindFirstAvailableSlot`, `SaveLoad_GetCurrentLevelInfo`, `SaveLoad_GetLastSetAndLevel`, `SaveLoad_GetLevelsFromSetIndex`, `SaveLoad_GetLoadResult`, `SaveLoad_GetPlaysetFilter`, `SaveLoad_GetSavedGamesList`, `SaveLoad_GetSlot`, `SaveLoad_IsSaving`, `SaveLoad_Load`, `SaveLoad_LocalSaveExists`, `SaveLoad_PageDown`, `SaveLoad_PageUp`, `SaveLoad_PlaysetContinueIsFinished`, `SaveLoad_Save`, `SaveLoad_SaveProfile`, `SaveLoad_SetIsPlayset`, `SaveLoad_SetPlaysetFilter`, `SaveLoad_ShowAutoSave`, `SaveLoad_ShowMessage`

</details>

<details><summary><strong>System_* (19)</strong></summary>

`System_AcceptVoiceInput`, `System_CheckForPendingInvitation`, `System_GetLanguage`, `System_GetVersionString`, `System_IgnoreVoiceInput`, `System_InFrontEnd`, `System_InFrontEndOld`, `System_LockController`, `System_SendMessage`, `System_SendMessageFromPlayer`, `System_ShowPhoneticSpellings`, `System_StartButtonPushed`, `System_TimerFrameDeltaSec`, `System_TimerFrameNum`, `System_TimerSec`, `System_TimerSecNow`, `System_TimerSecPausable`, `System_UnlockAllControllers`, `System_ValidStartingController`

</details>

<details><summary><strong>Players_* (15)</strong></summary>

`Players_ChangeAvatar`, `Players_ClearLastUnavailableTagUID`, `Players_ForceAvatar`, `Players_GetAvatarEntitlements`, `Players_GetHexCoin`, `Players_GetLocalPlayers`, `Players_GetRoundCoinsLeftById`, `Players_HandleDeathInput`, `Players_IsPlayerFirstLocalPlayer`, `Players_MaxPlayers`, `Players_RedeemCode`, `Players_RemovePlayerDeathInfo`, `Players_SetAvatarProgression`, `Players_SetHexCoins`, `Players_SetPurchasedRoundCoinsById`

</details>

<details><summary><strong>DailyQuest_* (13)</strong></summary>

`DailyQuest_ClearPurchaseRules`, `DailyQuest_ClearQuests`, `DailyQuest_ExchangeQuest`, `DailyQuest_GetAllQuests`, `DailyQuest_GetItemByName`, `DailyQuest_GetPurchaseRules`, `DailyQuest_GetQuest`, `DailyQuest_GetReward`, `DailyQuest_IsOnboardingQuest`, `DailyQuest_IsPurchasingAllowed`, `DailyQuest_SetData`, `DailyQuest_UpdatePurchasePenalty`, `DailyQuest_UpdateQuests`

</details>

<details><summary><strong>Catalog_* (13)</strong></summary>

`Catalog_ClearPurchasableNew`, `Catalog_Create`, `Catalog_Destroy`, `Catalog_GetPlayerInfo`, `Catalog_GetProfileIcons`, `Catalog_LaunchDisneyStore`, `Catalog_MarkIfImageNotLoaded`, `Catalog_MarkIfSubImageNotLoaded`, `Catalog_MixItem`, `Catalog_RequestImages`, `Catalog_RequestProfileUpdate`, `Catalog_RewardLockByName`, `Catalog_Update`

</details>

<details><summary><strong>LastPlayed_* (10)</strong></summary>

`LastPlayed_GetIsSpecialLastPlayed`, `LastPlayed_GetLastLevelDisplayName`, `LastPlayed_GetLastLevelInformation`, `LastPlayed_GetLastPlayedLevel`, `LastPlayed_GetLastPlaysetFilter`, `LastPlayed_GetLastUGCScreenName`, `LastPlayed_SetCanSetLastPlayed`, `LastPlayed_SetIsSpecialLastPlayed`, `LastPlayed_SetLastLevelDisplayName`, `LastPlayed_SetLastUGCScreenName`

</details>

<details><summary><strong>Activity_* (10)</strong></summary>

`Activity_AcceptInvite`, `Activity_CancelInfo`, `Activity_CancelLobby`, `Activity_DeclineInvite`, `Activity_SetDifficulty`, `Activity_ShowInvite`, `Activity_StartActivityInvite`, `Activity_StartLocalActivity`, `Activity_StartNetworkActivity`, `Activity_StoppedShowingLobby`

</details>

<details><summary><strong>VirtualReader_* (9)</strong></summary>

`VirtualReader_AddToPlacedObjectCount`, `VirtualReader_DidRetrieveSampleCharacters`, `VirtualReader_GetCurrentAvatarSku`, `VirtualReader_GetList`, `VirtualReader_GetPlacedObjectCount`, `VirtualReader_GetSampleCharacterId`, `VirtualReader_GetSampleExpirationTimeString`, `VirtualReader_PurchaseItem`, `VirtualReader_SetupForNewAccount`

</details>

<details><summary><strong>Mission_* (8)</strong></summary>

`Mission_GetMissionDetails`, `Mission_GetMissionObjectives`, `Mission_GetMissionRewards`, `Mission_MissionAbortReplay`, `Mission_MissionAccepted`, `Mission_MissionActivated`, `Mission_MissionLogExit`, `Mission_MissionReplay`

</details>

<details><summary><strong>Pause_* (7)</strong></summary>

`Pause_ExitGame`, `Pause_Get2ndPlayerQuitString`, `Pause_OKToSaveInPause`, `Pause_PauseGame`, `Pause_TestWiiDiskEject`, `Pause_UnPauseFromPausedScreen`, `Pause_UnPauseFromPausedScreenIfPaused`

</details>

<details><summary><strong>Prestige_* (6)</strong></summary>

`Prestige_Clear`, `Prestige_GetObjectives`, `Prestige_GetStats`, `Prestige_GetTitles`, `Prestige_RequestImage`, `Prestige_SaveTitle`

</details>

<details><summary><strong>ToyboxController_* (5)</strong></summary>

`ToyboxController_ButtonDown`, `ToyboxController_ButtonQueue`, `ToyboxController_ButtonUp`, `ToyboxController_IsVirtualControl`, `ToyboxController_ToggleState`

</details>

<details><summary><strong>DailyChallenge_* (5)</strong></summary>

`DailyChallenge_BeginDailyChallenge`, `DailyChallenge_IsDailyChallengeActive`, `DailyChallenge_IsDailyChallengeAvailable`, `DailyChallenge_IsDailyChallengeComplete`, `DailyChallenge_SetDailyChallengeDifficulty`

</details>

<details><summary><strong>Translator_* (4)</strong></summary>

`Translator_FormattedOutput`, `Translator_FormattedOutputKey`, `Translator_FormattedOutputKeyDefault`, `Translator_TranslateString`

</details>

<details><summary><strong>Network_* (4)</strong></summary>

`Network_RemoveFromGettingList`, `Network_SetJoinSession`, `Network_SetSessionName`, `Network_StartGettingList`

</details>

<details><summary><strong>Leaderboard_* (4)</strong></summary>

`Leaderboard_GetRumpusChallengeName`, `Leaderboard_HasLeaderboard`, `Leaderboard_RequestRanks`, `Leaderboard_SetName`

</details>

<details><summary><strong>LanguageSelect_* (3)</strong></summary>

`LanguageSelect_GetList`, `LanguageSelect_RefreshText`, `LanguageSelect_SetLanguageFromIndex`

</details>

<details><summary><strong>Sprite_* (2)</strong></summary>

`Sprite_SetColor`, `Sprite_SetText`

</details>

<details><summary><strong>Sfx_* (2)</strong></summary>

`Sfx_Play2D`, `Sfx_PlayDialog`

</details>

<details><summary><strong>PlayerInExclusiveMission_* (2)</strong></summary>

`PlayerInExclusiveMission_Abortable`, `PlayerInExclusiveMission_NonAbortable`

</details>

<details><summary><strong>PersistentData_* (2)</strong></summary>

`PersistentData_GetGlobal`, `PersistentData_SetGlobal`

</details>

<details><summary><strong>Lock_* (2)</strong></summary>

`Lock_IsLocked`, `Lock_SetAvailable`

</details>

<details><summary><strong>Game_* (2)</strong></summary>

`Game_AcceptGameInvite`, `Game_DeclineGameInvite`

</details>

<details><summary><strong>Friend_* (2)</strong></summary>

`Friend_AcceptFriend`, `Friend_DeclineFriend`

</details>

<details><summary><strong>Tool_* (1)</strong></summary>

`Tool_FindToolInToyTree`

</details>

<details><summary><strong>StorageSystem_* (1)</strong></summary>

`StorageSystem_IsEnabled`

</details>

<details><summary><strong>Profile_* (1)</strong></summary>

`Profile_MarkerSet`

</details>

<details><summary><strong>Player_* (1)</strong></summary>

`Player_IsCharacterValid`

</details>

<details><summary><strong>Door_* (1)</strong></summary>

`Door_SlotSelected`

</details>

<details><summary><strong>Display_* (1)</strong></summary>

`Display_GetViewportIDFromPlayerID`

</details>

<details><summary><strong>DLC_* (1)</strong></summary>

`DLC_DownloadItem`

</details>

<details><summary><strong>Collection_* (1)</strong></summary>

`Collection_GetPackData`

</details>

<details><summary><strong>Building_* (1)</strong></summary>

`Building_ReleaseRumpusObjectLock`

</details>

## Known gaps

**No death or kill event.** Nothing in the engine notifies Lua that an entity died. `Game.onDeath(playerId, fn)` (`20_hooks.lua`) polls `Game.IsCharacterDead` every tick and fires once on the alive → dead edge.

Leads: `=Crabe.dump.find("kill")`, `("death")`, `("defeat")`, `("damage")`, then cross-reference with a real call site in the decompiled scripts.

## Convention

A native only enters the confirmed tables after being **called successfully in-game**. Everything else stays in `crabe_dump.txt`: the dump says what exists, not what works nor with which arguments — Lua 5.1 gives no signature for C functions.
