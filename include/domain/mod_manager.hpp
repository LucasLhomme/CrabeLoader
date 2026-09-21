#ifndef CRABELOADER_DOMAIN_MOD_MANAGER_HPP_
#define CRABELOADER_DOMAIN_MOD_MANAGER_HPP_

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace crabe::domain {

    struct Mod {
        std::string name;
        std::filesystem::path rootPath;
        bool isLoaded = false;
    };

    // Manages discovered mods, sandboxed execution, drawing dispatch
    // and hot reloading lifecycles.
    class ModManager {
    public:
        static ModManager& get();

        void discoverAndLoadMods(void* L, const std::filesystem::path& modsFolder);
        void reloadAllMods(void* L);
        void dispatchDraw(void* L);

        [[nodiscard]] bool isHotReloadRequested() const noexcept;
        void requestHotReload() noexcept;

        [[nodiscard]] const std::vector<Mod>& getMods() const noexcept;
        [[nodiscard]] const std::filesystem::path& getModsFolder() const noexcept;

    private:
        ModManager() = default;
        ~ModManager() = default;
        ModManager(const ModManager&) = delete;
        ModManager& operator=(const ModManager&) = delete;

        void loadModDirectory(void* L, const std::filesystem::path& modPath);
        void loadStandaloneScript(void* L, const std::filesystem::path& scriptPath);
        std::filesystem::path resolveEntryScript(
            const std::filesystem::path& modPath,
            const std::string& modName) const;

        std::vector<Mod> _mods;
        std::filesystem::path _modsFolder;
        std::atomic<bool> _hotReloadRequested{false};
        std::mutex _mutex;
    };

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_MOD_MANAGER_HPP_ */
