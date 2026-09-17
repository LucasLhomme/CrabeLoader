/*
** CrabeLoader
** File description:
** ModManifest -- mod metadata value object
*/

#ifndef CRABELOADER_DOMAIN_MOD_MANIFEST_HPP_
#define CRABELOADER_DOMAIN_MOD_MANIFEST_HPP_

#include <filesystem>
#include <string>

namespace Crabe::Domain {

    class ModManifest {
    public:
        explicit ModManifest(const std::filesystem::path& manifestPath);
        ModManifest();
        ~ModManifest() = default;

        [[nodiscard]] bool isValid() const;
        [[nodiscard]] const std::string& getName() const;
        [[nodiscard]] const std::string& getVersion() const;
        [[nodiscard]] const std::string& getMinLoaderVersion() const;
        [[nodiscard]] const std::string& getEntry() const;

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
