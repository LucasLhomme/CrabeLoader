/*
** CrabeLoader
** File description:
** SteamworksService implementation
*/

#include "infrastructure/multiplayer/SteamworksService.hpp"
#include "shared/logger.hpp"

#include <cstring>
#include <windows.h>

namespace {
    // Windows SEH-isolated invocation helpers.
    // Pure C-style (no C++ non-trivial unwinding in scope) to comply with MSVC C2712.
    // Guarantees zero crashes even if Steam state, pipe, or vtable is in an unexpected state.

    static const char* sehGetPersonaName(void* pFriends, void* fnPtr) {
        __try {
            using Fn = const char* (__thiscall*)(void*);
            return reinterpret_cast<Fn>(fnPtr)(pFriends);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    static uint64_t sehGetLocalSteamId(void* pUser, void* fnPtr) {
        __try {
            uint64_t steamId = 0;
            using Fn = void (__thiscall*)(void*, uint64_t*);
            reinterpret_cast<Fn>(fnPtr)(pUser, &steamId);
            return steamId;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    static uint64_t sehCreateLobby(void* pMatchmaking, void* fnPtr, int lobbyType, int maxMembers) {
        __try {
            using Fn = uint64_t (__thiscall*)(void*, int, int);
            return reinterpret_cast<Fn>(fnPtr)(pMatchmaking, lobbyType, maxMembers);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    static uint64_t sehJoinLobby(void* pMatchmaking, void* fnPtr, uint64_t lobbyId) {
        __try {
            using Fn = uint64_t (__thiscall*)(void*, uint64_t);
            return reinterpret_cast<Fn>(fnPtr)(pMatchmaking, lobbyId);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    static void sehLeaveLobby(void* pMatchmaking, void* fnPtr, uint64_t lobbyId) {
        __try {
            using Fn = void (__thiscall*)(void*, uint64_t);
            reinterpret_cast<Fn>(fnPtr)(pMatchmaking, lobbyId);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    static bool sehSetLobbyData(void* pMatchmaking, void* fnPtr, uint64_t lobbyId, const char* k, const char* v) {
        __try {
            using Fn = bool (__thiscall*)(void*, uint64_t, const char*, const char*);
            return reinterpret_cast<Fn>(fnPtr)(pMatchmaking, lobbyId, k, v);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static const char* sehGetLobbyData(void* pMatchmaking, void* fnPtr, uint64_t lobbyId, const char* k) {
        __try {
            using Fn = const char* (__thiscall*)(void*, uint64_t, const char*);
            return reinterpret_cast<Fn>(fnPtr)(pMatchmaking, lobbyId, k);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    static int sehGetFriendCount(void* pFriends, void* fnPtr, int flags) {
        __try {
            using Fn = int (__thiscall*)(void*, int);
            return reinterpret_cast<Fn>(fnPtr)(pFriends, flags);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    static uint64_t sehGetFriendByIndex(void* pFriends, void* fnPtr, int index, int flags) {
        __try {
            uint64_t friendId = 0;
            // On Win32 x86 MSVC, CSteamID struct return requires hidden return pointer
            using Fn = void (__thiscall*)(void*, uint64_t*, int, int);
            reinterpret_cast<Fn>(fnPtr)(pFriends, &friendId, index, flags);
            return friendId;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    static const char* sehGetFriendPersonaName(void* pFriends, void* fnPtr, uint64_t friendId) {
        __try {
            using Fn = const char* (__thiscall*)(void*, uint64_t);
            return reinterpret_cast<Fn>(fnPtr)(pFriends, friendId);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    static void sehInviteOverlay(void* pFriends, void* fnPtr, uint64_t lobbyId) {
        __try {
            using Fn = void (__thiscall*)(void*, uint64_t);
            reinterpret_cast<Fn>(fnPtr)(pFriends, lobbyId);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    static void sehGeneralOverlay(void* pFriends, void* fnPtr, const char* dialog) {
        __try {
            using Fn = void (__thiscall*)(void*, const char*);
            reinterpret_cast<Fn>(fnPtr)(pFriends, dialog);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }
} // namespace

namespace Crabe::Multiplayer {

    SteamworksService::SteamworksService() = default;

    SteamworksService::~SteamworksService() {
        shutdown();
    }

    bool SteamworksService::bindSteamFunctions() {
        if (_steamApiModule) {
            return true;
        }

        // The game binary already delay-loads steam_api.dll or it is in the game root directory
        _steamApiModule = GetModuleHandleW(L"steam_api.dll");
        if (!_steamApiModule) {
            _steamApiModule = LoadLibraryW(L"steam_api.dll");
        }

        if (!_steamApiModule) {
            Logger::getInstance().warning("SteamworksService: steam_api.dll could not be loaded or located.");
            return false;
        }

        _fnSteamAPI_Init = reinterpret_cast<FnSteamAPI_Init>(GetProcAddress(_steamApiModule, "SteamAPI_Init"));
        _fnSteamAPI_RunCallbacks = reinterpret_cast<FnSteamAPI_RunCallbacks>(GetProcAddress(_steamApiModule, "SteamAPI_RunCallbacks"));
        _fnSteamFriends = reinterpret_cast<FnSteamFriends>(GetProcAddress(_steamApiModule, "SteamFriends"));
        _fnSteamMatchmaking = reinterpret_cast<FnSteamMatchmaking>(GetProcAddress(_steamApiModule, "SteamMatchmaking"));
        _fnSteamUser = reinterpret_cast<FnSteamUser>(GetProcAddress(_steamApiModule, "SteamUser"));
        _fnSteamUtils = reinterpret_cast<FnSteamUtils>(GetProcAddress(_steamApiModule, "SteamUtils"));

        return (_fnSteamAPI_Init != nullptr && _fnSteamFriends != nullptr);
    }

    bool SteamworksService::initialize() {
        if (_initialized.load()) {
            return _available.load();
        }

        std::lock_guard<std::mutex> lock(_mutex);
        if (!bindSteamFunctions()) {
            _initialized = true;
            _available = false;
            return false;
        }

        // SteamAPI_Init might have already been called by the game engine. Calling it again is safe and returns true.
        bool initOk = false;
        if (_fnSteamAPI_Init) {
            initOk = _fnSteamAPI_Init();
        }

        if (_fnSteamFriends) {
            _pSteamFriends = _fnSteamFriends();
        }
        if (_fnSteamMatchmaking) {
            _pSteamMatchmaking = _fnSteamMatchmaking();
        }
        if (_fnSteamUser) {
            _pSteamUser = _fnSteamUser();
        }
        if (_fnSteamUtils) {
            _pSteamUtils = _fnSteamUtils();
        }

        _available = (_pSteamFriends != nullptr);
        _initialized = true;

        if (_available) {
            std::string name = getPersonaName();
            Logger::getInstance().info("SteamworksService: initialized successfully. Logged in as '{}' (SteamID: {})",
                                       name, getLocalSteamId());
        } else {
            Logger::getInstance().warning("SteamworksService: initialized but Steam interfaces not yet resolved (is Steam running?).");
        }

        return _available.load();
    }

    void SteamworksService::shutdown() {
        if (!_initialized.load()) {
            return;
        }

        leaveLobby();

        std::lock_guard<std::mutex> lock(_mutex);
        _pSteamFriends = nullptr;
        _pSteamMatchmaking = nullptr;
        _pSteamUser = nullptr;
        _pSteamUtils = nullptr;
        _available = false;
        _initialized = false;
    }

    void SteamworksService::runCallbacks() {
        if (_available.load() && _fnSteamAPI_RunCallbacks) {
            _fnSteamAPI_RunCallbacks();
        }
    }

    bool SteamworksService::isAvailable() const noexcept {
        return _available.load();
    }

    uint64_t SteamworksService::getLocalSteamId() const noexcept {
        if (!_pSteamUser) {
            return 0;
        }

        void** vtable = *reinterpret_cast<void***>(_pSteamUser);
        if (!vtable || !vtable[2]) {
            return 0;
        }

        return sehGetLocalSteamId(_pSteamUser, vtable[2]);
    }

    std::string SteamworksService::getPersonaName() const {
        if (!_pSteamFriends) {
            return "Player";
        }

        void** vtable = *reinterpret_cast<void***>(_pSteamFriends);
        if (!vtable || !vtable[0]) {
            return "Player";
        }

        const char* name = sehGetPersonaName(_pSteamFriends, vtable[0]);
        return (name && name[0] != '\0') ? std::string(name) : "Player";
    }

    bool SteamworksService::createLobby(bool friendsOnly, int maxMembers) {
        if (!_pSteamMatchmaking) {
            Logger::getInstance().warning("SteamworksService::createLobby: ISteamMatchmaking not available.");
            return false;
        }

        // ISteamMatchmaking009 vtable:
        // index 12: virtual SteamAPICall_t CreateLobby( ELobbyType eLobbyType, int cMaxMembers ) = 0;
        // eLobbyType: 0 = Private, 1 = FriendsOnly, 2 = Public, 3 = Invisible
        int lobbyType = friendsOnly ? 1 : 2;
        void** vtable = *reinterpret_cast<void***>(_pSteamMatchmaking);
        if (!vtable || !vtable[12]) {
            return false;
        }

        uint64_t callHandle = sehCreateLobby(_pSteamMatchmaking, vtable[12], lobbyType, maxMembers);
        if (callHandle == 0) {
            Logger::getInstance().warning("SteamworksService::createLobby: CreateLobby API call returned 0.");
            return false;
        }

        std::lock_guard<std::mutex> lock(_mutex);
        _lobbyStatus.inLobby = true;
        _lobbyStatus.isHost = true;
        _lobbyStatus.memberCount = 1;
        _lobbyStatus.memberLimit = maxMembers;
        _lobbyStatus.hostSteamId = getLocalSteamId();

        Logger::getInstance().info("SteamworksService: CreateLobby dispatched (call handle: {})", callHandle);
        return true;
    }

    void SteamworksService::leaveLobby() {
        if (!_pSteamMatchmaking || !_lobbyStatus.inLobby) {
            return;
        }

        // ISteamMatchmaking009 vtable:
        // index 14: virtual void LeaveLobby( CSteamID steamIDLobby ) = 0;
        if (_lobbyStatus.lobbyId != 0) {
            void** vtable = *reinterpret_cast<void***>(_pSteamMatchmaking);
            if (vtable && vtable[14]) {
                sehLeaveLobby(_pSteamMatchmaking, vtable[14], _lobbyStatus.lobbyId);
            }
        }

        std::lock_guard<std::mutex> lock(_mutex);
        _lobbyStatus = {};
        Logger::getInstance().info("SteamworksService: left lobby.");
    }

    bool SteamworksService::joinLobby(uint64_t lobbyId) {
        if (!_pSteamMatchmaking || lobbyId == 0) {
            return false;
        }

        // ISteamMatchmaking009 vtable:
        // index 13: virtual SteamAPICall_t JoinLobby( CSteamID steamIDLobby ) = 0;
        void** vtable = *reinterpret_cast<void***>(_pSteamMatchmaking);
        if (!vtable || !vtable[13]) {
            return false;
        }

        uint64_t callHandle = sehJoinLobby(_pSteamMatchmaking, vtable[13], lobbyId);
        if (callHandle == 0) {
            return false;
        }

        std::lock_guard<std::mutex> lock(_mutex);
        _lobbyStatus.inLobby = true;
        _lobbyStatus.isHost = false;
        _lobbyStatus.lobbyId = lobbyId;

        Logger::getInstance().info("SteamworksService: JoinLobby dispatched for lobby {} (call handle: {})", lobbyId, callHandle);
        return true;
    }

    bool SteamworksService::setLobbyData(std::string_view key, std::string_view value) {
        if (!_pSteamMatchmaking || !_lobbyStatus.inLobby || _lobbyStatus.lobbyId == 0) {
            return false;
        }

        // ISteamMatchmaking009 vtable:
        // index 18: virtual bool SetLobbyData( CSteamID steamIDLobby, const char *pchKey, const char *pchValue ) = 0;
        void** vtable = *reinterpret_cast<void***>(_pSteamMatchmaking);
        if (!vtable || !vtable[18]) {
            return false;
        }

        std::string k(key);
        std::string v(value);
        return sehSetLobbyData(_pSteamMatchmaking, vtable[18], _lobbyStatus.lobbyId, k.c_str(), v.c_str());
    }

    std::string SteamworksService::getLobbyData(std::string_view key) const {
        if (!_pSteamMatchmaking || !_lobbyStatus.inLobby || _lobbyStatus.lobbyId == 0) {
            return "";
        }

        // ISteamMatchmaking009 vtable:
        // index 17: virtual const char *GetLobbyData( CSteamID steamIDLobby, const char *pchKey ) = 0;
        void** vtable = *reinterpret_cast<void***>(_pSteamMatchmaking);
        if (!vtable || !vtable[17]) {
            return "";
        }

        std::string k(key);
        const char* val = sehGetLobbyData(_pSteamMatchmaking, vtable[17], _lobbyStatus.lobbyId, k.c_str());
        return val ? std::string(val) : "";
    }

    bool SteamworksService::openInviteOverlay() {
        if (!_pSteamFriends) {
            return false;
        }

        // ISteamFriends014 vtable:
        // index 27: virtual void ActivateGameOverlayInviteDialog( CSteamID steamIDLobby ) = 0;
        // index 22: virtual void ActivateGameOverlay( const char *pchDialog ) = 0;
        void** vtable = *reinterpret_cast<void***>(_pSteamFriends);
        if (!vtable) {
            return false;
        }

        if (_lobbyStatus.inLobby && _lobbyStatus.lobbyId != 0 && vtable[27]) {
            sehInviteOverlay(_pSteamFriends, vtable[27], _lobbyStatus.lobbyId);
            Logger::getInstance().info("SteamworksService: activated Steam lobby invite dialog for lobby {}", _lobbyStatus.lobbyId);
            return true;
        }

        // Fallback: open general friends overlay
        if (vtable[22]) {
            sehGeneralOverlay(_pSteamFriends, vtable[22], "friends");
            Logger::getInstance().info("SteamworksService: activated Steam friends overlay.");
            return true;
        }
        return false;
    }

    int SteamworksService::getFriendCount() const noexcept {
        if (!_pSteamFriends) {
            return 0;
        }

        void** vtable = *reinterpret_cast<void***>(_pSteamFriends);
        if (!vtable || !vtable[3]) {
            return 0;
        }

        int count = sehGetFriendCount(_pSteamFriends, vtable[3], 0x04);
        return count > 0 ? count : 0;
    }

    std::vector<SteamFriendInfo> SteamworksService::getFriends() const {
        std::vector<SteamFriendInfo> friendsList;
        if (!_pSteamFriends) {
            return friendsList;
        }

        // ISteamFriends014 vtable:
        // index 3: virtual int GetFriendCount( int iFriendFlags ) = 0; (0x04 = k_EFriendFlagImmediate)
        // index 4: virtual CSteamID GetFriendByIndex( int iFriend, int iFriendFlags ) = 0;
        // index 7: virtual const char *GetFriendPersonaName( CSteamID steamIDFriend ) = 0;
        void** vtable = *reinterpret_cast<void***>(_pSteamFriends);
        if (!vtable || !vtable[3] || !vtable[4] || !vtable[7]) {
            return friendsList;
        }

        int count = sehGetFriendCount(_pSteamFriends, vtable[3], 0x04);
        if (count < 0) count = 0;
        if (count > 50) count = 50; // safety clamp

        for (int i = 0; i < count; ++i) {
            uint64_t friendId = sehGetFriendByIndex(_pSteamFriends, vtable[4], i, 0x04);
            if (friendId == 0) continue;

            const char* fname = sehGetFriendPersonaName(_pSteamFriends, vtable[7], friendId);
            SteamFriendInfo info;
            info.steamId = friendId;
            info.name = fname ? fname : "Friend";
            info.isPlayingThisGame = false;
            friendsList.push_back(std::move(info));
        }

        return friendsList;
    }

    SteamLobbyStatus SteamworksService::getLobbyStatus() const noexcept {
        std::lock_guard<std::mutex> lock(_mutex);
        return _lobbyStatus;
    }

    void SteamworksService::setOnLobbyJoinedHandler(OnLobbyJoinedCallback handler) {
        std::lock_guard<std::mutex> lock(_mutex);
        _onLobbyJoinedHandler = std::move(handler);
    }

} // namespace Crabe::Multiplayer
