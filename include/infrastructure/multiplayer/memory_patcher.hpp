/*
** CrabeLoader
** File description:
** Declares the engine patches that turn the Gold Edition offline flags back on.
** What lives here is true of the patch, not the build: the bytes, and the site byte pattern.
** Holds no address; every one arrives from the active domain/game_profile.hpp profile.
**
** Authors: @LucasLhomme
*/

#ifndef MEMORY_PATCHER_HPP_
#define MEMORY_PATCHER_HPP_

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "domain/multiplayer/i_engine_patcher.hpp"

namespace crabe::multiplayer::infrastructure {

    class MemoryPatcher final : public domain::IEnginePatcher {
    public:
        MemoryPatcher();
        ~MemoryPatcher() override;

        MemoryPatcher(const MemoryPatcher&) = delete;
        MemoryPatcher& operator=(const MemoryPatcher&) = delete;
        MemoryPatcher(MemoryPatcher&&) = delete;
        MemoryPatcher& operator=(MemoryPatcher&&) = delete;

        [[nodiscard]] std::expected<void, std::string> applyPatches() override;
        void restorePatches() override;

        [[nodiscard]] bool arePatchesActive() const noexcept override;
        [[nodiscard]] unsigned getPatchedCount() const noexcept override;

    private:
        struct PatchRecord {
            uintptr_t address{ 0 };
            std::vector<uint8_t> originalBytes;
            std::vector<uint8_t> patchedBytes;
            std::string name;
            bool active{ false };
        };

        bool applyRecord(PatchRecord& record);
        void restoreRecord(PatchRecord& record);

        std::mutex _mutex;
        std::vector<PatchRecord> _records;
        std::atomic<bool> _active{ false };
        std::atomic<unsigned> _patchedCount{ 0 };
    };

} // namespace crabe::multiplayer::infrastructure

#endif /* !MEMORY_PATCHER_HPP_ */

