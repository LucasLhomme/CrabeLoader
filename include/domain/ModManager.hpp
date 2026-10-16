/*
** CrabeLoader
** File description:
** Declares the singleton that discovers, loads and reloads the mods of the mods folder.
** Hot reload is a flag only: the caller polls isHotReloadRequested() then runs reloadAllMods().
** Owns no Lua state: each call receives L, and script execution is delegated to LuaCall.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_DOMAIN_MOD_MANAGER_HPP_
#define CRABELOADER_DOMAIN_MOD_MANAGER_HPP_

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace Crabe::Domain {

    struct Mod {
        std::string name;
        std::filesystem::path rootPath;
        bool isLoaded = false;
    };

    class ModManager {
    public:
        // Returns the singleton instance of the mod manager.
        static ModManager& get();

        // Discovers and loads mods from the given directory.
        void discoverAndLoadMods(void* L, const std::filesystem::path& modsFolder);

        // Reloads all active mods in the given Lua state.
        void reloadAllMods(void* L);

        // Dispatches the draw event across all loaded mods.
        void dispatchDraw(void* L);

        // Returns true if a hot reload has been requested.
        [[nodiscard]] bool isHotReloadRequested() const noexcept;

        // Requests a hot reload of all mods.
        void requestHotReload() noexcept;

        // Returns the list of discovered mods.
        [[nodiscard]] const std::vector<Mod>& getMods() const noexcept;

        // Returns the path to the mods directory.
        [[nodiscard]] const std::filesystem::path& getModsFolder() const noexcept;

    private:
        ModManager() = default;
        ~ModManager() = default;
        ModManager(const ModManager&) = delete;
        ModManager& operator=(const ModManager&) = delete;

        // Loads a single mod directory.
        void loadModDirectory(void* L, const std::filesystem::path& modPath);

        // Loads a standalone .lua script.
        void loadStandaloneScript(void* L, const std::filesystem::path& scriptPath);

        // Resolves the entry script for a mod directory.
        std::filesystem::path resolveEntryScript(
            const std::filesystem::path& modPath,
            const std::string& modName) const;

        std::vector<Mod> _mods;
        std::filesystem::path _modsFolder;
        std::atomic<bool> _hotReloadRequested{false};
        std::mutex _mutex;
    };

} // namespace Crabe::Domain

using ModManager = Crabe::Domain::ModManager;

#endif /* !CRABELOADER_DOMAIN_MOD_MANAGER_HPP_ */
