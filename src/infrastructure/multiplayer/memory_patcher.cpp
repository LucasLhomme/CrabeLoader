/*
** CrabeLoader
** File description:
** MemoryPatcher implementation
*/

#include "infrastructure/multiplayer/memory_patcher.hpp"

#include <cstring>
#include <format>
#include <string_view>
#include <windows.h>

#include "domain/game_profile.hpp"
#include "infrastructure/code_cave.hpp"
#include "infrastructure/memory.hpp"
#include "shared/logger.hpp"

namespace crabe::multiplayer::infrastructure {

    namespace {
        // Every address these patches land on now comes from the GameProfile
        // for the build being run -- see include/domain/game_profile.hpp. What
        // stays here is what is true of the patch rather than of the build: the
        // bytes written, and the byte pattern that identifies the site on any
        // build.
        //
        // Patch names, which are also the profile's keys.
        constexpr std::string_view kIsSignedIn = "IsSignedInToDisneyId";
        constexpr std::string_view kIsOnline = "IsOnline";
        constexpr std::string_view kIsOnlineContentAllowed = "IsOnlineContentAllowed";
        constexpr std::string_view kVerifyResponseSig = "VerifyResponseSignature";
        constexpr std::string_view kSteamRestartApp = "SteamAPI_RestartAppIfNecessary";
        constexpr std::string_view kHostingSessionGate = "HostingSessionGate";
        constexpr std::string_view kPlayerListAllNegotiated = "PlayerListAllNegotiated";
        constexpr std::string_view kOctaneAppMutex = "OctaneAppMutex";

        // Byte patches
        constexpr uint8_t kMovAl1[2] = { 0xB0, 0x01 };
        constexpr uint8_t kXorAlAl[2] = { 0x32, 0xC0 };
        constexpr uint8_t kMovAlBl[2] = { 0x8A, 0xC3 };
        constexpr uint8_t kVerifySigStub[6] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 }; // mov eax, 1; ret (safe 0-param bypass)

        // GetFriendsList error skip
        constexpr const char* kFriendsErrorPattern = "83 F8 1A 74 06";
        constexpr uint8_t kFriendsJeToJmp[2] = { 0xEB, 0x06 };

        // SteamAPI_RestartAppIfNecessary bypass (allow running multiple instances without restarting)
        constexpr uint8_t kSteamRestartAppNop[6] = { 0x31, 0xC0, 0x90, 0x90, 0x90, 0x90 }; // xor eax, eax; nop...

        // HostingSessionGate (Quazal OpenSession Instant Bypass)
        // cmp [esi+0Ch], edx ; je +0xEC -> mov [esi+0Ch], edx ; nop ; jmp +0xEC
        constexpr const char* kHostingSessionGatePattern = "39 56 0C 0F 84 EC 00 00 00";
        constexpr uint8_t kHostingSessionGateOriginal[9] = { 0x39, 0x56, 0x0C, 0x0F, 0x84, 0xEC, 0x00, 0x00, 0x00 };
        constexpr uint8_t kHostingSessionGatePatched[9]  = { 0x89, 0x56, 0x0C, 0x90, 0xE9, 0xEC, 0x00, 0x00, 0x00 };

        // PlayerListAllNegotiated (GameSpy NATNEG Bypass)
        // mov eax, [0x0225ADF8] ; test eax, eax ; jz +0x13 -> mov al, 1 ; ret ; 6x nop
        constexpr const char* kPlayerListAllNegotiatedPattern = "A1 F8 AD 25 02 85 C0 74 13";
        constexpr uint8_t kPlayerListAllNegotiatedOriginal[9] = { 0xA1, 0xF8, 0xAD, 0x25, 0x02, 0x85, 0xC0, 0x74, 0x13 };
        constexpr uint8_t kPlayerListAllNegotiatedPatched[9]  = { 0xB0, 0x01, 0xC3, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

        // OctaneApp Single-Instance Mutex Bypass (allow multi-instance local loopback on 1 PC)
        // cmp eax, 0xB7 ; je +0x18 ; call edi ; cmp eax, 5 ; je +0x11 -> NOP out both je jumps
        constexpr const char* kOctaneAppMutexPattern = "3D B7 00 00 00 74 18 FF D7 83 F8 05 74 11";
        constexpr uint8_t kOctaneAppMutexOriginal[14] = { 0x3D, 0xB7, 0x00, 0x00, 0x00, 0x74, 0x18, 0xFF, 0xD7, 0x83, 0xF8, 0x05, 0x74, 0x11 };
        constexpr uint8_t kOctaneAppMutexPatched[14]  = { 0x3D, 0xB7, 0x00, 0x00, 0x00, 0x90, 0x90, 0xFF, 0xD7, 0x83, 0xF8, 0x05, 0x90, 0x90 };

        // Where a named patch lands on this build: base + the profile's RVA for
        // it, plus how far into that function the rewritten bytes sit. Zero when
        // the profile does not state the site, which applyRecord() treats as
        // "skip this patch".
        uintptr_t siteAddress(const crabe::domain::GameProfile& profile, uintptr_t base,
                              std::string_view name)
        {
            const crabe::domain::PatchSite* site = profile.patchSite(name);
            if (!site || site->rva == crabe::domain::kUnmeasured) {
                crabe::shared::Logger::getInstance().warning(
                    "MemoryPatcher: profile '{}' states no address for '{}'; skipping that patch.",
                    profile.id, name);
                return 0;
            }
            return base + site->rva + site->offset;
        }

        // Where a patch that also has a byte pattern lands.
        //
        // The pattern is the evidence: it describes the instructions being
        // rewritten and holds on any build that still contains them. The RVA is
        // a confirmation and is preferred only when the bytes there are the ones
        // the patch expects -- or the ones it has already written, so a second
        // pass over an applied patch does not go looking elsewhere. This is the
        // order cases 7 to 9 have always used; it is kept, not reinvented.
        uintptr_t confirmedSite(uintptr_t rvaAddress, const char* pattern,
                                const uint8_t* original, size_t length, const uint8_t* patched)
        {
            const bool atRva = rvaAddress
                && crabe::memory::isReadable(rvaAddress, length)
                && (std::memcmp(reinterpret_cast<const void*>(rvaAddress), original, length) == 0
                 || std::memcmp(reinterpret_cast<const void*>(rvaAddress), patched, length) == 0);

            if (atRva) {
                return rvaAddress;
            }

            uintptr_t scanned = crabe::memory::patternScan(pattern);
            return scanned ? scanned : rvaAddress;
        }
    } // namespace

    MemoryPatcher::MemoryPatcher() = default;

    MemoryPatcher::~MemoryPatcher() {
        restorePatches();
    }

    bool MemoryPatcher::applyRecord(PatchRecord& record) {
        if (!record.address || record.patchedBytes.empty()) {
            return false;
        }

        if (record.active) {
            return true;
        }

        const size_t len = record.patchedBytes.size();
        if (!crabe::memory::isReadable(record.address, len)) {
            crabe::shared::Logger::getInstance().warning("MemoryPatcher: memory not readable at 0x{:X} for {}",
                                          record.address, record.name);
            return false;
        }

        // Save original bytes if not already saved
        if (record.originalBytes.empty()) {
            record.originalBytes.resize(len);
            std::memcpy(record.originalBytes.data(), reinterpret_cast<const void*>(record.address), len);
        }

        if (!crabe::infrastructure::CodeCave::patchBytes(record.address, record.patchedBytes.data(), len)) {
            crabe::shared::Logger::getInstance().error("MemoryPatcher: failed to patch bytes at 0x{:X} for {}",
                                        record.address, record.name);
            return false;
        }

        record.active = true;
        crabe::shared::Logger::getInstance().info("MemoryPatcher: successfully applied patch '{}' at 0x{:X}",
                                   record.name, record.address);
        return true;
    }

    void MemoryPatcher::restoreRecord(PatchRecord& record) {
        if (!record.active || record.originalBytes.empty() || !record.address) {
            return;
        }

        if (crabe::infrastructure::CodeCave::patchBytes(record.address, record.originalBytes.data(), record.originalBytes.size())) {
            record.active = false;
            crabe::shared::Logger::getInstance().info("MemoryPatcher: restored patch '{}' at 0x{:X}",
                                       record.name, record.address);
        }
    }

    std::expected<void, std::string> MemoryPatcher::applyPatches() {
        std::lock_guard<std::mutex> lock(_mutex);

        auto base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
        if (!base) {
            return std::unexpected("Failed to get main module handle");
        }

        // No profile, no patches. Five of the nine sites below are known only by
        // RVA -- no byte pattern was ever measured for them -- so on a build
        // nobody has identified they would be written blind, into whatever the
        // linker happened to put there. That is the failure T10 exists to stop,
        // and it is also why degraded mode disables multiplayer rather than
        // trying: Lua mods lose nothing by being unsure of an address, a byte
        // patch corrupts code.
        const crabe::domain::GameProfile* profile = crabe::domain::activeProfile();
        if (!profile) {
            const crabe::domain::PeIdentity identity = crabe::domain::runningGameIdentity();
            return std::unexpected(std::format(
                "no game profile matches this build (TimeDateStamp 0x{:08X}, SizeOfImage 0x{:X}); "
                "multiplayer patches need addresses measured against a known executable, so none "
                "were applied",
                identity.timeDateStamp, identity.sizeOfImage));
        }

        if (_records.empty()) {
            // 1. IsSignedInToDisneyId
            {
                PatchRecord rec;
                rec.address = siteAddress(*profile, base, kIsSignedIn);
                rec.patchedBytes.assign(kMovAl1, kMovAl1 + sizeof(kMovAl1));
                rec.name = std::string(kIsSignedIn);
                _records.push_back(std::move(rec));
            }

            // 2. IsOnline
            {
                PatchRecord rec;
                rec.address = siteAddress(*profile, base, kIsOnline);
                rec.patchedBytes.assign(kMovAl1, kMovAl1 + sizeof(kMovAl1));
                rec.name = std::string(kIsOnline);
                _records.push_back(std::move(rec));
            }

            // 3. IsOnlineContentAllowed
            {
                PatchRecord rec;
                rec.address = siteAddress(*profile, base, kIsOnlineContentAllowed);
                rec.patchedBytes.assign(kMovAl1, kMovAl1 + sizeof(kMovAl1));
                rec.name = std::string(kIsOnlineContentAllowed);
                _records.push_back(std::move(rec));
            }

            // 4. VerifyResponseSignature
            {
                PatchRecord rec;
                rec.address = siteAddress(*profile, base, kVerifyResponseSig);
                rec.patchedBytes.assign(kVerifySigStub, kVerifySigStub + sizeof(kVerifySigStub));
                rec.name = "VerifyResponseSignature (UGC RSA Bypass)";
                _records.push_back(std::move(rec));
            }

            // 5. GetFriendsList Session Error Bypass
            {
                uintptr_t site = crabe::memory::patternScan(kFriendsErrorPattern);
                if (site) {
                    PatchRecord rec;
                    rec.address = site + 3; // jump to je opcode
                    rec.patchedBytes.assign(kFriendsJeToJmp, kFriendsJeToJmp + sizeof(kFriendsJeToJmp));
                    rec.name = "GetFriendsList (Session Error Bypass)";
                    _records.push_back(std::move(rec));
                } else {
                    crabe::shared::Logger::getInstance().warning("MemoryPatcher: GetFriendsList pattern not found (may scan on retry)");
                }
            }

            // 6. SteamAPI_RestartAppIfNecessary Multi-Instance Bypass
            {
                PatchRecord rec;
                rec.address = siteAddress(*profile, base, kSteamRestartApp);
                rec.patchedBytes.assign(kSteamRestartAppNop, kSteamRestartAppNop + sizeof(kSteamRestartAppNop));
                rec.name = "SteamAPI_RestartAppIfNecessary (Multi-Instance Bypass)";
                _records.push_back(std::move(rec));
            }

            // 7. HostingSessionGate (Quazal OpenSession Instant Bypass)
            {
                PatchRecord rec;
                rec.address = confirmedSite(siteAddress(*profile, base, kHostingSessionGate),
                                            kHostingSessionGatePattern,
                                            kHostingSessionGateOriginal,
                                            sizeof(kHostingSessionGateOriginal),
                                            kHostingSessionGatePatched);
                rec.originalBytes.assign(kHostingSessionGateOriginal, kHostingSessionGateOriginal + sizeof(kHostingSessionGateOriginal));
                rec.patchedBytes.assign(kHostingSessionGatePatched, kHostingSessionGatePatched + sizeof(kHostingSessionGatePatched));
                rec.name = "HostingSessionGate (Quazal OpenSession Instant Bypass)";
                _records.push_back(std::move(rec));
            }

            // 8. PlayerListAllNegotiated (GameSpy NATNEG Bypass)
            {
                PatchRecord rec;
                rec.address = confirmedSite(siteAddress(*profile, base, kPlayerListAllNegotiated),
                                            kPlayerListAllNegotiatedPattern,
                                            kPlayerListAllNegotiatedOriginal,
                                            sizeof(kPlayerListAllNegotiatedOriginal),
                                            kPlayerListAllNegotiatedPatched);
                rec.originalBytes.assign(kPlayerListAllNegotiatedOriginal, kPlayerListAllNegotiatedOriginal + sizeof(kPlayerListAllNegotiatedOriginal));
                rec.patchedBytes.assign(kPlayerListAllNegotiatedPatched, kPlayerListAllNegotiatedPatched + sizeof(kPlayerListAllNegotiatedPatched));
                rec.name = "PlayerListAllNegotiated (GameSpy NATNEG Bypass)";
                _records.push_back(std::move(rec));
            }

            // 9. OctaneApp (Multi-Instance Mutex Bypass)
            {
                PatchRecord rec;
                rec.address = confirmedSite(siteAddress(*profile, base, kOctaneAppMutex),
                                            kOctaneAppMutexPattern,
                                            kOctaneAppMutexOriginal,
                                            sizeof(kOctaneAppMutexOriginal),
                                            kOctaneAppMutexPatched);
                rec.originalBytes.assign(kOctaneAppMutexOriginal, kOctaneAppMutexOriginal + sizeof(kOctaneAppMutexOriginal));
                rec.patchedBytes.assign(kOctaneAppMutexPatched, kOctaneAppMutexPatched + sizeof(kOctaneAppMutexPatched));
                rec.name = "OctaneApp (Multi-Instance Mutex Bypass)";
                _records.push_back(std::move(rec));
            }
        }

        unsigned applied = 0;
        for (auto& rec : _records) {
            if (applyRecord(rec)) {
                applied++;
            }
        }

        _patchedCount = applied;
        _active = (applied > 0);

        if (applied == 0) {
            return std::unexpected("No memory patches could be applied");
        }

        crabe::shared::Logger::getInstance().info("MemoryPatcher: {}/{} patches active on profile '{}'",
                                   applied, _records.size(), profile->id);
        return {};
    }

    void MemoryPatcher::restorePatches() {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto& rec : _records) {
            restoreRecord(rec);
        }
        _active = false;
        _patchedCount = 0;
    }

    bool MemoryPatcher::arePatchesActive() const noexcept {
        return _active.load();
    }

    unsigned MemoryPatcher::getPatchedCount() const noexcept {
        return _patchedCount.load();
    }

} // namespace crabe::multiplayer::infrastructure
