/*
** CrabeLoader
** File description:
** engine_patches -- engine-level binary patches
*/

#include "loader/engine_patches.hpp"

#include <cstdint>

#include "loader/codecave.hpp"
#include "loader/memory.hpp"
#include "logger/logger.hpp"

namespace EnginePatches {

    bool unlockEditorEverywhere()
    {
        constexpr const char* pattern = "0F B6 42 20 85 C0 74";
        uintptr_t site = Memory::patternScan(pattern);
        if (!site) {
            Logger::getInstance().warning("EnginePatches: editor unlock pattern not found.");
            return false;
        }
        uintptr_t jumpSite = site + 6;
        constexpr uint8_t nops[2] = { 0x90, 0x90 };
        if (!CodeCave::patchBytes(jumpSite, nops, sizeof(nops))) {
            Logger::getInstance().error("EnginePatches: failed to patch editor gate at 0x{:X}.", jumpSite);
            return false;
        }
        Logger::getInstance().info(
            "EnginePatches: Editor & Free Camera unlocked everywhere (patched 0x{:X}).", jumpSite);
        return true;
    }

}
