/*
** CrabeLoader
** File description:
** Declares validate: checks one mod folder for defects without starting the game.
** Schema checking already happened in ModManifest::parse; this adds the on-disk checks.
** Exit codes are contractual: 0 clean, 1 defect, 2 warnings only. See docs/guides/cli.md.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_CLI_VALIDATE_COMMAND_HPP_
#define CRABELOADER_CLI_VALIDATE_COMMAND_HPP_

// `crabe-cli validate` -- checks one mod without starting the game.
//
// Most of the schema-level checking (the id pattern, SemVer fields, and that
// every dependency/loadAfter/loadBefore/conflicts/provides entry is a
// well-formed id and range) is already done by ModManifest::parse -- a
// manifest that survived parsing as Valid has already passed all of that
// (see tests/fixtures/manifests/v1_bad_*.json, which are Malformed for
// exactly these reasons). What validate adds on top is what parsing a JSON
// file in isolation cannot know: whether the entry script this manifest
// names is actually there on disk, and a few non-fatal observations
// (unknown keys, a v0 manifest's leniently-accepted non-SemVer version, and
// what id the loader will actually assign when the manifest declares none).

#include <cstddef>
#include <filesystem>
#include <ostream>
#include <span>
#include <string>
#include <vector>

namespace crabe::cli {

    enum class Severity { Error, Warning };

    struct ValidateFinding {
        Severity severity{Severity::Error};
        std::string message;
        std::size_t line{0};   // 0 when the finding has no position in mod.json
        std::size_t column{0};
    };

    struct ValidateReport {
        std::filesystem::path target;
        bool hasManifest{false};

        // The id this mod will actually load under -- its own declared id,
        // or the "local.<folder>" ModManager would synthesise for it.
        std::string id;
        bool idSynthesised{false};
        std::string version;

        std::filesystem::path entryScript;
        bool entryScriptExists{false};

        std::vector<ValidateFinding> findings;

        // 0 clean, 1 if any finding is an Error, 2 if the worst finding is a
        // Warning.
        [[nodiscard]] int exitCode() const noexcept;
    };

    // Validates a mod folder (containing mod.json, or a bare directory of
    // .lua files), a mod.json file directly, or a standalone .lua script.
    [[nodiscard]] ValidateReport validateMod(const std::filesystem::path& path);

    void printHuman(const ValidateReport& report, std::ostream& out);

    // Parses argv (`<path> [--json]`) and runs the command. Exit codes:
    // 0 clean, 1 on error (including a target path that does not exist),
    // 2 on warnings only.
    int runValidate(std::span<const std::string> args, std::ostream& out, std::ostream& err);

} // namespace crabe::cli

#endif /* !CRABELOADER_CLI_VALIDATE_COMMAND_HPP_ */
