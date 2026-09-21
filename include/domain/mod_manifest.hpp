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

    // Three outcomes, not two. A mod may legitimately be a bare directory of
    // .lua files, so an absent mod.json is normal and must stay silent; a
    // mod.json that exists but does not parse is a defect its author has to be
    // told about, by file and position.
    enum class ManifestState : std::uint8_t {
        Valid,
        Absent,
        Malformed,
    };

    enum class ManifestError : std::uint8_t {
        FileNotFound,               // the only non-defect: state is Absent
        FileUnreadable,             // exists, but could not be opened or read
        EmptyFile,
        InvalidJson,                // the parser refused it
        NotAnObject,                // valid JSON, but an array/string/number
        WrongType,                  // a known key holds the wrong JSON type
        MissingRequiredField,
        InvalidId,
        InvalidVersion,
        InvalidVersionRange,
        InvalidDependency,
        UnsupportedManifestVersion,
    };

    [[nodiscard]] std::string_view describe(ManifestError error) noexcept;

    // Everything needed to fix the file without a debugger: which file, where
    // in it, and what was wrong. `line`/`column`/`byteOffset` are 1-based and
    // all zero when the defect is not positional (a missing key has no
    // position), in which case `jsonPointer` locates it structurally instead.
    struct ManifestDiagnostic {
        ManifestError code{ManifestError::FileNotFound};
        std::string message;
        std::filesystem::path file;
        std::size_t byteOffset{0};
        std::size_t line{0};
        std::size_t column{0};
        std::string jsonPointer;

        // An absent manifest is not a defect; every other code is.
        [[nodiscard]] bool isAbsent() const noexcept
        {
            return code == ManifestError::FileNotFound;
        }

        // One log-ready line: "<file>:<line>:<col>: <message>".
        [[nodiscard]] std::string what() const;
    };

    struct ManifestDependency {
        std::string id;
        std::string versionSpec; // the range exactly as written, for messages
        VersionRange range;      // parsed; "*" when versionSpec was omitted
    };

    // Checks an id against ^[a-z0-9]+(\.[a-z0-9-]+)+$ and, on failure, returns
    // a clause naming the offending character and its index. Hand-written
    // rather than a std::regex precisely so the message can be that specific.
    // Exposed because the dependency resolver validates ids too.
    [[nodiscard]] std::expected<void, std::string> validateModId(std::string_view id);

    // A parsed mod.json.
    //
    // Two manifest versions exist. A file without "manifestVersion" is version
    // 0 -- the shape CrabeLoader shipped before there was a schema -- and only
    // name, version, minLoaderVersion and entry are read from it; everything
    // else, "id" included, is ignored. Version 1 is the full schema and
    // requires "id" and "version".
    //
    // Unknown keys are never fatal at either version: they are collected in
    // ignoredKeys() so the loader can mention them at debug level. Refusing
    // them would break every mod that carries extra metadata (the shipped
    // mods/_template/mod.json carries "author" and "description") and would
    // make the schema impossible to extend without a version bump.
    class ModManifest {
    public:
        ModManifest();

        // Kept for the existing call sites: parses if it can, and otherwise
        // holds the diagnostic instead of throwing. Prefer load() in new code.
        explicit ModManifest(const std::filesystem::path& manifestPath);

        // The destructor is deliberately not declared: declaring one would
        // suppress the implicit move operations, and load() returns manifests
        // by value inside a std::expected.

        [[nodiscard]] static std::expected<ModManifest, ManifestDiagnostic> load(
            const std::filesystem::path& manifestPath);

        // Parses in-memory text. `origin` only labels diagnostics; nothing is
        // read from disk, which is what makes the fixtures testable.
        [[nodiscard]] static std::expected<ModManifest, ManifestDiagnostic> parse(
            std::string_view text, const std::filesystem::path& origin);

        [[nodiscard]] ManifestState state() const noexcept;
        [[nodiscard]] bool isValid() const noexcept;
        [[nodiscard]] bool isAbsent() const noexcept;
        [[nodiscard]] bool isMalformed() const noexcept;

        // Empty when state() is Valid.
        [[nodiscard]] const std::optional<ManifestDiagnostic>& diagnostic() const noexcept;

        [[nodiscard]] unsigned int getManifestVersion() const noexcept;

        // Empty for every v0 manifest: `id` arrived with manifestVersion 1, and
        // v0 is read for backward compatibility precisely so existing mods keep
        // loading without one. A dependency resolver keyed on `id` therefore has
        // no key for those mods, and choosing what to do about it -- synthesise
        // one from the folder name, exclude them from resolution, or something
        // else -- is a policy decision this class deliberately does not make.
        [[nodiscard]] const std::string& getId() const noexcept;
        [[nodiscard]] const std::string& getName() const noexcept;
        [[nodiscard]] const std::string& getVersion() const noexcept;
        [[nodiscard]] const std::string& getMinLoaderVersion() const noexcept;
        [[nodiscard]] const std::string& getMaxLoaderVersion() const noexcept;
        [[nodiscard]] const std::string& getEntry() const noexcept;
        [[nodiscard]] const std::string& getLicense() const noexcept;
        [[nodiscard]] const std::vector<std::string>& getAuthors() const noexcept;

        // Set when "version" parsed as a semantic version. Always set for a
        // valid v1 manifest, because v1 requires it; best-effort for v0, which
        // never validated the field and whose existing mods must keep loading.
        [[nodiscard]] const std::optional<SemVer>& getSemVer() const noexcept;

        [[nodiscard]] const std::vector<ManifestDependency>& getDependencies() const noexcept;
        [[nodiscard]] const std::vector<ManifestDependency>& getOptionalDependencies() const noexcept;
        [[nodiscard]] const std::vector<std::string>& getLoadAfter() const noexcept;
        [[nodiscard]] const std::vector<std::string>& getLoadBefore() const noexcept;
        [[nodiscard]] const std::vector<std::string>& getConflicts() const noexcept;
        [[nodiscard]] const std::vector<std::string>& getProvides() const noexcept;

        // Keys the file carries that this manifest version does not define.
        [[nodiscard]] const std::vector<std::string>& getIgnoredKeys() const noexcept;

        // True when the running loader satisfies minLoaderVersion and does not
        // exceed maxLoaderVersion. A manifest that constrains neither is
        // compatible with everything.
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
