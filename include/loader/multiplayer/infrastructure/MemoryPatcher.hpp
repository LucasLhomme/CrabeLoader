/*
** CrabeLoader
** File description:
** MemoryPatcher - Implementation of IEnginePatcher for Disney Infinity 3.0
*/

#ifndef MEMORY_PATCHER_HPP_
#define MEMORY_PATCHER_HPP_

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "loader/multiplayer/domain/IEnginePatcher.hpp"

namespace Multiplayer::Infrastructure {

    class MemoryPatcher final : public Domain::IEnginePatcher {
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

} // namespace Multiplayer::Infrastructure

#endif /* !MEMORY_PATCHER_HPP_ */

