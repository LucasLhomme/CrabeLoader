/*
** CrabeLoader
** File description:
** Declares the bridge between .asi plugins and CrabeLoader: the table sdk/crabe_plugin.h describes.
** Hands each plugin exporting CrabePluginInit its own table, and keeps the values shared with Lua.
** Reaches the log, hooks and Lua only through a PluginHost, so tests can stand in for the loader.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_APPLICATION_PLUGIN_API_HPP_
#define CRABELOADER_APPLICATION_PLUGIN_API_HPP_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "infrastructure/asi_loader.hpp"
#include "sdk/crabe_plugin.h"

namespace crabe::application {

    class PluginHost {
    public:
        virtual ~PluginHost() = default;

        /// Writes one line already prefixed with the plugin's name, at a CRABE_LOG_* level.
        virtual void log(int level, std::string_view line) = 0;

        /// Installs a hook owned by `owner`; returns false when the registry refuses it.
        virtual bool installHook(const std::string& owner, std::string_view name, void* target, void* detour,
                                 void** original) = 0;

        /// Removes every hook `owner` installed and returns how many there were.
        virtual std::size_t removeHooks(const std::string& owner) = 0;

        /// Queues a Lua chunk to run on the script thread; `owner` names it in error messages.
        virtual void queueLua(const std::string& owner, std::string code) = 0;
    };

    class PluginApi final {
    public:
        explicit PluginApi(PluginHost& host);
        ~PluginApi();

        PluginApi(const PluginApi&) = delete;
        PluginApi& operator=(const PluginApi&) = delete;

        /// The instance the running loader uses, backed by the real log, hook registry and Lua queue.
        [[nodiscard]] static PluginApi& get();

        /// Calls CrabePluginInit of every loaded plugin that exports it, once per plugin, under an SEH guard.
        /// Returns how many plugins received a table.
        std::size_t startPlugins(const std::vector<crabe::infrastructure::AsiLoadRecord>& records);

        /// Sets a shared value; an empty optional erases the key.
        void setValue(const std::string& key, std::optional<std::string> value);

        /// Returns a shared value, or nothing when the key is not set.
        [[nodiscard]] std::optional<std::string> value(const std::string& key) const;

        /// Calls every plugin's Present callback in load order, each under SEH. Takes no lock: safe in Present.
        void dispatchPresent(void* swapChain, std::uint32_t flags);

        /// Calls every plugin's ResizeBuffers callback in load order, each under SEH. Takes no lock.
        void dispatchBeforeResize(void* swapChain);

        struct Entry;
        struct RenderCallbacks;

    private:
        PluginHost& _host;
        mutable std::mutex _mutex;
        std::vector<std::unique_ptr<Entry>> _entries;
        std::map<std::string, std::string> _values;
        std::atomic<const RenderCallbacks*> _render{nullptr};
        std::vector<std::unique_ptr<const RenderCallbacks>> _publishedRender;

        /// Rebuilds the callback list from every entry and publishes it; call with _mutex held.
        void publishRenderCallbacks();

        /// Logs a render callback's fault once and stops calling that plugin's render callbacks.
        void disableRenderCallbacks(Entry& entry, std::string_view event, unsigned long exception);

        friend struct PluginApiAccess;
    };

    /// Returns the owner id the hook registry records for a plugin file name ("plugin:my_plugin.asi").
    [[nodiscard]] std::string pluginOwnerId(std::string_view pluginName);

} // namespace crabe::application

#endif /* !CRABELOADER_APPLICATION_PLUGIN_API_HPP_ */
