/*
** CrabeLoader
** File description:
** Builds one CrabePluginApi table per plugin and implements its functions as C callbacks.
** A callback finds its plugin from the table pointer, so one set of functions serves every plugin.
** No callback lets a C++ exception escape into the plugin, and CrabePluginInit runs under SEH.
**
** Authors: @LucasLhomme
*/

#include "application/plugin_api.hpp"

#include <windows.h>

#include <algorithm>
#include <cstring>
#include <format>

#include "shared/version.hpp"

namespace crabe::application {

    struct PluginApi::Entry {
        CrabePluginApi table{};
        PluginApi* api{nullptr};
        void* module{nullptr};
        std::string name;
        std::string owner;
        CrabePresentFn present{nullptr};
        void* presentUser{nullptr};
        CrabeResizeFn resize{nullptr};
        void* resizeUser{nullptr};
        std::atomic<bool> renderFaulted{false};
    };

    struct PluginApi::RenderCallbacks {
        struct Present {
            PluginApi::Entry* entry;
            CrabePresentFn callback;
            void* user;
        };
        struct Resize {
            PluginApi::Entry* entry;
            CrabeResizeFn callback;
            void* user;
        };
        std::vector<Present> present;
        std::vector<Resize> resize;
    };

    struct PluginApiAccess {
        /// Returns the host of the instance an entry belongs to.
        static PluginHost& host(const PluginApi::Entry& entry) { return entry.api->_host; }

        /// Stores a plugin's Present callback and republishes the list the render thread reads.
        static void setPresent(PluginApi::Entry& entry, CrabePresentFn callback, void* user)
        {
            const std::lock_guard lock(entry.api->_mutex);
            entry.present = callback;
            entry.presentUser = user;
            entry.api->publishRenderCallbacks();
        }

        /// Stores a plugin's ResizeBuffers callback and republishes the list the render thread reads.
        static void setResize(PluginApi::Entry& entry, CrabeResizeFn callback, void* user)
        {
            const std::lock_guard lock(entry.api->_mutex);
            entry.resize = callback;
            entry.resizeUser = user;
            entry.api->publishRenderCallbacks();
        }
    };

    namespace {

        struct TableIndex {
            std::mutex mutex;
            std::map<const CrabePluginApi*, PluginApi::Entry*> entries;
        };

        /// Maps every table handed out to its plugin. Never destroyed: a plugin may still call in while the
        /// process exits, after this DLL's statics would otherwise be gone.
        TableIndex& tables()
        {
            static TableIndex* index = std::make_unique<TableIndex>().release();
            return *index;
        }

        /// Returns the plugin a table belongs to, or null for a pointer CrabeLoader never handed out.
        PluginApi::Entry* entryOf(const CrabePluginApi* self)
        {
            TableIndex& index = tables();
            const std::lock_guard lock(index.mutex);
            const auto it = index.entries.find(self);
            return it == index.entries.end() ? nullptr : it->second;
        }

        /// CrabePluginApi::log.
        void __cdecl apiLog(const CrabePluginApi* self, int level, const char* message)
        {
            try {
                PluginApi::Entry* entry = entryOf(self);
                if (!entry || !message)
                    return;
                const int clamped = std::clamp(level, static_cast<int>(CRABE_LOG_DEBUG), static_cast<int>(CRABE_LOG_ERROR));
                PluginApiAccess::host(*entry).log(clamped, std::format("[{}] {}", entry->name, message));
            } catch (...) {
            }
        }

        /// CrabePluginApi::installHook.
        int __cdecl apiInstallHook(const CrabePluginApi* self, void* target, void* detour, void** original,
                                   const char* name)
        {
            try {
                PluginApi::Entry* entry = entryOf(self);
                if (!entry || !target || !detour)
                    return 0;
                const std::string hookName = std::format("{}:{}", entry->name, name ? name : "hook");
                return PluginApiAccess::host(*entry).installHook(entry->owner, hookName, target, detour, original) ? 1
                                                                                                                  : 0;
            } catch (...) {
                return 0;
            }
        }

        /// CrabePluginApi::removeHooks.
        int __cdecl apiRemoveHooks(const CrabePluginApi* self)
        {
            try {
                PluginApi::Entry* entry = entryOf(self);
                return entry ? static_cast<int>(PluginApiAccess::host(*entry).removeHooks(entry->owner)) : 0;
            } catch (...) {
                return 0;
            }
        }

        /// CrabePluginApi::runLua.
        int __cdecl apiRunLua(const CrabePluginApi* self, const char* code)
        {
            try {
                PluginApi::Entry* entry = entryOf(self);
                if (!entry || !code || std::strspn(code, " \t\r\n") == std::strlen(code))
                    return 0;
                PluginApiAccess::host(*entry).queueLua(entry->name, code);
                return 1;
            } catch (...) {
                return 0;
            }
        }

        /// CrabePluginApi::setValue.
        int __cdecl apiSetValue(const CrabePluginApi* self, const char* key, const char* value)
        {
            try {
                PluginApi::Entry* entry = entryOf(self);
                if (!entry || !key || !*key)
                    return 0;
                entry->api->setValue(key, value ? std::optional<std::string>(value) : std::nullopt);
                return 1;
            } catch (...) {
                return 0;
            }
        }

        /// CrabePluginApi::getValue.
        int __cdecl apiGetValue(const CrabePluginApi* self, const char* key, char* buffer, uint32_t bufferSize)
        {
            try {
                PluginApi::Entry* entry = entryOf(self);
                if (!entry || !key)
                    return -1;
                const std::optional<std::string> found = entry->api->value(key);
                if (!found)
                    return -1;
                if (buffer && bufferSize > 0) {
                    const std::size_t copied = std::min<std::size_t>(found->size(), bufferSize - 1);
                    std::memcpy(buffer, found->data(), copied);
                    buffer[copied] = '\0';
                }
                return static_cast<int>(found->size());
            } catch (...) {
                return -1;
            }
        }

        /// CrabePluginApi::onPresent.
        int __cdecl apiOnPresent(const CrabePluginApi* self, CrabePresentFn callback, void* user)
        {
            try {
                PluginApi::Entry* entry = entryOf(self);
                if (!entry)
                    return 0;
                PluginApiAccess::setPresent(*entry, callback, user);
                return 1;
            } catch (...) {
                return 0;
            }
        }

        /// CrabePluginApi::onBeforeResize.
        int __cdecl apiOnBeforeResize(const CrabePluginApi* self, CrabeResizeFn callback, void* user)
        {
            try {
                PluginApi::Entry* entry = entryOf(self);
                if (!entry)
                    return 0;
                PluginApiAccess::setResize(*entry, callback, user);
                return 1;
            } catch (...) {
                return 0;
            }
        }

        /// Calls a plugin's Present callback under SEH; returns the exception code it raised, or 0.
        DWORD callPresentGuarded(CrabePresentFn callback, void* swapChain, uint32_t flags, void* user) noexcept
        {
            __try {
                callback(swapChain, flags, user);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }
            return 0;
        }

        /// Calls a plugin's ResizeBuffers callback under SEH; returns the exception code it raised, or 0.
        DWORD callResizeGuarded(CrabeResizeFn callback, void* swapChain, void* user) noexcept
        {
            __try {
                callback(swapChain, user);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }
            return 0;
        }

        /// Calls a plugin's CrabePluginInit under SEH; returns the exception code it raised, or 0.
        DWORD callInitGuarded(CrabePluginInitFn init, const CrabePluginApi* table) noexcept
        {
            __try {
                init(table);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }
            return 0;
        }

        /// Returns a path's file name as UTF-8, which never throws whatever the name holds.
        std::string utf8FileName(const std::filesystem::path& path)
        {
            const std::u8string text = path.filename().u8string();
            return std::string(text.begin(), text.end());
        }

    } // namespace

    std::string pluginOwnerId(std::string_view pluginName)
    {
        return std::format("plugin:{}", pluginName);
    }

    PluginApi::PluginApi(PluginHost& host) : _host(host)
    {
    }

    PluginApi::~PluginApi()
    {
        TableIndex& index = tables();
        const std::lock_guard lock(index.mutex);
        for (const std::unique_ptr<Entry>& entry : _entries)
            index.entries.erase(&entry->table);
    }

    std::size_t PluginApi::startPlugins(const std::vector<crabe::infrastructure::AsiLoadRecord>& records)
    {
        std::size_t started = 0;
        for (const crabe::infrastructure::AsiLoadRecord& record : records) {
            if (!record.module)
                continue;
            {
                const std::lock_guard lock(_mutex);
                if (std::ranges::any_of(_entries, [&](const auto& entry) { return entry->module == record.module; }))
                    continue;
            }
            const auto init = reinterpret_cast<CrabePluginInitFn>(
                GetProcAddress(static_cast<HMODULE>(record.module), CRABE_PLUGIN_INIT_SYMBOL));
            if (!init)
                continue;

            auto entry = std::make_unique<Entry>();
            entry->api = this;
            entry->module = record.module;
            entry->name = utf8FileName(record.path);
            entry->owner = pluginOwnerId(entry->name);
            entry->table.size = sizeof(CrabePluginApi);
            entry->table.version = CRABE_PLUGIN_API_VERSION;
            entry->table.loaderVersion = crabe::version::String.data();
            entry->table.pluginName = entry->name.c_str();
            entry->table.log = &apiLog;
            entry->table.installHook = &apiInstallHook;
            entry->table.removeHooks = &apiRemoveHooks;
            entry->table.runLua = &apiRunLua;
            entry->table.setValue = &apiSetValue;
            entry->table.getValue = &apiGetValue;
            entry->table.onPresent = &apiOnPresent;
            entry->table.onBeforeResize = &apiOnBeforeResize;

            const CrabePluginApi* table = &entry->table;
            const std::string name = entry->name;
            {
                TableIndex& index = tables();
                const std::lock_guard tablesLock(index.mutex);
                index.entries.emplace(table, entry.get());
            }
            {
                const std::lock_guard lock(_mutex);
                _entries.push_back(std::move(entry));
            }

            const DWORD exception = callInitGuarded(init, table);
            if (exception != 0) {
                _host.log(CRABE_LOG_ERROR,
                          std::format("PluginApi: {} raised exception 0x{:08X} in CrabePluginInit; the plugin stays "
                                      "loaded but may not work.", name, exception));
                continue;
            }
            _host.log(CRABE_LOG_INFO, std::format("PluginApi: {} started with plugin API v{}.", name,
                                                  CRABE_PLUGIN_API_VERSION));
            ++started;
        }
        return started;
    }

    void PluginApi::setValue(const std::string& key, std::optional<std::string> value)
    {
        const std::lock_guard lock(_mutex);
        if (value)
            _values.insert_or_assign(key, std::move(*value));
        else
            _values.erase(key);
    }

    std::optional<std::string> PluginApi::value(const std::string& key) const
    {
        const std::lock_guard lock(_mutex);
        const auto it = _values.find(key);
        if (it == _values.end())
            return std::nullopt;
        return it->second;
    }

    void PluginApi::dispatchPresent(void* swapChain, std::uint32_t flags)
    {
        const RenderCallbacks* callbacks = _render.load(std::memory_order_acquire);
        if (!callbacks)
            return;
        for (const RenderCallbacks::Present& slot : callbacks->present) {
            if (slot.entry->renderFaulted.load(std::memory_order_relaxed))
                continue;
            const DWORD exception = callPresentGuarded(slot.callback, swapChain, flags, slot.user);
            if (exception != 0)
                disableRenderCallbacks(*slot.entry, "Present", exception);
        }
    }

    void PluginApi::dispatchBeforeResize(void* swapChain)
    {
        const RenderCallbacks* callbacks = _render.load(std::memory_order_acquire);
        if (!callbacks)
            return;
        for (const RenderCallbacks::Resize& slot : callbacks->resize) {
            if (slot.entry->renderFaulted.load(std::memory_order_relaxed))
                continue;
            const DWORD exception = callResizeGuarded(slot.callback, swapChain, slot.user);
            if (exception != 0)
                disableRenderCallbacks(*slot.entry, "ResizeBuffers", exception);
        }
    }

    // Published lists are never freed before the API itself: the render thread may still be walking an old one.
    void PluginApi::publishRenderCallbacks()
    {
        auto callbacks = std::make_unique<RenderCallbacks>();
        for (const std::unique_ptr<Entry>& entry : _entries) {
            if (entry->present)
                callbacks->present.push_back({entry.get(), entry->present, entry->presentUser});
            if (entry->resize)
                callbacks->resize.push_back({entry.get(), entry->resize, entry->resizeUser});
        }
        _render.store(callbacks.get(), std::memory_order_release);
        _publishedRender.push_back(std::move(callbacks));
    }

    void PluginApi::disableRenderCallbacks(Entry& entry, std::string_view event, unsigned long exception)
    {
        if (entry.renderFaulted.exchange(true))
            return;
        _host.log(CRABE_LOG_ERROR, std::format("PluginApi: {} raised exception 0x{:08X} in its {} callback; its render "
                                               "callbacks are off until the game restarts.", entry.name, exception, event));
    }

} // namespace crabe::application
