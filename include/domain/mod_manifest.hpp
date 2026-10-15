/*
** CrabeLoader
** File description:
** Declares the parsed mod.json: three states, positional diagnostics, and both schema versions.
** An absent manifest is normal for a bare folder of .lua files and must stay silent.
** Chooses no id for a v0 manifest; that policy belongs to domain/mod_id.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_DOMAIN_MOD_MANIFEST_HPP_
#define CRABELOADER_DOMAIN_MOD_MANIFEST_HPP_

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "domain/semver.hpp"

namespace crabe::domain {

    enum class ManifestState : std::uint8_t {
        Valid,
        Absent,
        Malformed,
    };

    enum class ManifestError : std::uint8_t {
        FileNotFound,
        FileUnreadable,
        EmptyFile,
        InvalidJson,
        NotAnObject,
        WrongType,
        MissingRequiredField,
        InvalidId,
        InvalidVersion,
        InvalidVersionRange,
        InvalidDependency,
        UnsupportedManifestVersion,
    };

    // Returns a readable description of a manifest parsing error.
    [[nodiscard]] std::string_view describe(ManifestError error) noexcept;

    struct ManifestDiagnostic {
        ManifestError code{ManifestError::FileNotFound};
        std::string message;
        std::filesystem::path file;
        std::size_t byteOffset{0};
        std::size_t line{0};
        std::size_t column{0};
        std::string jsonPointer;

        // Returns true if the manifest was simply absent.
        [[nodiscard]] bool isAbsent() const noexcept
        {
            return code == ManifestError::FileNotFound;
        }

        // Formats the diagnostic into a log-ready line.
        [[nodiscard]] std::string what() const;
    };

    struct ManifestDependency {
        std::string id;
        std::string versionSpec;
        VersionRange range;
    };

    // Validates a mod id string against reverse-DNS syntax requirements.
    [[nodiscard]] std::expected<void, std::string> validateModId(std::string_view id);

    class ModManifest {
    public:
        ModManifest();

        // Parses the manifest at the given path, storing any diagnostic.
        explicit ModManifest(const std::filesystem::path& manifestPath);

        // Loads and parses a manifest file from disk.
        [[nodiscard]] static std::expected<ModManifest, ManifestDiagnostic> load(
            const std::filesystem::path& manifestPath);

        // Parses manifest contents from in-memory text.
        [[nodiscard]] static std::expected<ModManifest, ManifestDiagnostic> parse(
            std::string_view text, const std::filesystem::path& origin);

        [[nodiscard]] ManifestState state() const noexcept;
        [[nodiscard]] bool isValid() const noexcept;
        [[nodiscard]] bool isAbsent() const noexcept;
        [[nodiscard]] bool isMalformed() const noexcept;

        // Returns the diagnostic if the manifest is invalid or absent.
        [[nodiscard]] const std::optional<ManifestDiagnostic>& diagnostic() const noexcept;

        [[nodiscard]] unsigned int getManifestVersion() const noexcept;
        [[nodiscard]] const std::string& getId() const noexcept;
        [[nodiscard]] const std::string& getName() const noexcept;
        [[nodiscard]] const std::string& getVersion() const noexcept;
        [[nodiscard]] const std::string& getMinLoaderVersion() const noexcept;
        [[nodiscard]] const std::string& getMaxLoaderVersion() const noexcept;
        [[nodiscard]] const std::string& getEntry() const noexcept;
        [[nodiscard]] const std::string& getLicense() const noexcept;
        [[nodiscard]] const std::vector<std::string>& getAuthors() const noexcept;
        [[nodiscard]] const std::optional<SemVer>& getSemVer() const noexcept;
        [[nodiscard]] const std::vector<ManifestDependency>& getDependencies() const noexcept;
        [[nodiscard]] const std::vector<ManifestDependency>& getOptionalDependencies() const noexcept;
        [[nodiscard]] const std::vector<std::string>& getLoadAfter() const noexcept;
        [[nodiscard]] const std::vector<std::string>& getLoadBefore() const noexcept;
        [[nodiscard]] const std::vector<std::string>& getConflicts() const noexcept;
        [[nodiscard]] const std::vector<std::string>& getProvides() const noexcept;

        // Returns unrecognized keys present in the manifest.
        [[nodiscard]] const std::vector<std::string>& getIgnoredKeys() const noexcept;

        // Returns true if the running loader satisfies version constraints.
        [[nodiscard]] bool isCompatible() const;

    private:
        ManifestState _state{ManifestState::Absent};
        std::optional<ManifestDiagnostic> _diagnostic;

        unsigned int _manifestVersion{0};
        std::string _id;
        std::string _name;
        std::string _version;
        std::string _minLoaderVersion;
        std::string _maxLoaderVersion;
        std::string _entry;
        std::string _license;
        std::vector<std::string> _authors;
        std::optional<SemVer> _semver;
        std::optional<VersionRange> _maxLoaderRange;

        std::vector<ManifestDependency> _dependencies;
        std::vector<ManifestDependency> _optionalDependencies;
        std::vector<std::string> _loadAfter;
        std::vector<std::string> _loadBefore;
        std::vector<std::string> _conflicts;
        std::vector<std::string> _provides;
        std::vector<std::string> _ignoredKeys;
    };

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_MOD_MANIFEST_HPP_ */
