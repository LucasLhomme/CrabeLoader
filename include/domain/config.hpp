/*
** CrabeLoader
** File description:
** Declares crabe.toml: general, display, keybinds, multiplayer, http, plugins, profiles, ...
** A malformed file is reported and fallen back from, never overwritten; comments survive.
** Holds no Win32 type; key names stay strings, resolved by shared/keybind_names.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_DOMAIN_CONFIG_HPP_
#define CRABELOADER_DOMAIN_CONFIG_HPP_

#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace crabe::domain {

    enum class ConfigWindowMode : std::uint8_t {
        Borderless,
        Windowed,
    };

    enum class ConfigError : std::uint8_t {
        FileNotFound,
        FileUnreadable,
        InvalidToml,
    };

    // Returns a human-readable description of a configuration error.
    [[nodiscard]] std::string_view describe(ConfigError error) noexcept;

    struct ConfigDiagnostic {
        ConfigError code{ConfigError::FileNotFound};
        std::string message;
        std::filesystem::path file;
        std::size_t line{0};
        std::size_t column{0};

        // Returns true if the configuration file was absent.
        [[nodiscard]] bool isAbsent() const noexcept
        {
            return code == ConfigError::FileNotFound;
        }

        // Returns a log-formatted message string.
        [[nodiscard]] std::string what() const;
    };

    struct QuarantinedCallback {
        std::string modId;
        std::string callback;
        unsigned int consecutiveFailures{0};
        bool disabled{false};
        std::string lastError;

        [[nodiscard]] bool operator==(const QuarantinedCallback&) const = default;
    };

    struct QuarantineSnapshot {
        std::vector<std::string> disabledMods;
        std::vector<QuarantinedCallback> callbacks;

        // Returns true if no mods or callbacks are quarantined.
        [[nodiscard]] bool empty() const noexcept
        {
            return disabledMods.empty() && callbacks.empty();
        }

        [[nodiscard]] bool operator==(const QuarantineSnapshot&) const = default;

        // Parses the tab-separated quarantine report returned by Lua.
        [[nodiscard]] static QuarantineSnapshot parseReport(std::string_view report);
    };

    class Config {
    public:
        // Constructs a configuration instance with default settings.
        Config();

        // Returns the active singleton Config instance.
        [[nodiscard]] static Config& active();

        // Loads and sets the active configuration from the game root.
        static void initializeActive(const std::filesystem::path& gameRoot);

        // Parses a TOML configuration string.
        [[nodiscard]] static std::expected<Config, ConfigDiagnostic> parse(
            std::string_view text, const std::filesystem::path& origin);

        // Loads <gameRoot>/Crabe/crabe.toml.
        //
        //  * Missing: migrates <gameRoot>/crabe_window_mode.cfg if it exists
        //    (read only -- it is never deleted or rewritten), then writes a
        //    commented default file seeded with that window mode.
        //  * Malformed: logs the diagnostic (via the returned Config's own
        //    diagnostic(), which the caller logs) and keeps built-in
        //    defaults in memory. The file on disk is never touched.
        //  * Valid: parsed normally.
        [[nodiscard]] static Config load(const std::filesystem::path& gameRoot);

        // Reads <gameRoot>/Crabe/crabe.toml without writing anything: built-in
        // defaults when it is missing or malformed. For readers that run before
        // the loader thread has called load(), such as the .asi plugin loader.
        [[nodiscard]] static Config peek(const std::filesystem::path& gameRoot);

        [[nodiscard]] bool isValid() const noexcept { return !_diagnostic.has_value(); }
        [[nodiscard]] const std::optional<ConfigDiagnostic>& diagnostic() const noexcept { return _diagnostic; }
        [[nodiscard]] const std::vector<std::string>& getIgnoredKeys() const noexcept { return _ignoredKeys; }

        [[nodiscard]] const std::string& language() const noexcept { return _language; }
        [[nodiscard]] const std::string& logLevel() const noexcept { return _logLevel; }
        [[nodiscard]] ConfigWindowMode windowMode() const noexcept { return _windowMode; }
        [[nodiscard]] std::uint32_t frameLimit() const noexcept { return _frameLimit; }
        [[nodiscard]] bool multiplayerEnabled() const noexcept { return _multiplayerEnabled; }
        [[nodiscard]] bool updateCheckEnabled() const noexcept { return _updateCheckEnabled; }

        [[nodiscard]] bool asiEnabled() const noexcept { return _asiEnabled; }
        [[nodiscard]] const std::vector<std::string>& asiSkipList() const noexcept { return _asiSkip; }

        [[nodiscard]] const std::vector<std::pair<std::string, std::string>>& httpRedirects() const noexcept
        {
            return _httpRedirects;
        }

        [[nodiscard]] const std::string& hotReloadKeybind() const noexcept { return _hotReloadKeybind; }
        [[nodiscard]] const std::string& devOverlayKeybind() const noexcept { return _devOverlayKeybind; }

        [[nodiscard]] const std::string& activeProfileName() const noexcept { return _activeProfile; }
        [[nodiscard]] bool activeProfileFound() const noexcept { return _activeProfileFound; }
        [[nodiscard]] const std::vector<std::string>& enabledPatterns() const noexcept { return _activeProfilePatterns; }

        // Returns true if the given mod id is enabled under the active profile.
        [[nodiscard]] bool isModEnabled(std::string_view modId) const noexcept;

        // Persists a runtime window mode change into crabe.toml's [display] section.
        void setWindowMode(const std::filesystem::path& gameRoot, ConfigWindowMode mode);

        // Persists a frame rate cap into crabe.toml's [display] section (0 = unlimited).
        void setFrameLimit(const std::filesystem::path& gameRoot, std::uint32_t fps);

        static constexpr std::uint32_t kMinFrameLimit = 15;
        static constexpr std::uint32_t kMaxFrameLimit = 1000;

        // Clamps a frame rate value to within valid engine limits.
        [[nodiscard]] static std::uint32_t clampFrameLimit(std::int64_t fps) noexcept;

        // Updates the quarantine snapshot in memory and persists changes to disk.
        void updateQuarantineSnapshot(const std::filesystem::path& gameRoot, QuarantineSnapshot snapshot);
        [[nodiscard]] const QuarantineSnapshot& quarantineSnapshot() const noexcept { return _quarantine; }

    private:
        std::optional<ConfigDiagnostic> _diagnostic;
        std::vector<std::string> _ignoredKeys;

        std::string _language{"en"};
        std::string _logLevel{"info"};
        ConfigWindowMode _windowMode{ConfigWindowMode::Borderless};
        std::uint32_t _frameLimit{0};

        void writeDisplaySection(const std::filesystem::path& gameRoot) const;
        bool _multiplayerEnabled{false};
        bool _updateCheckEnabled{true};
        bool _asiEnabled{true};
        std::vector<std::string> _asiSkip;
        std::vector<std::pair<std::string, std::string>> _httpRedirects;
        std::string _hotReloadKeybind{"F4"};
        std::string _devOverlayKeybind{"Insert"};

        std::string _activeProfile{"default"};
        bool _activeProfileFound{true};
        std::vector<std::string> _activeProfilePatterns{"*"};
        std::unordered_map<std::string, std::vector<std::string>> _profiles;

        QuarantineSnapshot _quarantine;
    };

    // Replaces or appends a named TOML section within original text while preserving comments.
    [[nodiscard]] std::string spliceTomlSection(std::string_view original,
                                                std::string_view sectionName,
                                                std::string_view newSectionText);

    // Generates default crabe.toml content, optionally seeded with a migrated window mode.
    [[nodiscard]] std::string buildDefaultConfigText(
        std::optional<ConfigWindowMode> migratedWindowMode = std::nullopt);

    // Reads a legacy crabe_window_mode.cfg file if present.
    [[nodiscard]] std::optional<ConfigWindowMode> readLegacyWindowModeFile(
        const std::filesystem::path& path);

    // Returns the string representation of a window mode.
    [[nodiscard]] std::string_view windowModeName(ConfigWindowMode mode) noexcept;

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_CONFIG_HPP_ */
