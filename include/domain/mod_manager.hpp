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
        // The name the dependency resolver knows this mod by: what its mod.json
        // declares, or the "local.<folder>" id ModManager synthesised for a
        // manifest that declares none. Unique across a single discovery pass.
        std::string id;

        // The folder or script name as it appears on disk, which is what the
        // Lua sandbox is keyed on and what the reader has to go and look at.
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

        // Asked per state, from inside that state's own pcall, rather than
        // once globally: a hot reload has to reach every live state, and the
        // only safe thread to reach one from is its own. See
        // domain/reload_generation.hpp for why this is not a loop.
        [[nodiscard]] bool needsReload(void* L) const;
        void requestHotReload();

        [[nodiscard]] const std::vector<Mod>& getMods() const noexcept;
        [[nodiscard]] const std::filesystem::path& getModsFolder() const noexcept;

    private:
        ModManager() = default;
        ~ModManager() = default;
        ModManager(const ModManager&) = delete;
        ModManager& operator=(const ModManager&) = delete;

        // Both run one mod and return whether it ran. Neither records anything
        // in _mods and neither decides whether the mod should run at all: by
        // the time either is called the dependency resolver has already said
        // so, and discoverAndLoadMods owns the load report.
        // Both take the manifest the caller already parsed rather than reading
        // mod.json a second time: parsing it twice would also report any defect
        // in it twice. Which script a directory runs is domain/mod_entry.hpp's
        // answer, not one this class works out for itself.
        bool loadModDirectory(void* L, const std::filesystem::path& modPath,
                              const std::string& modName, const ModManifest& manifest);
        bool loadStandaloneScript(void* L, const std::filesystem::path& scriptPath,
                                  const std::string& modName);

        std::vector<Mod> _mods;
        std::filesystem::path _modsFolder;
        ReloadGenerations _reloads;
        std::mutex _mutex;
    };

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_MOD_MANAGER_HPP_ */
