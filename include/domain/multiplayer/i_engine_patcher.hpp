/*
** CrabeLoader
** File description:
** IEnginePatcher - Domain interface for engine binary memory gates & patches
*/

#ifndef IENGINE_PATCHER_HPP_
#define IENGINE_PATCHER_HPP_

#include <expected>
#include <string>

namespace crabe::multiplayer::domain {

    class IEnginePatcher {
    public:
        virtual ~IEnginePatcher() = default;

        [[nodiscard]] virtual std::expected<void, std::string> applyPatches() = 0;
        virtual void restorePatches() = 0;

        [[nodiscard]] virtual bool arePatchesActive() const noexcept = 0;
        [[nodiscard]] virtual unsigned getPatchedCount() const noexcept = 0;
    };

} // namespace crabe::multiplayer::domain

#endif /* !IENGINE_PATCHER_HPP_ */

