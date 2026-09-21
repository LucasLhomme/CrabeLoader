/*
** CrabeLoader
** File description:
** The impure half of game profiling: reads the running image headers and picks the profile once.
** Read once behind call_once, because the answer cannot change while the process lives.
** Interprets no PE field itself -- readPeIdentity and matchProfile are in the domain layer.
**
** Authors: @LucasLhomme
*/

// The impure half of domain/game_profile.hpp.
//
// Everything here needs the live process: a module handle, and the header bytes
// the loader mapped. It is kept out of src/domain/ so that windows.h never
// reaches the matching logic -- see the compile-time guards at the foot of
// src/domain/game_profile.cpp, which would fail the build if it did.
//
// What this file must not do is decide anything. It reads three numbers and
// hands them to matchProfile(); every rule about which build those numbers mean
// lives in the pure half, where a test can reach it.

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <span>
#include <windows.h>

#include "domain/game_profile.hpp"
#include "infrastructure/memory.hpp"
#include "shared/logger.hpp"

namespace {

    // How much of the mapped image to hand the parser.
    //
    // The headers sit in their own region, usually one page, and
    // crabe::memory::isReadable insists on a single region -- so the sizes are
    // tried largest first and the first readable one wins. 0x40 is the DOS
    // header alone: readPeIdentity will reject it, which is the honest answer
    // for an image whose headers are not mapped readable.
    constexpr std::size_t kHeaderProbeSizes[] = { 0x2000, 0x1000, 0x400, 0x40 };

    std::span<const unsigned char> mappedHeaders(uintptr_t base)
    {
        for (std::size_t size : kHeaderProbeSizes) {
            if (crabe::memory::isReadable(base, size))
                return { reinterpret_cast<const unsigned char*>(base), size };
        }
        return {};
    }

    // Detected once. The main module's headers cannot change under us, so a
    // second answer could only differ by being wrong, and both the Lua symbol
    // resolver and MemoryPatcher ask independently -- MemoryPatcher as early as
    // DLL_PROCESS_ATTACH, before the Loader thread exists.
    struct Detection {
        crabe::domain::PeIdentity identity{};
        std::optional<crabe::domain::GameProfile> profile{};
    };

    const Detection& detection()
    {
        static std::once_flag once;
        static Detection state;

        std::call_once(once, [] {
            crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
            auto base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));

            if (!base) {
                logger.error("GameProfile: no handle on the main module; no build can be "
                             "identified.");
                return;
            }

            std::optional<crabe::domain::PeIdentity> identity =
                crabe::domain::readPeIdentity(mappedHeaders(base));
            if (!identity) {
                logger.error("GameProfile: the main module's PE header at 0x{:X} could not be "
                             "read; no build can be identified.", base);
                return;
            }

            state.identity = *identity;
            state.profile = crabe::domain::matchProfile(*identity);

            if (state.profile) {
                logger.info("GameProfile: {} ({}), TimeDateStamp 0x{:08X}, SizeOfImage 0x{:X}.",
                            state.profile->id, state.profile->displayName,
                            identity->timeDateStamp, identity->sizeOfImage);
            } else {
                logger.warning("GameProfile: no profile matches this build "
                               "(TimeDateStamp 0x{:08X}, SizeOfImage 0x{:X}, CheckSum 0x{:08X}).",
                               identity->timeDateStamp, identity->sizeOfImage,
                               identity->checkSum);
            }
        });

        return state;
    }

} // namespace

namespace crabe::domain {

    std::optional<GameProfile> detectRunningGame()
    {
        return detection().profile;
    }

    PeIdentity runningGameIdentity()
    {
        return detection().identity;
    }

    const GameProfile* activeProfile()
    {
        const Detection& state = detection();
        return state.profile ? &*state.profile : nullptr;
    }

} // namespace crabe::domain
