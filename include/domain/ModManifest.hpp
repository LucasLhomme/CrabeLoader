/*
** CrabeLoader
** File description:
** Value object holding a mod's mod.json metadata: name, version, minLoaderVersion and entry.
** isValid() is true once a name or an entry was found; isCompatible() checks the loader version.
** Read-only after construction: it never loads or runs the mod itself, see ModManager.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_DOMAIN_MOD_MANIFEST_HPP_
#define CRABELOADER_DOMAIN_MOD_MANIFEST_HPP_

#include <filesystem>
#include <string>

namespace Crabe::Domain {

    class ModManifest {
    public:
        // Parses a mod.json manifest file from the given path.
        explicit ModManifest(const std::filesystem::path& manifestPath);

        // Constructs an empty, invalid manifest.
        ModManifest();

        ~ModManifest() = default;

        // Returns true if the manifest was parsed successfully.
        [[nodiscard]] bool isValid() const;

        // Returns the declared name of the mod.
        [[nodiscard]] const std::string& getName() const;

        // Returns the declared version of the mod.
        [[nodiscard]] const std::string& getVersion() const;

        // Returns the minimum required CrabeLoader version.
        [[nodiscard]] const std::string& getMinLoaderVersion() const;

        // Returns the relative entry script path.
        [[nodiscard]] const std::string& getEntry() const;

        // Returns true if the manifest is compatible with the current loader version.
        [[nodiscard]] bool isCompatible() const;

    private:
        bool _valid{false};
        std::string _name;
        std::string _version;
        std::string _minLoaderVersion;
        std::string _entry;
    };

} // namespace Crabe::Domain

#endif /* !CRABELOADER_DOMAIN_MOD_MANIFEST_HPP_ */
