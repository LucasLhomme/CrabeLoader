/*
** CrabeLoader
** File description:
** Declares crabe.toml: general settings, display, keybinds, multiplayer, profiles, quarantine.
** A malformed file is reported and fallen back from, never overwritten; comments survive.
** Holds no Win32 type; key names stay strings, resolved by shared/keybind_names.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_DOMAIN_CONFIG_HPP_
#define CRABELOADER_DOMAIN_CONFIG_HPP_

// crabe.toml -- the one persisted settings file (T11).
//
// Three things used to be true that are not any more: the only setting ever
// saved was the window mode, in a bare `crabe_window_mode.cfg` written next
// to the DLL; keybinds were hardcoded in application/loader.cpp; and every
// mod in mods/ always loaded, with no way to turn one off short of deleting
// it. This file is what replaced all three.
//
// Mirrors mod_manifest.hpp's shape on purpose (ManifestState/ManifestError/
// ManifestDiagnostic -> ConfigState/ConfigError/ConfigDiagnostic): a config
// file has the same three-outcome story a manifest does (present-and-valid,
// absent, present-but-broken), and the same rule applies to the broken case
// -- log where the defect is and never touch the user's file. Parsing itself
// uses toml++ (include/third_party/toml.hpp) exactly the way mod_manifest.cpp
// uses nlohmann/json.

#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace crabe::domain {

    enum class ConfigWindowMode : std::uint8_t {
        Borderless,
        Windowed,
    };

    enum class ConfigError : std::uint8_t {
        FileNotFound,   // the only non-defect: state is Absent
        FileUnreadable, // exists, but could not be opened or read
        InvalidToml,    // the parser refused it
    };

    [[nodiscard]] std::string_view describe(ConfigError error) noexcept;

    // Same contract as ManifestDiagnostic: everything needed to fix the file
    // without a debugger. line/column are 1-based and zero when the parser
    // never got far enough to report a position.
    struct ConfigDiagnostic {
        ConfigError code{ConfigError::FileNotFound};
        std::string message;
        std::filesystem::path file;
        std::size_t line{0};
        std::size_t column{0};

        [[nodiscard]] bool isAbsent() const noexcept
        {
            return code == ConfigError::FileNotFound;
        }

        // One log-ready line: "<file>:<line>:<col>: <message>".
        [[nodiscard]] std::string what() const;
    };

    // One entry of the [quarantine] section: a callback that has failed at
    // least once. `callback` is whatever src/api/02c_quarantine.lua named it
    // ("onUpdate", "onTick:function: 0x...", ...); `modId` is the same string
    // Crabe.Mod._registered entries and Game.onTick owners are keyed on --
    // not necessarily the dependency resolver's ModId, since an unregistered
    // or ad hoc mod name can end up here too.
    struct QuarantinedCallback {
        std::string modId;
        std::string callback;
        unsigned int consecutiveFailures{0};
        bool disabled{false};
        std::string lastError;

        [[nodiscard]] bool operator==(const QuarantinedCallback&) const = default;
    };

    // A point-in-time view of Crabe.Quarantine's Lua-side state (see
    // src/api/02c_quarantine.lua), as reported by Crabe.Quarantine.report()
    // and polled from application::Loader::runTicks the same way
    // drainLuaOutput polls Crabe.flush(). Written into crabe.toml's
    // [quarantine] section and read by the ImGui overlay.
    struct QuarantineSnapshot {
        std::vector<std::string> disabledMods;
        std::vector<QuarantinedCallback> callbacks;

        [[nodiscard]] bool empty() const noexcept
        {
            return disabledMods.empty() && callbacks.empty();
        }

        [[nodiscard]] bool operator==(const QuarantineSnapshot&) const = default;

        // Parses the tab-separated wire format Crabe.Quarantine.report()
        // returns:
        //   MOD\t<id1>\t<id2>\t...
        //   CB\t<modId>\t<callback>\t<consecutiveFailures>\t<0|1>\t<lastError>
        // One line per record; either line kind may repeat or be absent.
        // Never throws: a line that does not fit the shape is skipped rather
        // than failing the whole report, since a partial report is far more
        // useful than none while diagnosing a mod.
        [[nodiscard]] static QuarantineSnapshot parseReport(std::string_view report);
    };

    // A parsed crabe.toml, plus the legacy-migration and default-writing
    // policy around it. Layout mirrors ModManifest: parse() is the pure,
    // testable half (no disk I/O, takes text); load() is the impure half a
    // real caller uses.
    class Config {
    public:
        Config();

        // The single Config instance the running loader uses once
        // initializeActive() has loaded it -- a thin convenience over
        // threading a Config through every layer that needs one
        // (application::Loader for keybinds/logLevel/multiplayer,
        // presentation::RenderHook for windowMode, domain::ModManager for
        // profile filtering, the overlay for the quarantine snapshot).
        // Mirrors ModManager::get(); load()/parse() below stay the pure,
        // directly-testable half that never touches it.
        [[nodiscard]] static Config& active();
        static void initializeActive(const std::filesystem::path& gameRoot);

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

        [[nodiscard]] bool isValid() const noexcept { return !_diagnostic.has_value(); }
        [[nodiscard]] const std::optional<ConfigDiagnostic>& diagnostic() const noexcept { return _diagnostic; }
        [[nodiscard]] const std::vector<std::string>& getIgnoredKeys() const noexcept { return _ignoredKeys; }

        [[nodiscard]] const std::string& language() const noexcept { return _language; }
        [[nodiscard]] const std::string& logLevel() const noexcept { return _logLevel; }
        [[nodiscard]] ConfigWindowMode windowMode() const noexcept { return _windowMode; }
        [[nodiscard]] bool multiplayerEnabled() const noexcept { return _multiplayerEnabled; }

        // Raw key names as written in [keybinds] ("F4", "Insert"), not yet
        // resolved to a virtual-key code -- that needs <windows.h>, which
        // this pure domain type does not pull in. See
        // shared::parseVirtualKeyName, used by application::Loader.
        [[nodiscard]] const std::string& hotReloadKeybind() const noexcept { return _hotReloadKeybind; }
        [[nodiscard]] const std::string& devOverlayKeybind() const noexcept { return _devOverlayKeybind; }

        [[nodiscard]] const std::string& activeProfileName() const noexcept { return _activeProfile; }

        // False when [general].profile names a profile with no matching
        // [profiles.<name>] table; enabledPatterns() then falls back to
        // every mod being enabled, but the caller is the one that logs it
        // (mirrors ModManifest leaving what to do with a v0 id to the
        // caller).
        [[nodiscard]] bool activeProfileFound() const noexcept { return _activeProfileFound; }

        // Patterns of the active profile: "*" (match everything) or an exact
        // mod id, including a synthesised "local.<folder>" one -- the same
        // string ModManager::discoverAndLoadMods would assign, so a profile
        // can name a mod that ships no "id" of its own.
        [[nodiscard]] const std::vector<std::string>& enabledPatterns() const noexcept { return _activeProfilePatterns; }

        [[nodiscard]] bool isModEnabled(std::string_view modId) const noexcept;

        // Persists a runtime window-mode change into the file's [display]
        // section, touching nothing else in it (see spliceTomlSection). A
        // no-op on the in-memory value's caller-visible behaviour either
        // way; only the on-disk write can fail, silently, if the section
        // cannot be spliced (e.g. the file was deleted after load()).
        void setWindowMode(const std::filesystem::path& gameRoot, ConfigWindowMode mode);

        // Updates the [quarantine] section, in memory and (if it changed)
        // on disk, splicing just that section (see spliceTomlSection).
        void updateQuarantineSnapshot(const std::filesystem::path& gameRoot, QuarantineSnapshot snapshot);
        [[nodiscard]] const QuarantineSnapshot& quarantineSnapshot() const noexcept { return _quarantine; }

    private:
        std::optional<ConfigDiagnostic> _diagnostic;
        std::vector<std::string> _ignoredKeys;

        std::string _language{"en"};
        std::string _logLevel{"info"};
        ConfigWindowMode _windowMode{ConfigWindowMode::Borderless};
        bool _multiplayerEnabled{false};
        std::string _hotReloadKeybind{"F4"};
        std::string _devOverlayKeybind{"Insert"};

        std::string _activeProfile{"default"};
        bool _activeProfileFound{true};
        std::vector<std::string> _activeProfilePatterns{"*"};
        std::unordered_map<std::string, std::vector<std::string>> _profiles;

        QuarantineSnapshot _quarantine;
    };

    // Splices the top-level TOML section named `sectionName` (and every
    // dotted sub-table under it, e.g. "[quarantine.x]" or "[[quarantine.x]]")
    // out of `original` and replaces it with `newSectionText`, which must
    // itself start with "[<sectionName>]" and end with a newline. If the
    // section is not present, `newSectionText` is appended at the end,
    // preceded by exactly one blank line when `original` is non-empty.
    //
    // Every byte of `original` outside that section's extent is reproduced
    // exactly -- comments and formatting included -- which is what lets
    // Config own the [quarantine] and [display] sections without a full
    // parse/reserialize round trip destroying whatever the user wrote
    // anywhere else in the file.
    [[nodiscard]] std::string spliceTomlSection(std::string_view original,
                                                std::string_view sectionName,
                                                std::string_view newSectionText);

    // The fully-commented file written when crabe.toml does not exist yet.
    // `migratedWindowMode` seeds [display].windowMode from the legacy
    // crabe_window_mode.cfg when migrating; nullopt uses the shipped default
    // (borderless).
    [[nodiscard]] std::string buildDefaultConfigText(
        std::optional<ConfigWindowMode> migratedWindowMode = std::nullopt);

    // Reads the legacy crabe_window_mode.cfg format (a single bare word,
    // "windowed" or "borderless"). Returns nullopt if the file is absent or
    // unreadable -- migration then falls back to the shipped default rather
    // than failing.
    [[nodiscard]] std::optional<ConfigWindowMode> readLegacyWindowModeFile(
        const std::filesystem::path& path);

    [[nodiscard]] std::string_view windowModeName(ConfigWindowMode mode) noexcept;

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_CONFIG_HPP_ */
