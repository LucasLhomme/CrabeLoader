/*
** CrabeLoader
** File description:
** Parses crabe.toml and writes back single sections without disturbing the rest of the file.
** Unrecognised values are reported, never guessed at, and a malformed file is never overwritten.
** Resolves no key name to a virtual-key code; that is src/shared/keybind_names.cpp.
**
** Authors: @LucasLhomme
*/

#include "domain/config.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <span>
#include <utility>

#include "third_party/toml.hpp"

namespace crabe::domain {

    namespace {

        constexpr std::array<std::string_view, 3> kKnownGeneralKeys = {"language", "logLevel", "profile"};
        constexpr std::array<std::string_view, 1> kKnownDisplayKeys = {"windowMode"};
        constexpr std::array<std::string_view, 2> kKnownKeybindKeys = {"hotReload", "devOverlay"};
        constexpr std::array<std::string_view, 1> kKnownMultiplayerKeys = {"enabled"};
        constexpr std::array<std::string_view, 6> kKnownTopLevelKeys = {
            "general", "display", "keybinds", "multiplayer", "profiles", "quarantine",
        };

        [[nodiscard]] ConfigDiagnostic makeDiagnostic(ConfigError code, const std::filesystem::path& file,
                                                      std::string message)
        {
            ConfigDiagnostic diagnostic;
            diagnostic.code = code;
            diagnostic.file = file;
            diagnostic.message = std::move(message);
            return diagnostic;
        }

        // Exact-match on purpose, both here and in readLegacyWindowModeFile:
        // the values this loader ever writes are exactly "windowed" and
        // "borderless", and a config that says anything else is a value to
        // report as unrecognised, not a case to guess at.
        [[nodiscard]] std::optional<ConfigWindowMode> parseWindowMode(std::string_view text) noexcept
        {
            if (text == "windowed")
                return ConfigWindowMode::Windowed;
            if (text == "borderless")
                return ConfigWindowMode::Borderless;
            return std::nullopt;
        }

        void collectIgnored(const toml::table& table, std::span<const std::string_view> known,
                            std::string_view prefix, std::vector<std::string>& out)
        {
            for (auto&& entry : table) {
                const std::string_view key = entry.first.str();
                if (std::ranges::find(known, key) == known.end())
                    out.push_back(prefix.empty() ? std::string(key) : std::format("{}.{}", prefix, key));
            }
        }

        [[nodiscard]] std::string escapeTomlBasicString(std::string_view text)
        {
            std::string out;
            out.reserve(text.size());
            for (const char c : text) {
                switch (c) {
                case '\\': out += "\\\\"; break;
                case '"':  out += "\\\""; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:   out += c; break;
                }
            }
            return out;
        }

        [[nodiscard]] std::string formatQuarantineSection(const QuarantineSnapshot& snapshot)
        {
            std::string text = "[quarantine]\n"
                               "# Written by CrabeLoader; do not hand-edit. Rewritten whenever a mod\n"
                               "# callback trips the thresholds in src/api/02c_quarantine.lua.\n";

            if (!snapshot.disabledMods.empty()) {
                text += "disabledMods = [";
                for (std::size_t i = 0; i < snapshot.disabledMods.size(); ++i) {
                    if (i != 0)
                        text += ", ";
                    text += std::format("\"{}\"", escapeTomlBasicString(snapshot.disabledMods[i]));
                }
                text += "]\n";
            }

            for (const QuarantinedCallback& callback : snapshot.callbacks) {
                text += "\n[[quarantine.callbacks]]\n";
                text += std::format("mod = \"{}\"\n", escapeTomlBasicString(callback.modId));
                text += std::format("callback = \"{}\"\n", escapeTomlBasicString(callback.callback));
                text += std::format("consecutiveFailures = {}\n", callback.consecutiveFailures);
                text += std::format("disabled = {}\n", callback.disabled ? "true" : "false");
                text += std::format("lastError = \"{}\"\n", escapeTomlBasicString(callback.lastError));
            }

            return text;
        }

        [[nodiscard]] std::filesystem::path configPathFor(const std::filesystem::path& gameRoot)
        {
            return gameRoot / "Crabe" / "crabe.toml";
        }

        [[nodiscard]] std::optional<std::string> readWholeFile(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open())
                return std::nullopt;
            return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        }

        bool writeWholeFile(const std::filesystem::path& path, std::string_view text)
        {
            std::ofstream file(path, std::ios::trunc | std::ios::binary);
            if (!file.is_open())
                return false;
            file << text;
            return static_cast<bool>(file);
        }

        // True when `line` opens `sectionName` itself or a dotted child of it
        // -- "[quarantine]", "[quarantine.x]", "[[quarantine.x]]" -- so the
        // extent-finder in spliceTomlSection knows a header belongs to the
        // section it is replacing rather than ending it. Exact-prefix plus a
        // boundary character so "[quarantine2]" is never mistaken for a
        // child of "[quarantine]".
        [[nodiscard]] bool opensSectionOrChild(std::string_view line, std::string_view sectionName) noexcept
        {
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t'))
                line.remove_prefix(1);

            if (line.starts_with("[["))
                line.remove_prefix(2);
            else if (line.starts_with("["))
                line.remove_prefix(1);
            else
                return false;

            if (!line.starts_with(sectionName))
                return false;
            line.remove_prefix(sectionName.size());

            return line.starts_with("]") || line.starts_with(".");
        }

        [[nodiscard]] bool opensAnyTableHeader(std::string_view line) noexcept
        {
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t'))
                line.remove_prefix(1);
            return line.starts_with("[");
        }

    } // namespace

    std::string_view describe(ConfigError error) noexcept
    {
        switch (error) {
        case ConfigError::FileNotFound:
            return "there is no crabe.toml (defaults are used and a commented one is written)";
        case ConfigError::FileUnreadable:
            return "the file exists but could not be read";
        case ConfigError::InvalidToml:
            return "the file is not valid TOML";
        }
        return "the configuration could not be read";
    }

    std::string ConfigDiagnostic::what() const
    {
        std::string out = file.empty() ? std::string("<no file>") : file.string();
        if (line != 0)
            out += std::format(":{}:{}", line, column);
        out += ": ";
        out += message;
        return out;
    }

    std::string_view windowModeName(ConfigWindowMode mode) noexcept
    {
        return mode == ConfigWindowMode::Windowed ? "windowed" : "borderless";
    }

    QuarantineSnapshot QuarantineSnapshot::parseReport(std::string_view report)
    {
        QuarantineSnapshot snapshot;

        std::size_t pos = 0;
        while (pos < report.size()) {
            const std::size_t newline = report.find('\n', pos);
            const std::string_view line = (newline == std::string_view::npos)
                ? report.substr(pos)
                : report.substr(pos, newline - pos);
            pos = (newline == std::string_view::npos) ? report.size() : newline + 1;

            if (line.empty())
                continue;

            std::vector<std::string_view> fields;
            std::size_t fieldStart = 0;
            while (true) {
                const std::size_t tab = line.find('\t', fieldStart);
                fields.push_back(line.substr(fieldStart,
                    (tab == std::string_view::npos ? line.size() : tab) - fieldStart));
                if (tab == std::string_view::npos)
                    break;
                fieldStart = tab + 1;
            }

            if (fields[0] == "MOD") {
                for (std::size_t i = 1; i < fields.size(); ++i)
                    snapshot.disabledMods.emplace_back(fields[i]);
            } else if (fields[0] == "CB" && fields.size() >= 6) {
                QuarantinedCallback callback;
                callback.modId = std::string(fields[1]);
                callback.callback = std::string(fields[2]);

                for (const char c : fields[3]) {
                    if (c < '0' || c > '9') {
                        callback.consecutiveFailures = 0;
                        break;
                    }
                    callback.consecutiveFailures = callback.consecutiveFailures * 10
                        + static_cast<unsigned int>(c - '0');
                }
                callback.disabled = (fields[4] == "1");

                // The error text itself should never contain a tab (the Lua
                // side strips them), but if it somehow did, rejoin every
                // field past the fifth rather than silently truncating it.
                std::string lastError(fields[5]);
                for (std::size_t i = 6; i < fields.size(); ++i) {
                    lastError += '\t';
                    lastError += fields[i];
                }
                callback.lastError = std::move(lastError);

                snapshot.callbacks.push_back(std::move(callback));
            }
            // Any other first field is an unrecognised line kind, skipped
            // per this function's contract: a partial report beats none.
        }

        return snapshot;
    }

    Config::Config() = default;

    Config& Config::active()
    {
        static Config instance;
        return instance;
    }

    void Config::initializeActive(const std::filesystem::path& gameRoot)
    {
        active() = load(gameRoot);
    }

    std::expected<Config, ConfigDiagnostic> Config::parse(std::string_view text,
                                                           const std::filesystem::path& origin)
    {
        // An empty file is not a defect the way an empty mod.json is -- it
        // simply carries no overrides, so it parses to built-in defaults
        // silently.
        if (text.find_first_not_of(" \t\r\n") == std::string_view::npos)
            return Config{};

        toml::table root;
        try {
            root = toml::parse(text, origin.generic_string());
        } catch (const toml::parse_error& error) {
            ConfigDiagnostic diagnostic = makeDiagnostic(
                ConfigError::InvalidToml, origin,
                std::format("not valid TOML: {}", error.description()));
            diagnostic.line = error.source().begin.line;
            diagnostic.column = error.source().begin.column;
            return std::unexpected(std::move(diagnostic));
        }

        Config config;

        if (const auto* general = root["general"].as_table()) {
            if (auto value = (*general)["language"].value<std::string>())
                config._language = *value;
            if (auto value = (*general)["logLevel"].value<std::string>())
                config._logLevel = *value;
            if (auto value = (*general)["profile"].value<std::string>())
                config._activeProfile = *value;
            collectIgnored(*general, kKnownGeneralKeys, "general", config._ignoredKeys);
        }

        if (const auto* display = root["display"].as_table()) {
            if (auto value = (*display)["windowMode"].value<std::string>()) {
                if (auto mode = parseWindowMode(*value)) {
                    config._windowMode = *mode;
                } else {
                    // Not an unknown key -- a bad value for a known one --
                    // but surfaced the same way (the caller logs every
                    // ignoredKeys entry at debug level) so it is not silent.
                    config._ignoredKeys.push_back(std::format(
                        "display.windowMode (unrecognised value \"{}\", using \"{}\")",
                        *value, windowModeName(config._windowMode)));
                }
            }
            collectIgnored(*display, kKnownDisplayKeys, "display", config._ignoredKeys);
        }

        if (const auto* keybinds = root["keybinds"].as_table()) {
            if (auto value = (*keybinds)["hotReload"].value<std::string>())
                config._hotReloadKeybind = *value;
            if (auto value = (*keybinds)["devOverlay"].value<std::string>())
                config._devOverlayKeybind = *value;
            collectIgnored(*keybinds, kKnownKeybindKeys, "keybinds", config._ignoredKeys);
        }

        if (const auto* multiplayer = root["multiplayer"].as_table()) {
            if (auto value = (*multiplayer)["enabled"].value<bool>())
                config._multiplayerEnabled = *value;
            collectIgnored(*multiplayer, kKnownMultiplayerKeys, "multiplayer", config._ignoredKeys);
        }

        if (const auto* profiles = root["profiles"].as_table()) {
            for (auto&& entry : *profiles) {
                const auto* profileTable = entry.second.as_table();
                if (!profileTable)
                    continue; // a malformed profiles.* entry is ignored, not fatal.

                std::vector<std::string> patterns;
                if (const auto* enabled = (*profileTable)["enabled"].as_array()) {
                    // Present, even if empty: an explicit `enabled = []`
                    // means "enable nothing", and must not be upgraded to
                    // "enable everything" below.
                    for (auto&& element : *enabled) {
                        if (auto value = element.value<std::string>())
                            patterns.push_back(*value);
                    }
                } else {
                    // No "enabled" key at all: this profile constrains
                    // nothing, so everything is enabled under it.
                    patterns.push_back("*");
                }
                config._profiles.emplace(std::string(entry.first.str()), std::move(patterns));
            }
        }

        collectIgnored(root, kKnownTopLevelKeys, "", config._ignoredKeys);

        if (config._profiles.empty()) {
            config._activeProfileFound = true; // nothing to find; "*" is correct either way.
            config._activeProfilePatterns = {"*"};
        } else if (const auto it = config._profiles.find(config._activeProfile);
                   it != config._profiles.end()) {
            config._activeProfileFound = true;
            config._activeProfilePatterns = it->second;
        } else {
            config._activeProfileFound = false;
            config._activeProfilePatterns = {"*"};
        }

        // [quarantine] is loader-owned and re-derived from the running Lua
        // state, not read back from disk: whatever a previous run wrote here
        // described that run's counters, which do not exist yet in this one.

        return config;
    }

    Config Config::load(const std::filesystem::path& gameRoot)
    {
        const std::filesystem::path configDir = gameRoot / "Crabe";
        const std::filesystem::path configPath = configDir / "crabe.toml";

        std::error_code existsError;
        if (!std::filesystem::exists(configPath, existsError)) {
            const std::optional<ConfigWindowMode> migrated =
                readLegacyWindowModeFile(gameRoot / "crabe_window_mode.cfg");

            std::error_code createError;
            std::filesystem::create_directories(configDir, createError);

            const std::string text = buildDefaultConfigText(migrated);
            writeWholeFile(configPath, text);

            // Parsed from the text just decided on, rather than re-read from
            // disk: reflects it faithfully even if the write above failed
            // (e.g. a read-only game folder).
            auto parsed = parse(text, configPath);
            return parsed ? std::move(*parsed) : Config{};
        }

        const std::optional<std::string> content = readWholeFile(configPath);
        if (!content) {
            Config config;
            config._diagnostic = makeDiagnostic(ConfigError::FileUnreadable, configPath,
                "the file could not be opened for reading");
            return config;
        }

        auto parsed = parse(*content, configPath);
        if (parsed)
            return std::move(*parsed);

        // Malformed: built-in defaults in memory, diagnostic attached for the
        // caller to log, file on disk untouched.
        Config config;
        config._diagnostic = std::move(parsed.error());
        return config;
    }

    bool Config::isModEnabled(std::string_view modId) const noexcept
    {
        for (const std::string& pattern : _activeProfilePatterns) {
            if (pattern == "*" || pattern == modId)
                return true;
        }
        return false;
    }

    void Config::setWindowMode(const std::filesystem::path& gameRoot, ConfigWindowMode mode)
    {
        _windowMode = mode;

        const std::filesystem::path path = configPathFor(gameRoot);
        const std::optional<std::string> current = readWholeFile(path);
        if (!current)
            return; // Nothing to splice into; load() never ran or the file is gone.

        const std::string section = std::format(
            "[display]\nwindowMode = \"{}\"   # borderless | windowed\n", windowModeName(mode));
        writeWholeFile(path, spliceTomlSection(*current, "display", section));
    }

    void Config::updateQuarantineSnapshot(const std::filesystem::path& gameRoot, QuarantineSnapshot snapshot)
    {
        if (snapshot == _quarantine)
            return;
        _quarantine = std::move(snapshot);

        const std::filesystem::path path = configPathFor(gameRoot);
        const std::optional<std::string> current = readWholeFile(path);
        if (!current)
            return;

        writeWholeFile(path, spliceTomlSection(*current, "quarantine", formatQuarantineSection(_quarantine)));
    }

    std::string spliceTomlSection(std::string_view original, std::string_view sectionName,
                                  std::string_view newSectionText)
    {
        std::size_t sectionStart = std::string_view::npos;
        std::size_t cursor = 0;
        while (cursor < original.size()) {
            const std::size_t newline = original.find('\n', cursor);
            const std::string_view line = (newline == std::string_view::npos)
                ? original.substr(cursor)
                : original.substr(cursor, newline - cursor);
            if (opensSectionOrChild(line, sectionName)) {
                sectionStart = cursor;
                break;
            }
            if (newline == std::string_view::npos)
                break;
            cursor = newline + 1;
        }

        if (sectionStart == std::string_view::npos) {
            std::string result(original);
            if (!result.empty()) {
                if (!result.ends_with('\n'))
                    result += '\n';
                result += '\n'; // exactly one blank line separates old content from the new section.
            }
            result += newSectionText;
            return result;
        }

        // Extend to just before the next line that opens a *different*
        // top-level header (or to EOF, if this section runs to the end).
        std::size_t sectionEnd = original.size();
        {
            const std::size_t firstLineNewline = original.find('\n', sectionStart);
            cursor = (firstLineNewline == std::string_view::npos) ? original.size() : firstLineNewline + 1;
        }
        while (cursor < original.size()) {
            const std::size_t newline = original.find('\n', cursor);
            const std::string_view line = (newline == std::string_view::npos)
                ? original.substr(cursor)
                : original.substr(cursor, newline - cursor);
            if (opensAnyTableHeader(line) && !opensSectionOrChild(line, sectionName)) {
                sectionEnd = cursor;
                break;
            }
            if (newline == std::string_view::npos) {
                sectionEnd = original.size();
                break;
            }
            cursor = newline + 1;
        }

        std::string result;
        result.reserve(original.size() + newSectionText.size());
        result.append(original.substr(0, sectionStart));
        result.append(newSectionText);
        result.append(original.substr(sectionEnd));
        return result;
    }

    std::string buildDefaultConfigText(std::optional<ConfigWindowMode> migratedWindowMode)
    {
        const std::string_view windowModeValue = windowModeName(migratedWindowMode.value_or(ConfigWindowMode::Borderless));

        return std::format(
R"TOML(# CrabeLoader configuration. Written automatically the first time the loader
# runs, with every value defaulted (or migrated from crabe_window_mode.cfg, if
# that file existed next to the DLL). Edit freely: a malformed file is logged
# with its exact line and column, then ignored in favour of these defaults --
# it is never overwritten, so your edits are always safe to experiment with.

[general]
language  = "en"       # UI language (currently informational only)
logLevel  = "info"     # debug | info | warning | error
profile   = "default"  # selects one of the [profiles.*] tables below

[display]
windowMode = "{}"   # borderless | windowed

[keybinds]
hotReload  = "F4"      # reloads every mod from disk
devOverlay = "Insert"  # toggles the debug overlay / console

[multiplayer]
enabled = false

# A profile's "enabled" list decides which mods discoverAndLoadMods() even
# considers -- filtering happens before dependency resolution, so a disabled
# mod is simply not a candidate, never a rejection. "*" means every mod;
# otherwise list ids exactly as declared in mod.json, or "local.<folder-name>"
# for a mod that ships no "id" of its own.
[profiles.default]
enabled = ["*"]

[profiles.speedrun]
enabled = ["com.example.crabemenu"]

[quarantine]
# Written by CrabeLoader; do not hand-edit. Rewritten whenever a mod callback
# trips the thresholds in src/api/02c_quarantine.lua (ten consecutive
# failures disables a callback, three disabled callbacks disable the mod).
)TOML", windowModeValue);
    }

    std::optional<ConfigWindowMode> readLegacyWindowModeFile(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            return std::nullopt;

        std::string mode;
        file >> mode;

        // Matches RenderHook::loadWindowModeConfig()'s old rule byte for
        // byte (anything other than exactly "windowed" was borderless), so
        // migration cannot change anyone's active mode.
        return mode == "windowed" ? ConfigWindowMode::Windowed : ConfigWindowMode::Borderless;
    }

} // namespace crabe::domain
