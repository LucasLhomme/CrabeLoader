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
        std::size_t line{0};
        std::size_t column{0};
    };

    struct ValidateReport {
        std::filesystem::path target;
        bool hasManifest{false};
        std::string id;
        bool idSynthesised{false};
        std::string version;
        std::filesystem::path entryScript;
        bool entryScriptExists{false};
        std::vector<std::string> contentDirectories;
        bool contentOnly{false};
        std::vector<ValidateFinding> findings;

        // Returns 0 if clean, 1 on errors, or 2 on warnings only.
        [[nodiscard]] int exitCode() const noexcept;
    };

    // Validates a mod folder (containing mod.json, or a bare directory of
    // .lua files), a mod.json file directly, or a standalone .lua script.
    [[nodiscard]] ValidateReport validateMod(const std::filesystem::path& path);

    // Formats and prints the validate report to the given stream.
    void printHuman(const ValidateReport& report, std::ostream& out);

    // Parses command line arguments and runs the mod validation command.
    int runValidate(std::span<const std::string> args, std::ostream& out, std::ostream& err);

} // namespace crabe::cli

#endif /* !CRABELOADER_CLI_VALIDATE_COMMAND_HPP_ */
