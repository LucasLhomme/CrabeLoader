/*
** CrabeLoader
** File description:
** ModManifest implementation
*/

#include <fstream>
#include <string_view>

#include "domain/ModManifest.hpp"
#include "shared/version.hpp"

namespace Crabe::Domain {

    namespace {

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

    bool ModManifest::isValid() const
    {
        return _valid;
    }

    const std::string& ModManifest::getName() const
    {
        return _name;
    }

    const std::string& ModManifest::getVersion() const
    {
        return _version;
    }

    const std::string& ModManifest::getMinLoaderVersion() const
    {
        return _minLoaderVersion;
    }

    const std::string& ModManifest::getEntry() const
    {
        return _entry;
    }

    bool ModManifest::isCompatible() const
    {
        if (_minLoaderVersion.empty())
            return true;
        return Crabe::Version::isCompatible(_minLoaderVersion);
    }

} // namespace Crabe::Domain
