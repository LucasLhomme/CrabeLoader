/*
** CrabeLoader
** File description:
** MemoryPatcher implementation
*/

#include "infrastructure/multiplayer/MemoryPatcher.hpp"

#include <cstring>
#include <windows.h>

#include "infrastructure/codecave.hpp"
#include "infrastructure/memory.hpp"
#include "shared/logger.hpp"

namespace Multiplayer::Infrastructure {

    namespace {
        constexpr uintptr_t kIsSignedInRva = 0x00F62550;
        constexpr uintptr_t kIsSignedInFailOffset = 0x4A;

        constexpr uintptr_t kIsOnlineRva = 0x00F5EE90;
        constexpr uintptr_t kIsOnlineFailOffset = 0x249;

        constexpr uintptr_t kIsOnlineContentAllowedRva = 0x00F630F0;
        constexpr uintptr_t kIsOnlineContentAllowedOffset = 0x1DD;

        constexpr uintptr_t kVerifyResponseSigRva = 0x00F35790;

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
        constexpr uintptr_t kHostingSessionGateRva = 0x0074E97A;
        constexpr const char* kHostingSessionGatePattern = "39 56 0C 0F 84 EC 00 00 00";
        constexpr uint8_t kHostingSessionGateOriginal[9] = { 0x39, 0x56, 0x0C, 0x0F, 0x84, 0xEC, 0x00, 0x00, 0x00 };
        constexpr uint8_t kHostingSessionGatePatched[9]  = { 0x89, 0x56, 0x0C, 0x90, 0xE9, 0xEC, 0x00, 0x00, 0x00 };

        // PlayerListAllNegotiated (GameSpy NATNEG Bypass)
        // mov eax, [0x0225ADF8] ; test eax, eax ; jz +0x13 -> mov al, 1 ; ret ; 6x nop
        constexpr uintptr_t kPlayerListAllNegotiatedRva = 0x00F29850;
        constexpr const char* kPlayerListAllNegotiatedPattern = "A1 F8 AD 25 02 85 C0 74 13";
        constexpr uint8_t kPlayerListAllNegotiatedOriginal[9] = { 0xA1, 0xF8, 0xAD, 0x25, 0x02, 0x85, 0xC0, 0x74, 0x13 };
        constexpr uint8_t kPlayerListAllNegotiatedPatched[9]  = { 0xB0, 0x01, 0xC3, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

        // OctaneApp Single-Instance Mutex Bypass (allow multi-instance local loopback on 1 PC)
        // cmp eax, 0xB7 ; je +0x18 ; call edi ; cmp eax, 5 ; je +0x11 -> NOP out both je jumps
        constexpr uintptr_t kOctaneAppMutexRva = 0x0003F0DE;
        constexpr const char* kOctaneAppMutexPattern = "3D B7 00 00 00 74 18 FF D7 83 F8 05 74 11";
        constexpr uint8_t kOctaneAppMutexOriginal[14] = { 0x3D, 0xB7, 0x00, 0x00, 0x00, 0x74, 0x18, 0xFF, 0xD7, 0x83, 0xF8, 0x05, 0x74, 0x11 };
        constexpr uint8_t kOctaneAppMutexPatched[14]  = { 0x3D, 0xB7, 0x00, 0x00, 0x00, 0x90, 0x90, 0xFF, 0xD7, 0x83, 0xF8, 0x05, 0x90, 0x90 };
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
        if (!Memory::isReadable(record.address, len)) {
            Logger::getInstance().warning("MemoryPatcher: memory not readable at 0x{:X} for {}",
                                          record.address, record.name);
            return false;
        }

        // Save original bytes if not already saved
        if (record.originalBytes.empty()) {
            record.originalBytes.resize(len);
            std::memcpy(record.originalBytes.data(), reinterpret_cast<const void*>(record.address), len);
        }

        if (!CodeCave::patchBytes(record.address, record.patchedBytes.data(), len)) {
            Logger::getInstance().error("MemoryPatcher: failed to patch bytes at 0x{:X} for {}",
                                        record.address, record.name);
            return false;
        }

        record.active = true;
        Logger::getInstance().info("MemoryPatcher: successfully applied patch '{}' at 0x{:X}",
                                   record.name, record.address);
        return true;
    }

    void MemoryPatcher::restoreRecord(PatchRecord& record) {
        if (!record.active || record.originalBytes.empty() || !record.address) {
            return;
        }

        if (CodeCave::patchBytes(record.address, record.originalBytes.data(), record.originalBytes.size())) {
            record.active = false;
            Logger::getInstance().info("MemoryPatcher: restored patch '{}' at 0x{:X}",
                                       record.name, record.address);
        }
    }

    std::expected<void, std::string> MemoryPatcher::applyPatches() {
        std::lock_guard<std::mutex> lock(_mutex);

        auto base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
        if (!base) {
            return std::unexpected("Failed to get main module handle");
        }

        if (_records.empty()) {
            // 1. IsSignedInToDisneyId
            {
                PatchRecord rec;
                rec.address = base + kIsSignedInRva + kIsSignedInFailOffset;
                rec.patchedBytes.assign(kMovAl1, kMovAl1 + sizeof(kMovAl1));
                rec.name = "IsSignedInToDisneyId";
                _records.push_back(std::move(rec));
            }

            // 2. IsOnline
            {
                PatchRecord rec;
                rec.address = base + kIsOnlineRva + kIsOnlineFailOffset;
                rec.patchedBytes.assign(kMovAl1, kMovAl1 + sizeof(kMovAl1));
                rec.name = "IsOnline";
                _records.push_back(std::move(rec));
            }

            // 3. IsOnlineContentAllowed
            {
                PatchRecord rec;
                rec.address = base + kIsOnlineContentAllowedRva + kIsOnlineContentAllowedOffset;
                rec.patchedBytes.assign(kMovAl1, kMovAl1 + sizeof(kMovAl1));
                rec.name = "IsOnlineContentAllowed";
                _records.push_back(std::move(rec));
            }

            // 4. VerifyResponseSignature
            {
                PatchRecord rec;
                rec.address = base + kVerifyResponseSigRva;
                rec.patchedBytes.assign(kVerifySigStub, kVerifySigStub + sizeof(kVerifySigStub));
                rec.name = "VerifyResponseSignature (UGC RSA Bypass)";
                _records.push_back(std::move(rec));
            }

            // 5. GetFriendsList Session Error Bypass
            {
                uintptr_t site = Memory::patternScan(kFriendsErrorPattern);
                if (site) {
                    PatchRecord rec;
                    rec.address = site + 3; // jump to je opcode
                    rec.patchedBytes.assign(kFriendsJeToJmp, kFriendsJeToJmp + sizeof(kFriendsJeToJmp));
                    rec.name = "GetFriendsList (Session Error Bypass)";
                    _records.push_back(std::move(rec));
                } else {
                    Logger::getInstance().warning("MemoryPatcher: GetFriendsList pattern not found (may scan on retry)");
                }
            }

            // 6. SteamAPI_RestartAppIfNecessary Multi-Instance Bypass
            {
                constexpr uintptr_t kRestartAppCallRva = 0x00032C9E;
                PatchRecord rec;
                rec.address = base + kRestartAppCallRva;
                rec.patchedBytes.assign(kSteamRestartAppNop, kSteamRestartAppNop + sizeof(kSteamRestartAppNop));
                rec.name = "SteamAPI_RestartAppIfNecessary (Multi-Instance Bypass)";
                _records.push_back(std::move(rec));
            }

            // 7. HostingSessionGate (Quazal OpenSession Instant Bypass)
            {
                uintptr_t target = base + kHostingSessionGateRva;
                const bool atRva = Memory::isReadable(target, sizeof(kHostingSessionGateOriginal)) &&
                    (std::memcmp(reinterpret_cast<const void*>(target), kHostingSessionGateOriginal, sizeof(kHostingSessionGateOriginal)) == 0 ||
                     std::memcmp(reinterpret_cast<const void*>(target), kHostingSessionGatePatched, sizeof(kHostingSessionGatePatched)) == 0);

                if (!atRva) {
                    uintptr_t scanned = Memory::patternScan(kHostingSessionGatePattern);
                    if (scanned) {
                        target = scanned;
                    }
                }

                PatchRecord rec;
                rec.address = target;
                rec.originalBytes.assign(kHostingSessionGateOriginal, kHostingSessionGateOriginal + sizeof(kHostingSessionGateOriginal));
                rec.patchedBytes.assign(kHostingSessionGatePatched, kHostingSessionGatePatched + sizeof(kHostingSessionGatePatched));
                rec.name = "HostingSessionGate (Quazal OpenSession Instant Bypass)";
                _records.push_back(std::move(rec));
            }

            // 8. PlayerListAllNegotiated (GameSpy NATNEG Bypass)
            {
                uintptr_t target = base + kPlayerListAllNegotiatedRva;
                const bool atRva = Memory::isReadable(target, sizeof(kPlayerListAllNegotiatedOriginal)) &&
                    (std::memcmp(reinterpret_cast<const void*>(target), kPlayerListAllNegotiatedOriginal, sizeof(kPlayerListAllNegotiatedOriginal)) == 0 ||
                     std::memcmp(reinterpret_cast<const void*>(target), kPlayerListAllNegotiatedPatched, sizeof(kPlayerListAllNegotiatedPatched)) == 0);

                if (!atRva) {
                    uintptr_t scanned = Memory::patternScan(kPlayerListAllNegotiatedPattern);
                    if (scanned) {
                        target = scanned;
                    }
                }

                PatchRecord rec;
                rec.address = target;
                rec.originalBytes.assign(kPlayerListAllNegotiatedOriginal, kPlayerListAllNegotiatedOriginal + sizeof(kPlayerListAllNegotiatedOriginal));
                rec.patchedBytes.assign(kPlayerListAllNegotiatedPatched, kPlayerListAllNegotiatedPatched + sizeof(kPlayerListAllNegotiatedPatched));
                rec.name = "PlayerListAllNegotiated (GameSpy NATNEG Bypass)";
                _records.push_back(std::move(rec));
            }

            // 9. OctaneApp (Multi-Instance Mutex Bypass)
            {
                uintptr_t target = base + kOctaneAppMutexRva;
                const bool atRva = Memory::isReadable(target, sizeof(kOctaneAppMutexOriginal)) &&
                    (std::memcmp(reinterpret_cast<const void*>(target), kOctaneAppMutexOriginal, sizeof(kOctaneAppMutexOriginal)) == 0 ||
                     std::memcmp(reinterpret_cast<const void*>(target), kOctaneAppMutexPatched, sizeof(kOctaneAppMutexPatched)) == 0);

                if (!atRva) {
                    uintptr_t scanned = Memory::patternScan(kOctaneAppMutexPattern);
                    if (scanned) {
                        target = scanned;
                    }
                }

                PatchRecord rec;
                rec.address = target;
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

        Logger::getInstance().info("MemoryPatcher: {}/{} patches active", applied, _records.size());
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

} // namespace Multiplayer::Infrastructure

