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
        constexpr std::array<std::string_view, 2> kKnownDisplayKeys = {"windowMode", "frameLimit"};
        constexpr std::array<std::string_view, 2> kKnownKeybindKeys = {"hotReload", "devOverlay"};
        constexpr std::array<std::string_view, 1> kKnownMultiplayerKeys = {"enabled"};
        constexpr std::array<std::string_view, 1> kKnownUpdatesKeys = {"check"};
        constexpr std::array<std::string_view, 1> kKnownStartupKeys = {"skipLogos"};
        constexpr std::array<std::string_view, 1> kKnownHttpKeys = {"redirect"};
        constexpr std::array<std::string_view, 2> kKnownPluginsKeys = {"asi", "skip"};
        constexpr std::array<std::string_view, 10> kKnownTopLevelKeys = {
            "general", "display", "keybinds", "multiplayer", "updates", "startup", "http", "plugins", "profiles",
            "quarantine",
        };

        // Constructs a structured configuration diagnostic.
        [[nodiscard]] ConfigDiagnostic makeDiagnostic(ConfigError code, const std::filesystem::path& file,
                                                      std::string message)
        {
            ConfigDiagnostic diagnostic;
            diagnostic.code = code;
            diagnostic.file = file;
            diagnostic.message = std::move(message);
            return diagnostic;
        }

        // Parses a window mode string into the corresponding enum value.
        [[nodiscard]] std::optional<ConfigWindowMode> parseWindowMode(std::string_view text) noexcept
        {
            if (text == "windowed")
                return ConfigWindowMode::Windowed;
            if (text == "borderless")
                return ConfigWindowMode::Borderless;
            return std::nullopt;
        }

        // Collects keys from a TOML table that are unrecognized against a known list.
        void collectIgnored(const toml::table& table, std::span<const std::string_view> known,
                            std::string_view prefix, std::vector<std::string>& out)
        {
            for (auto&& entry : table) {
                const std::string_view key = entry.first.str();
                if (std::ranges::find(known, key) == known.end())
                    out.push_back(prefix.empty() ? std::string(key) : std::format("{}.{}", prefix, key));
            }
        }

        // Escapes special characters for standard TOML basic strings.
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

        // Formats the quarantine table text from the given snapshot.
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

        // Computes the expected crabe.toml file path for a given game root directory.
        [[nodiscard]] std::filesystem::path configPathFor(const std::filesystem::path& gameRoot)
        {
            return gameRoot / "Crabe" / "crabe.toml";
        }

        // Reads the entire contents of a file into a string.
        [[nodiscard]] std::optional<std::string> readWholeFile(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open())
                return std::nullopt;
            return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        }

        // Overwrites the specified file with the given text.
        bool writeWholeFile(const std::filesystem::path& path, std::string_view text)
        {
            std::ofstream file(path, std::ios::trunc | std::ios::binary);
            if (!file.is_open())
                return false;
            file << text;
            return static_cast<bool>(file);
        }

        // Checks whether a line starts the specified TOML table header or child.
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

        // Checks whether a line begins any TOML table header.
        [[nodiscard]] bool opensAnyTableHeader(std::string_view line) noexcept
        {
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t'))
                line.remove_prefix(1);
            return line.starts_with("[");
        }

    } // namespace

    // Returns a readable description of a configuration error.
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

    // Formats the diagnostic into a file:line:col: message string.
    std::string ConfigDiagnostic::what() const
    {
        std::string out = file.empty() ? std::string("<no file>") : file.string();
        if (line != 0)
            out += std::format(":{}:{}", line, column);
        out += ": ";
        out += message;
        return out;
    }

    // Converts a window mode enum value to its string representation.
    std::string_view windowModeName(ConfigWindowMode mode) noexcept
    {
        return mode == ConfigWindowMode::Windowed ? "windowed" : "borderless";
    }

    // Parses a tab-delimited quarantine report string into a structured snapshot.
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

                std::string lastError(fields[5]);
                for (std::size_t i = 6; i < fields.size(); ++i) {
                    lastError += '\t';
                    lastError += fields[i];
                }
                callback.lastError = std::move(lastError);

                snapshot.callbacks.push_back(std::move(callback));
            }
        }

        return snapshot;
    }

    Config::Config() = default;

    // Returns the active singleton Config instance.
    Config& Config::active()
    {
        static Config instance;
        return instance;
    }

    // Loads and initializes the active configuration from the game root.
    void Config::initializeActive(const std::filesystem::path& gameRoot)
    {
        active() = load(gameRoot);
    }

    // Parses configuration from TOML text, attaching diagnostics on failure.
    std::expected<Config, ConfigDiagnostic> Config::parse(std::string_view text,
                                                           const std::filesystem::path& origin)
    {
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
                    config._ignoredKeys.push_back(std::format(
                        "display.windowMode (unrecognised value \"{}\", using \"{}\")",
                        *value, windowModeName(config._windowMode)));
                }
            }
            if (auto value = (*display)["frameLimit"].value<std::int64_t>()) {
                config._frameLimit = clampFrameLimit(*value);
            } else if ((*display).contains("frameLimit")) {
                config._ignoredKeys.push_back("display.frameLimit (not an integer, using 0 = unlimited)");
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

        if (const auto* updates = root["updates"].as_table()) {
            if (auto value = (*updates)["check"].value<bool>())
                config._updateCheckEnabled = *value;
            collectIgnored(*updates, kKnownUpdatesKeys, "updates", config._ignoredKeys);
        }

        if (const auto* startup = root["startup"].as_table()) {
            if (auto value = (*startup)["skipLogos"].value<bool>())
                config._skipStartupLogos = *value;
            collectIgnored(*startup, kKnownStartupKeys, "startup", config._ignoredKeys);
        }

        if (const auto* plugins = root["plugins"].as_table()) {
            if (auto value = (*plugins)["asi"].value<bool>())
                config._asiEnabled = *value;
            if (const auto* skip = (*plugins)["skip"].as_array()) {
                for (auto&& element : *skip) {
                    if (auto name = element.value<std::string>())
                        config._asiSkip.push_back(*name);
                    else
                        config._ignoredKeys.push_back("plugins.skip (an entry that is not a string)");
                }
            }
            collectIgnored(*plugins, kKnownPluginsKeys, "plugins", config._ignoredKeys);
        }

        if (const auto* http = root["http"].as_table()) {
            if (const auto* redirect = (*http)["redirect"].as_table()) {
                for (auto&& entry : *redirect) {
                    if (auto target = entry.second.value<std::string>())
                        config._httpRedirects.emplace_back(std::string(entry.first.str()), *target);
                    else
                        config._ignoredKeys.push_back(std::format("http.redirect.{} (not a string)", entry.first.str()));
                }
            }
            collectIgnored(*http, kKnownHttpKeys, "http", config._ignoredKeys);
        }

        if (const auto* profiles = root["profiles"].as_table()) {
            for (auto&& entry : *profiles) {
                const auto* profileTable = entry.second.as_table();
                if (!profileTable)
                    continue;

                std::vector<std::string> patterns;
                if (const auto* enabled = (*profileTable)["enabled"].as_array()) {
                    for (auto&& element : *enabled) {
                        if (auto value = element.value<std::string>())
                            patterns.push_back(*value);
                    }
                } else {
                    patterns.push_back("*");
                }
                config._profiles.emplace(std::string(entry.first.str()), std::move(patterns));
            }
        }

        collectIgnored(root, kKnownTopLevelKeys, "", config._ignoredKeys);

        if (config._profiles.empty()) {
            config._activeProfileFound = true;
            config._activeProfilePatterns = {"*"};
        } else if (const auto it = config._profiles.find(config._activeProfile);
                   it != config._profiles.end()) {
            config._activeProfileFound = true;
            config._activeProfilePatterns = it->second;
        } else {
            config._activeProfileFound = false;
            config._activeProfilePatterns = {"*"};
        }

        return config;
    }

    // Reads and parses configuration from disk without creating a default file if missing.
    Config Config::peek(const std::filesystem::path& gameRoot)
    {
        const std::filesystem::path configPath = gameRoot / "Crabe" / "crabe.toml";
        const std::optional<std::string> content = readWholeFile(configPath);
        if (!content)
            return Config{};
        auto parsed = parse(*content, configPath);
        return parsed ? std::move(*parsed) : Config{};
    }

    // Loads configuration from disk, creating a default file if missing.
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

        Config config;
        config._diagnostic = std::move(parsed.error());
        return config;
    }

    // Checks whether a given mod ID is enabled under the active profile.
    bool Config::isModEnabled(std::string_view modId) const noexcept
    {
        for (const std::string& pattern : _activeProfilePatterns) {
            if (pattern == "*" || pattern == modId)
                return true;
        }
        return false;
    }

    // Updates window mode and persists the display section to crabe.toml.
    void Config::setWindowMode(const std::filesystem::path& gameRoot, ConfigWindowMode mode)
    {
        _windowMode = mode;
        writeDisplaySection(gameRoot);
    }

    // Clamps an FPS limit value to the supported range [0, 360].
    std::uint32_t Config::clampFrameLimit(std::int64_t fps) noexcept
    {
        if (fps <= 0)
            return 0;
        if (fps < kMinFrameLimit)
            return kMinFrameLimit;
        if (fps > kMaxFrameLimit)
            return kMaxFrameLimit;
        return static_cast<std::uint32_t>(fps);
    }

    // Updates frame limit and persists the display section to crabe.toml.
    void Config::setFrameLimit(const std::filesystem::path& gameRoot, std::uint32_t fps)
    {
        _frameLimit = clampFrameLimit(fps);
        writeDisplaySection(gameRoot);
    }

    // Rewrites the [display] section in crabe.toml without disturbing other sections.
    void Config::writeDisplaySection(const std::filesystem::path& gameRoot) const
    {
        const std::filesystem::path path = configPathFor(gameRoot);
        const std::optional<std::string> current = readWholeFile(path);
        if (!current)
            return;

        const std::string section = std::format(
            "[display]\nwindowMode = \"{}\"   # borderless | windowed\n"
            "frameLimit = {}   # max frames per second, 0 = unlimited\n",
            windowModeName(_windowMode), _frameLimit);
        writeWholeFile(path, spliceTomlSection(*current, "display", section));
    }

    // Updates quarantine snapshot in memory and rewrites the quarantine section in crabe.toml.
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

    // Splices or appends a named TOML section inside an existing TOML text.
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
                result += '\n';
            }
            result += newSectionText;
            return result;
        }

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

    // Builds default crabe.toml text, taking an optional migrated window mode.
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
frameLimit = 0   # max frames per second, 0 = unlimited

[keybinds]
hotReload  = "F4"      # reloads every mod from disk
devOverlay = "Insert"  # toggles the debug overlay / console

[multiplayer]
enabled = false

[updates]
check = true           # ask GitHub at launch whether a newer CrabeLoader exists; false = never connect

# What the game does while it starts.
[startup]
skipLogos = false      # true = skip the Avalanche, Disney, Lucasfilm and Marvel logo movies at launch

# .asi plugins are native DLLs loaded before the game starts, from the game
# folder, scripts/, plugins/ and the folder of every enabled mod.
[plugins]
asi  = true            # false = load no .asi plugin at all
skip = []              # file names never to load, e.g. ["example.asi"]

# Sends the game's WinHTTP connections for a server name to another server.
# Only the host and port change; "http://" also drops TLS, so a local server
# needs no certificate. One line per server name, for example:
#[http.redirect]
#"api.example.com" = "http://127.0.0.1:8080"

# A profile's "enabled" list decides which mods discoverAndLoadMods() even
# considers -- filtering happens before dependency resolution, so a disabled
# mod is simply not a candidate, never a rejection. "*" means every mod;
# otherwise list ids exactly as declared in mod.json, or "local.<folder-name>"
# for a mod that ships no "id" of its own.
[profiles.default]
enabled = ["*"]

# EXAMPLE ONLY: this profile is not used unless [general].profile is set to
# "speedrun". "com.example.crabemenu" is a placeholder id; replace it with a
# real mod id (or "local.<folder-name>") first, or no mod will load.
[profiles.speedrun]
enabled = ["com.example.crabemenu"]

[quarantine]
# Written by CrabeLoader; do not hand-edit. Rewritten whenever a mod callback
# trips the thresholds in src/api/02c_quarantine.lua (ten consecutive
# failures disables a callback, three disabled callbacks disable the mod).
)TOML", windowModeValue);
    }

    // Reads the legacy window mode configuration file if present.
    std::optional<ConfigWindowMode> readLegacyWindowModeFile(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            return std::nullopt;

        std::string mode;
        file >> mode;

        return mode == "windowed" ? ConfigWindowMode::Windowed : ConfigWindowMode::Borderless;
    }

} // namespace crabe::domain
