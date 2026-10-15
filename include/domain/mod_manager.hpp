/*
** CrabeLoader
** File description:
** Declares mod discovery and drives their load, hot reload and shutdown.
** Mod chunks run sandboxed with the registry owner set, so a reload can revoke what they took.
** Resolves no load order itself -- it calls domain/dependency_resolver.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_DOMAIN_MOD_MANAGER_HPP_
#define CRABELOADER_DOMAIN_MOD_MANAGER_HPP_

#include "domain/reload_generation.hpp"

#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace crabe::domain {

    class ModManifest;

    struct Mod {
        std::string id;
        std::string name;
        std::filesystem::path rootPath;
        bool isLoaded = false;
    };

    class ModManager {
    public:
        // Returns the singleton instance of ModManager.
        static ModManager& get();

        // Discovers and loads mods located in the specified folder.
        void discoverAndLoadMods(void* L, const std::filesystem::path& modsFolder);

        // Triggers lifecycle shutdown and completely reloads all mods.
        void reloadAllMods(void* L);

        // Dispatches mod drawing callbacks for the current frame.
        void dispatchDraw(void* L);

        // Queries whether the given Lua state needs a hot reload.
        [[nodiscard]] bool needsReload(void* L) const;

        // Flags a hot reload request across all tracked states.
        void requestHotReload();

        // Returns the list of tracked mods.
        [[nodiscard]] const std::vector<Mod>& getMods() const noexcept;

        // Returns the root folder containing discovered mods.
        [[nodiscard]] const std::filesystem::path& getModsFolder() const noexcept;

    private:
        ModManager() = default;
        ~ModManager() = default;
        ModManager(const ModManager&) = delete;
        ModManager& operator=(const ModManager&) = delete;

        // Loads a mod directory using its resolved manifest.
        bool loadModDirectory(void* L, const std::filesystem::path& modPath,
                              const std::string& modName, const ModManifest& manifest);

        // Loads a standalone Lua mod script.
        bool loadStandaloneScript(void* L, const std::filesystem::path& scriptPath,
                                  const std::string& modName);

        std::vector<Mod> _mods;
        std::filesystem::path _modsFolder;
        ReloadGenerations _reloads;
        std::mutex _mutex;
    };

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_MOD_MANAGER_HPP_ */
