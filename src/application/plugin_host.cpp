/*
** CrabeLoader
** File description:
** The PluginHost the running loader gives plugins: loader.log, the hook registry, the Lua queue.
** Kept apart from plugin_api.cpp so the API's tests can run without a game or a Lua state.
** Owns nothing; every call is forwarded to the singleton that does the work.
**
** Authors: @LucasLhomme
*/

#include "application/plugin_api.hpp"

#include <cstdint>
#include <memory>

#include "application/loader.hpp"
#include "infrastructure/hook_registry.hpp"
#include "shared/logger.hpp"

namespace crabe::application {

    namespace {

        class LoaderPluginHost final : public PluginHost {
        public:
            /// Writes to loader.log at the matching level.
            void log(int level, std::string_view line) override
            {
                crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
                switch (level) {
                    case CRABE_LOG_DEBUG:
                        logger.debug("{}", line);
                        break;
                    case CRABE_LOG_WARNING:
                        logger.warning("{}", line);
                        break;
                    case CRABE_LOG_ERROR:
                        logger.error("{}", line);
                        break;
                    default:
                        logger.info("{}", line);
                        break;
                }
            }

            /// Installs through the core registry, which logs the reason of a refusal.
            bool installHook(const std::string& owner, std::string_view name, void* target, void* detour,
                             void** original) override
            {
                const auto installed = crabe::infrastructure::coreRegistry().install(
                    owner, name, reinterpret_cast<std::uintptr_t>(target), detour, original);
                if (!installed)
                    crabe::shared::Logger::getInstance().warning(
                        "PluginApi: {} could not hook 0x{:08X} ({}).", owner,
                        reinterpret_cast<std::uintptr_t>(target), name);
                return installed.has_value();
            }

            /// Removes through the core registry.
            std::size_t removeHooks(const std::string& owner) override
            {
                return crabe::infrastructure::coreRegistry().removeAllOwnedBy(owner);
            }

            /// Queues on the loader, which runs it on the script thread.
            void queueLua(const std::string& owner, std::string code) override
            {
                Loader::get().queuePluginSnippet(owner, std::move(code));
            }
        };

    } // namespace

    PluginApi& PluginApi::get()
    {
        static LoaderPluginHost* host = std::make_unique<LoaderPluginHost>().release();
        static PluginApi* instance = std::make_unique<PluginApi>(*host).release();
        return *instance;
    }

} // namespace crabe::application
