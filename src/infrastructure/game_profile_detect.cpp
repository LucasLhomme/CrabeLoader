/*
** CrabeLoader
** File description:
** The impure half of game profiling: reads the running image headers and picks the profile once.
** Read once behind call_once, because the answer cannot change while the process lives.
** Interprets no PE field itself -- readPeIdentity and matchProfile are in the domain layer.
**
** Authors: @LucasLhomme
*/

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

    constexpr std::size_t kHeaderProbeSizes[] = { 0x2000, 0x1000, 0x400, 0x40 };

    // Probes mapped headers using predefined sizes and returns a valid readable span.
    std::span<const unsigned char> mappedHeaders(uintptr_t base)
    {
        for (std::size_t size : kHeaderProbeSizes) {
            if (crabe::memory::isReadable(base, size))
                return { reinterpret_cast<const unsigned char*>(base), size };
        }
        return {};
    }

    struct Detection {
        crabe::domain::PeIdentity identity{};
        std::optional<crabe::domain::GameProfile> profile{};
    };

    // Resolves and caches the detected PE identity and matching GameProfile.
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

    // Reads main module PE header and matches it against known game profiles.
    std::optional<GameProfile> detectRunningGame()
    {
        return detection().profile;
    }

    // Returns the PE identity of the running game image.
    PeIdentity runningGameIdentity()
    {
        return detection().identity;
    }

    // Returns the active GameProfile for the running executable, or nullptr if unmatched.
    const GameProfile* activeProfile()
    {
        const Detection& state = detection();
        return state.profile ? &*state.profile : nullptr;
    }

} // namespace crabe::domain
