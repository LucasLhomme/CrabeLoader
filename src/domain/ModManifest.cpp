/*
** CrabeLoader
** File description:
** Reads mod.json with a minimal string scanner instead of a JSON library.
** Takes the first quoted value after each key: no escapes, numbers, booleans or nesting.
** Compatibility is delegated to Crabe::Version::isCompatible; nothing is loaded or logged here.
**
** Authors: @LucasLhomme
*/

#include <fstream>
#include <string_view>

#include "domain/ModManifest.hpp"
#include "shared/version.hpp"

namespace Crabe::Domain {

    namespace {

        // Extracts a string value for the specified key from minimal JSON.
        std::string extractValue(std::string_view json, std::string_view key)
        {
            std::string needle = "\"" + std::string(key) + "\"";
            auto pos = json.find(needle);
            if (pos == std::string_view::npos)
                return {};

            auto colon = json.find(':', pos + needle.size());
            if (colon == std::string_view::npos)
                return {};

            auto start = json.find('\"', colon + 1);
            if (start == std::string_view::npos)
                return {};

            auto end = json.find('\"', start + 1);
            if (end == std::string_view::npos)
                return {};

            return std::string(json.substr(start + 1, end - start - 1));
        }

    } // namespace

    ModManifest::ModManifest() = default;

    // Parses a basic manifest from the given path.
    ModManifest::ModManifest(const std::filesystem::path& manifestPath)
    {
        std::ifstream file(manifestPath);
        if (!file.is_open())
            return;

        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        if (content.empty())
            return;

        _name = extractValue(content, "name");
        _version = extractValue(content, "version");
        _minLoaderVersion = extractValue(content, "minLoaderVersion");
        _entry = extractValue(content, "entry");
        _valid = !_name.empty() || !_entry.empty();
    }

    // Returns whether the manifest has valid minimal fields.
    bool ModManifest::isValid() const
    {
        return _valid;
    }

    // Returns the declared name of the mod.
    const std::string& ModManifest::getName() const
    {
        return _name;
    }

    // Returns the declared version of the mod.
    const std::string& ModManifest::getVersion() const
    {
        return _version;
    }

    // Returns the minimum loader version required by the mod.
    const std::string& ModManifest::getMinLoaderVersion() const
    {
        return _minLoaderVersion;
    }

    // Returns the custom entry script if declared.
    const std::string& ModManifest::getEntry() const
    {
        return _entry;
    }

    // Checks whether the mod is compatible with the current loader version.
    bool ModManifest::isCompatible() const
    {
        if (_minLoaderVersion.empty())
            return true;
        return Crabe::Version::isCompatible(_minLoaderVersion);
    }

} // namespace Crabe::Domain
