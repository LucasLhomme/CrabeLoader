/*
** CrabeLoader
** File description:
** Parses mod.json against both schema versions, reporting every defect by line and column.
** The id pattern is quoted verbatim in each message, so an author can compare it with theirs.
** Synthesises no id for a v0 manifest; that policy belongs to src/domain/mod_id.cpp.
**
** Authors: @LucasLhomme
*/

#include "domain/mod_manifest.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <span>
#include <utility>

#include "shared/version.hpp"
#include "third_party/json.hpp"

namespace crabe::domain {

    namespace {

        using Json = nlohmann::json;

        constexpr unsigned int kHighestSupportedManifestVersion = 1;

        // The canonical spelling of the id pattern, quoted verbatim in error
        // messages so the reader can compare it against what they wrote.
        constexpr std::string_view kIdPattern = "^[a-z0-9]+(\\.[a-z0-9-]+)+$";

        constexpr std::array<std::string_view, 5> kKnownKeysV0 = {
            "manifestVersion", "name", "version", "minLoaderVersion", "entry",
        };

        constexpr std::array<std::string_view, 15> kKnownKeysV1 = {
            "manifestVersion", "id", "name", "version", "authors",
            "license", "entry", "minLoaderVersion", "maxLoaderVersion",
            "dependencies", "optionalDependencies", "loadAfter", "loadBefore",
            "conflicts", "provides",
        };

        [[nodiscard]] std::string_view jsonTypeName(const Json& value) noexcept
        {
            switch (value.type()) {
            case Json::value_t::null:
                return "null";
            case Json::value_t::object:
                return "an object";
            case Json::value_t::array:
                return "an array";
            case Json::value_t::string:
                return "a string";
            case Json::value_t::boolean:
                return "a boolean";
            case Json::value_t::number_integer:
            case Json::value_t::number_unsigned:
            case Json::value_t::number_float:
                return "a number";
            case Json::value_t::binary:
                return "binary data";
            case Json::value_t::discarded:
                break;
            }
            return "an unrecognised value";
        }

        struct SourcePosition {
            std::size_t line{0};
            std::size_t column{0};
        };

        // `byte` is nlohmann's 1-based index of the last character it read, so
        // the character it blames sits at byte - 1.
        [[nodiscard]] SourcePosition positionOf(std::string_view text, std::size_t byte) noexcept
        {
            if (byte == 0)
                return {};

            const std::size_t limit = std::min(byte - 1, text.size());
            SourcePosition position{1, 1};
            for (std::size_t i = 0; i < limit; ++i) {
                if (text[i] == '\n') {
                    ++position.line;
                    position.column = 1;
                } else {
                    ++position.column;
                }
            }
            return position;
        }

        [[nodiscard]] ManifestDiagnostic makeDiagnostic(ManifestError code,
                                                        const std::filesystem::path& file,
                                                        std::string message,
                                                        std::string pointer = {})
        {
            ManifestDiagnostic diagnostic;
            diagnostic.code = code;
            diagnostic.file = file;
            diagnostic.message = std::move(message);
            diagnostic.jsonPointer = std::move(pointer);
            return diagnostic;
        }

        // nlohmann's what() reads
        // "[json.exception.parse_error.101] parse error at line 3, column 5: <detail>".
        // The exception id is noise and the position is reported separately in
        // structured form, so only <detail> is kept.
        [[nodiscard]] std::string parseErrorDetail(std::string_view what)
        {
            if (const std::size_t bracket = what.find("] "); bracket != std::string_view::npos)
                what.remove_prefix(bracket + 2);

            constexpr std::string_view kAtLine = "parse error at line";
            constexpr std::string_view kDash = "parse error - ";
            if (what.starts_with(kAtLine)) {
                if (const std::size_t colon = what.find(": "); colon != std::string_view::npos)
                    what.remove_prefix(colon + 2);
            } else if (what.starts_with(kDash)) {
                what.remove_prefix(kDash.size());
            }
            return std::string(what);
        }

        // ------------------------------------------------------------------
        // Typed field access.
        //
        // Every reader below reports the key, the type it wanted and the type
        // it found, plus a JSON pointer: a manifest defect has to be fixable
        // from the log line alone.
        // ------------------------------------------------------------------

        class FieldReader {
        public:
            FieldReader(const Json& root, const std::filesystem::path& file)
                : _root(root), _file(file)
            {
            }

            // A std::string is built on purpose: nlohmann's heterogeneous
            // lookup depends on the object comparator being transparent, which
            // is not something to bet a build on for one avoided allocation.
            [[nodiscard]] const Json* find(std::string_view key) const
            {
                const auto it = _root.find(std::string(key));
                if (it == _root.end() || it->is_null())
                    return nullptr;
                return &(*it);
            }

            [[nodiscard]] ManifestDiagnostic wrongType(std::string_view key,
                                                       std::string_view wanted,
                                                       const Json& found,
                                                       std::string pointer = {}) const
            {
                if (pointer.empty())
                    pointer = std::format("/{}", key);
                return makeDiagnostic(
                    ManifestError::WrongType, _file,
                    std::format("\"{}\" must be {}, but it is {}", key, wanted, jsonTypeName(found)),
                    std::move(pointer));
            }

            [[nodiscard]] ManifestDiagnostic missing(std::string_view key,
                                                     std::string_view why) const
            {
                return makeDiagnostic(ManifestError::MissingRequiredField, _file,
                                      std::format("\"{}\" is required but missing: {}", key, why),
                                      std::format("/{}", key));
            }

            // Absent -> empty string. Present but not a string -> diagnostic.
            [[nodiscard]] std::expected<std::string, ManifestDiagnostic> optionalString(
                std::string_view key) const
            {
                const Json* value = find(key);
                if (value == nullptr)
                    return std::string{};
                if (!value->is_string())
                    return std::unexpected(wrongType(key, "a string", *value));
                return value->get<std::string>();
            }

            [[nodiscard]] std::expected<std::vector<std::string>, ManifestDiagnostic>
            optionalStringArray(std::string_view key, bool entriesAreModIds) const
            {
                std::vector<std::string> out;
                const Json* value = find(key);
                if (value == nullptr)
                    return out;
                if (!value->is_array())
                    return std::unexpected(wrongType(key, "an array of strings", *value));

                out.reserve(value->size());
                for (std::size_t i = 0; i < value->size(); ++i) {
                    const Json& element = (*value)[i];
                    const std::string pointer = std::format("/{}/{}", key, i);
                    if (!element.is_string()) {
                        return std::unexpected(makeDiagnostic(
                            ManifestError::WrongType, _file,
                            std::format("\"{}\"[{}] must be a string, but it is {}", key, i,
                                        jsonTypeName(element)),
                            pointer));
                    }
                    std::string text = element.get<std::string>();
                    if (text.empty()) {
                        return std::unexpected(makeDiagnostic(
                            ManifestError::WrongType, _file,
                            std::format("\"{}\"[{}] is an empty string", key, i), pointer));
                    }
                    if (entriesAreModIds) {
                        if (const auto ok = validateModId(text); !ok) {
                            return std::unexpected(makeDiagnostic(
                                ManifestError::InvalidId, _file,
                                std::format("\"{}\"[{}] \"{}\" is not a valid mod id: {}", key, i,
                                            text, ok.error()),
                                pointer));
                        }
                    }
                    out.push_back(std::move(text));
                }
                return out;
            }

            [[nodiscard]] std::expected<std::vector<ManifestDependency>, ManifestDiagnostic>
            optionalDependencyArray(std::string_view key,
                                    std::vector<std::string>& ignoredKeys) const
            {
                std::vector<ManifestDependency> out;
                const Json* value = find(key);
                if (value == nullptr)
                    return out;
                if (!value->is_array())
                    return std::unexpected(wrongType(key, "an array of objects", *value));

                out.reserve(value->size());
                for (std::size_t i = 0; i < value->size(); ++i) {
                    const Json& element = (*value)[i];
                    if (!element.is_object()) {
                        return std::unexpected(makeDiagnostic(
                            ManifestError::InvalidDependency, _file,
                            std::format("\"{}\"[{}] must be an object such as "
                                        "{{ \"id\": \"com.example.other\", \"version\": \">=0.4.0\" }}, "
                                        "but it is {}",
                                        key, i, jsonTypeName(element)),
                            std::format("/{}/{}", key, i)));
                    }

                    ManifestDependency dependency;

                    const auto idIt = element.find("id");
                    if (idIt == element.end() || idIt->is_null()) {
                        return std::unexpected(makeDiagnostic(
                            ManifestError::InvalidDependency, _file,
                            std::format("\"{}\"[{}] has no \"id\"", key, i),
                            std::format("/{}/{}/id", key, i)));
                    }
                    if (!idIt->is_string()) {
                        return std::unexpected(makeDiagnostic(
                            ManifestError::WrongType, _file,
                            std::format("\"{}\"[{}].id must be a string, but it is {}", key, i,
                                        jsonTypeName(*idIt)),
                            std::format("/{}/{}/id", key, i)));
                    }
                    dependency.id = idIt->get<std::string>();
                    if (const auto ok = validateModId(dependency.id); !ok) {
                        return std::unexpected(makeDiagnostic(
                            ManifestError::InvalidId, _file,
                            std::format("\"{}\"[{}].id \"{}\" is not a valid mod id: {}", key, i,
                                        dependency.id, ok.error()),
                            std::format("/{}/{}/id", key, i)));
                    }

                    const auto versionIt = element.find("version");
                    if (versionIt != element.end() && !versionIt->is_null()) {
                        if (!versionIt->is_string()) {
                            return std::unexpected(makeDiagnostic(
                                ManifestError::WrongType, _file,
                                std::format("\"{}\"[{}].version must be a version range string "
                                            "such as \">=0.4.0\", but it is {}",
                                            key, i, jsonTypeName(*versionIt)),
                                std::format("/{}/{}/version", key, i)));
                        }
                        dependency.versionSpec = versionIt->get<std::string>();
                        const auto range = VersionRange::parse(dependency.versionSpec);
                        if (!range) {
                            return std::unexpected(makeDiagnostic(
                                ManifestError::InvalidVersionRange, _file,
                                std::format("\"{}\"[{}].version \"{}\" is not a version range: {}",
                                            key, i, dependency.versionSpec,
                                            describe(range.error())),
                                std::format("/{}/{}/version", key, i)));
                        }
                        dependency.range = *range;
                    } else {
                        // An omitted range means any version of that mod will
                        // do, which is what "*" says.
                        dependency.versionSpec = "*";
                    }

                    for (auto member = element.begin(); member != element.end(); ++member) {
                        const std::string& memberKey = member.key();
                        if (memberKey != "id" && memberKey != "version")
                            ignoredKeys.push_back(std::format("{}[{}].{}", key, i, memberKey));
                    }

                    out.push_back(std::move(dependency));
                }
                return out;
            }

        private:
            const Json& _root;
            const std::filesystem::path& _file;
        };

        void collectIgnoredKeys(const Json& root,
                                std::span<const std::string_view> knownKeys,
                                std::vector<std::string>& out)
        {
            for (auto it = root.begin(); it != root.end(); ++it) {
                const std::string& key = it.key();
                if (std::ranges::find(knownKeys, key) == knownKeys.end())
                    out.push_back(key);
            }
        }

    } // namespace

    std::string_view describe(ManifestError error) noexcept
    {
        switch (error) {
        case ManifestError::FileNotFound:
            return "there is no mod.json (a mod may be a bare directory of .lua files)";
        case ManifestError::FileUnreadable:
            return "the file exists but could not be read";
        case ManifestError::EmptyFile:
            return "the file is empty";
        case ManifestError::InvalidJson:
            return "the file is not valid JSON";
        case ManifestError::NotAnObject:
            return "the top level of the file is not a JSON object";
        case ManifestError::WrongType:
            return "a field holds the wrong kind of value";
        case ManifestError::MissingRequiredField:
            return "a required field is missing";
        case ManifestError::InvalidId:
            return "a mod id does not match the required pattern";
        case ManifestError::InvalidVersion:
            return "a version is not a semantic version";
        case ManifestError::InvalidVersionRange:
            return "a version range could not be parsed";
        case ManifestError::InvalidDependency:
            return "a dependency entry is malformed";
        case ManifestError::UnsupportedManifestVersion:
            return "the manifest version is newer than this loader understands";
        }
        return "the manifest could not be read";
    }

    std::string ManifestDiagnostic::what() const
    {
        std::string out = file.empty() ? std::string("<no file>") : file.string();
        if (line != 0)
            out += std::format(":{}:{}", line, column);
        out += ": ";
        out += message;
        if (line == 0 && !jsonPointer.empty())
            out += std::format(" (at \"{}\")", jsonPointer);
        return out;
    }

    std::expected<void, std::string> validateModId(std::string_view id)
    {
        // ^[a-z0-9]+(\.[a-z0-9-]+)+$ -- checked by hand rather than with
        // std::regex so the failure can name the character and its index.
        if (id.empty())
            return std::unexpected(std::format("it is empty; ids must match {}", kIdPattern));

        std::size_t segmentStart = 0;
        std::size_t segmentIndex = 0;
        std::size_t position = 0;

        while (true) {
            const bool atEnd = position == id.size();
            if (atEnd || id[position] == '.') {
                const std::size_t length = position - segmentStart;
                if (length == 0) {
                    return std::unexpected(std::format(
                        "segment {} is empty (index {}); ids must match {}",
                        segmentIndex + 1, position, kIdPattern));
                }
                if (atEnd)
                    break;
                ++segmentIndex;
                segmentStart = position + 1;
                ++position;
                continue;
            }

            const char c = id[position];
            const bool lower = c >= 'a' && c <= 'z';
            const bool digit = c >= '0' && c <= '9';
            const bool hyphen = c == '-';

            if (c >= 'A' && c <= 'Z') {
                return std::unexpected(std::format(
                    "'{}' at index {} is upper case; ids are lower case and must match {}",
                    c, position, kIdPattern));
            }
            if (hyphen && segmentIndex == 0) {
                return std::unexpected(std::format(
                    "'-' at index {} is in the first segment, which accepts only [a-z0-9]; "
                    "ids must match {}",
                    position, kIdPattern));
            }
            if (!lower && !digit && !hyphen) {
                return std::unexpected(std::format(
                    "'{}' at index {} is not allowed; ids must match {}", c, position, kIdPattern));
            }
            ++position;
        }

        if (segmentIndex == 0) {
            return std::unexpected(std::format(
                "it has no '.', so it is a single segment; ids are reverse-DNS with at least two "
                "segments, such as \"com.example.my-mod\", and must match {}",
                kIdPattern));
        }
        return {};
    }

    ModManifest::ModManifest() = default;

    ModManifest::ModManifest(const std::filesystem::path& manifestPath)
    {
        auto result = load(manifestPath);
        if (result) {
            *this = std::move(*result);
            return;
        }
        _diagnostic = std::move(result.error());
        _state = _diagnostic->isAbsent() ? ManifestState::Absent : ManifestState::Malformed;
    }

    std::expected<ModManifest, ManifestDiagnostic> ModManifest::load(
        const std::filesystem::path& manifestPath)
    {
        std::error_code ec;
        const auto status = std::filesystem::status(manifestPath, ec);
        if (ec || !std::filesystem::exists(status)) {
            return std::unexpected(makeDiagnostic(
                ManifestError::FileNotFound, manifestPath,
                "there is no mod.json here"));
        }
        if (!std::filesystem::is_regular_file(status)) {
            return std::unexpected(makeDiagnostic(
                ManifestError::FileUnreadable, manifestPath,
                "this path is not a regular file"));
        }

        std::ifstream file(manifestPath, std::ios::binary);
        if (!file.is_open()) {
            return std::unexpected(makeDiagnostic(
                ManifestError::FileUnreadable, manifestPath,
                "the file could not be opened for reading"));
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        if (file.bad()) {
            return std::unexpected(makeDiagnostic(
                ManifestError::FileUnreadable, manifestPath,
                "the file could not be read to the end"));
        }

        return parse(content, manifestPath);
    }

    std::expected<ModManifest, ManifestDiagnostic> ModManifest::parse(
        std::string_view text, const std::filesystem::path& origin)
    {
        if (text.find_first_not_of(" \t\r\n") == std::string_view::npos) {
            return std::unexpected(makeDiagnostic(
                ManifestError::EmptyFile, origin,
                "the file is empty; delete it, or give it at least "
                "{ \"manifestVersion\": 1, \"id\": \"com.example.my-mod\", \"version\": \"1.0.0\" }"));
        }

        Json root;
        try {
            root = Json::parse(text.begin(), text.end());
        } catch (const Json::parse_error& error) {
            ManifestDiagnostic diagnostic = makeDiagnostic(
                ManifestError::InvalidJson, origin,
                std::format("not valid JSON: {}", parseErrorDetail(error.what())));
            diagnostic.byteOffset = error.byte;
            const SourcePosition position = positionOf(text, error.byte);
            diagnostic.line = position.line;
            diagnostic.column = position.column;
            return std::unexpected(std::move(diagnostic));
        }

        if (!root.is_object()) {
            return std::unexpected(makeDiagnostic(
                ManifestError::NotAnObject, origin,
                std::format("the top level of a manifest must be a JSON object, but this is {}",
                            jsonTypeName(root))));
        }

        ModManifest manifest;
        const FieldReader reader(root, origin);

        // manifestVersion decides which schema applies, so it is read first
        // and read strictly.
        if (const Json* declared = reader.find("manifestVersion"); declared != nullptr) {
            if (!declared->is_number_unsigned()) {
                return std::unexpected(reader.wrongType(
                    "manifestVersion", "a non-negative whole number", *declared));
            }
            manifest._manifestVersion = declared->get<unsigned int>();
            if (manifest._manifestVersion > kHighestSupportedManifestVersion) {
                return std::unexpected(makeDiagnostic(
                    ManifestError::UnsupportedManifestVersion, origin,
                    std::format("\"manifestVersion\" is {}, but this loader understands 0 to {}; "
                                "update CrabeLoader to load this mod",
                                manifest._manifestVersion, kHighestSupportedManifestVersion),
                    "/manifestVersion"));
            }
        }

        // Shared by both schema versions.
        if (auto value = reader.optionalString("name"); value)
            manifest._name = std::move(*value);
        else
            return std::unexpected(std::move(value.error()));

        if (auto value = reader.optionalString("version"); value)
            manifest._version = std::move(*value);
        else
            return std::unexpected(std::move(value.error()));

        if (auto value = reader.optionalString("minLoaderVersion"); value)
            manifest._minLoaderVersion = std::move(*value);
        else
            return std::unexpected(std::move(value.error()));

        if (auto value = reader.optionalString("entry"); value)
            manifest._entry = std::move(*value);
        else
            return std::unexpected(std::move(value.error()));

        if (manifest._manifestVersion == 0) {
            // v0 is frozen: exactly the four keys above, no validation beyond
            // their type. Existing mods were written against a loader that
            // never checked more than that, and tightening it now would
            // retroactively break them.
            //
            // "version" is still parsed if it happens to be a semantic
            // version, because the dependency resolver can use it when it is
            // there -- but a v0 manifest is never rejected over it.
            if (auto parsed = SemVer::parse(manifest._version); parsed)
                manifest._semver = *parsed;

            collectIgnoredKeys(root, kKnownKeysV0, manifest._ignoredKeys);
            manifest._state = ManifestState::Valid;
            return manifest;
        }

        // ---- manifestVersion 1 ----

        if (const Json* value = reader.find("id"); value == nullptr) {
            return std::unexpected(reader.missing(
                "id", "every v1 manifest is identified by a reverse-DNS id such as "
                      "\"com.example.my-mod\""));
        } else if (!value->is_string()) {
            return std::unexpected(reader.wrongType("id", "a string", *value));
        } else {
            manifest._id = value->get<std::string>();
        }

        if (const auto ok = validateModId(manifest._id); !ok) {
            return std::unexpected(makeDiagnostic(
                ManifestError::InvalidId, origin,
                std::format("\"id\" \"{}\" is not a valid mod id: {}", manifest._id, ok.error()),
                "/id"));
        }

        if (manifest._version.empty()) {
            return std::unexpected(reader.missing(
                "version", "a v1 manifest states its own semantic version, such as \"1.2.0\""));
        }
        if (const auto parsed = SemVer::parse(manifest._version); parsed) {
            manifest._semver = *parsed;
        } else {
            return std::unexpected(makeDiagnostic(
                ManifestError::InvalidVersion, origin,
                std::format("\"version\" \"{}\" is not a semantic version: {}", manifest._version,
                            describe(parsed.error())),
                "/version"));
        }

        if (!manifest._minLoaderVersion.empty()) {
            if (const auto parsed = SemVer::parse(manifest._minLoaderVersion); !parsed) {
                return std::unexpected(makeDiagnostic(
                    ManifestError::InvalidVersion, origin,
                    std::format("\"minLoaderVersion\" \"{}\" is not a semantic version: {}",
                                manifest._minLoaderVersion, describe(parsed.error())),
                    "/minLoaderVersion"));
            }
        }

        if (auto value = reader.optionalString("maxLoaderVersion"); value)
            manifest._maxLoaderVersion = std::move(*value);
        else
            return std::unexpected(std::move(value.error()));

        if (!manifest._maxLoaderVersion.empty()) {
            const auto range = VersionRange::parse(manifest._maxLoaderVersion);
            if (!range) {
                return std::unexpected(makeDiagnostic(
                    ManifestError::InvalidVersionRange, origin,
                    std::format("\"maxLoaderVersion\" \"{}\" is not a version range: {}",
                                manifest._maxLoaderVersion, describe(range.error())),
                    "/maxLoaderVersion"));
            }
            manifest._maxLoaderRange = *range;
        }

        if (auto value = reader.optionalString("license"); value)
            manifest._license = std::move(*value);
        else
            return std::unexpected(std::move(value.error()));

        if (auto value = reader.optionalStringArray("authors", false); value)
            manifest._authors = std::move(*value);
        else
            return std::unexpected(std::move(value.error()));

        if (auto value = reader.optionalDependencyArray("dependencies", manifest._ignoredKeys); value)
            manifest._dependencies = std::move(*value);
        else
            return std::unexpected(std::move(value.error()));

        if (auto value = reader.optionalDependencyArray("optionalDependencies",
                                                        manifest._ignoredKeys);
            value)
            manifest._optionalDependencies = std::move(*value);
        else
            return std::unexpected(std::move(value.error()));

        // Ordering hints and conflicts name other mods, so their entries are
        // held to the id pattern. "provides" names capabilities, which are
        // free-form ("hud-registry" is not and need not be an id).
        if (auto value = reader.optionalStringArray("loadAfter", true); value)
            manifest._loadAfter = std::move(*value);
        else
            return std::unexpected(std::move(value.error()));

        if (auto value = reader.optionalStringArray("loadBefore", true); value)
            manifest._loadBefore = std::move(*value);
        else
            return std::unexpected(std::move(value.error()));

        if (auto value = reader.optionalStringArray("conflicts", true); value)
            manifest._conflicts = std::move(*value);
        else
            return std::unexpected(std::move(value.error()));

        if (auto value = reader.optionalStringArray("provides", false); value)
            manifest._provides = std::move(*value);
        else
            return std::unexpected(std::move(value.error()));

        collectIgnoredKeys(root, kKnownKeysV1, manifest._ignoredKeys);
        manifest._state = ManifestState::Valid;
        return manifest;
    }

    ManifestState ModManifest::state() const noexcept
    {
        return _state;
    }

    bool ModManifest::isValid() const noexcept
    {
        return _state == ManifestState::Valid;
    }

    bool ModManifest::isAbsent() const noexcept
    {
        return _state == ManifestState::Absent;
    }

    bool ModManifest::isMalformed() const noexcept
    {
        return _state == ManifestState::Malformed;
    }

    const std::optional<ManifestDiagnostic>& ModManifest::diagnostic() const noexcept
    {
        return _diagnostic;
    }

    unsigned int ModManifest::getManifestVersion() const noexcept
    {
        return _manifestVersion;
    }

    const std::string& ModManifest::getId() const noexcept
    {
        return _id;
    }

    const std::string& ModManifest::getName() const noexcept
    {
        return _name;
    }

    const std::string& ModManifest::getVersion() const noexcept
    {
        return _version;
    }

    const std::string& ModManifest::getMinLoaderVersion() const noexcept
    {
        return _minLoaderVersion;
    }

    const std::string& ModManifest::getMaxLoaderVersion() const noexcept
    {
        return _maxLoaderVersion;
    }

    const std::string& ModManifest::getEntry() const noexcept
    {
        return _entry;
    }

    const std::string& ModManifest::getLicense() const noexcept
    {
        return _license;
    }

    const std::vector<std::string>& ModManifest::getAuthors() const noexcept
    {
        return _authors;
    }

    const std::optional<SemVer>& ModManifest::getSemVer() const noexcept
    {
        return _semver;
    }

    const std::vector<ManifestDependency>& ModManifest::getDependencies() const noexcept
    {
        return _dependencies;
    }

    const std::vector<ManifestDependency>& ModManifest::getOptionalDependencies() const noexcept
    {
        return _optionalDependencies;
    }

    const std::vector<std::string>& ModManifest::getLoadAfter() const noexcept
    {
        return _loadAfter;
    }

    const std::vector<std::string>& ModManifest::getLoadBefore() const noexcept
    {
        return _loadBefore;
    }

    const std::vector<std::string>& ModManifest::getConflicts() const noexcept
    {
        return _conflicts;
    }

    const std::vector<std::string>& ModManifest::getProvides() const noexcept
    {
        return _provides;
    }

    const std::vector<std::string>& ModManifest::getIgnoredKeys() const noexcept
    {
        return _ignoredKeys;
    }

    bool ModManifest::isCompatible() const
    {
        // The lower bound still goes through crabe::version::isCompatible so a
        // v0 manifest gets bit-for-bit the answer it got before this rewrite.
        if (!_minLoaderVersion.empty() && !crabe::version::isCompatible(_minLoaderVersion))
            return false;

        if (_maxLoaderRange.has_value()) {
            const SemVer running(crabe::version::Major, crabe::version::Minor,
                                 crabe::version::Patch);
            if (_maxLoaderRange->exceededBy(running))
                return false;
        }
        return true;
    }

} // namespace crabe::domain
