/*
** CrabeLoader
** File description:
** Declares new-mod: writes out a mod folder shaped the way the loader actually accepts one.
** The manifest it emits is manifestVersion 1 with a valid reverse-DNS id, not a v0 stub.
** Checks nothing afterwards -- that is cli/validate_command.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_CLI_MOD_SCAFFOLD_HPP_
#define CRABELOADER_CLI_MOD_SCAFFOLD_HPP_

#include <expected>
#include <filesystem>
#include <ostream>
#include <span>
#include <string>
#include <vector>

namespace crabe::cli {

    struct ScaffoldOptions {
        std::string id;
        std::string name;
        std::filesystem::path dir;
    };

    // Parses command line arguments into scaffold options.
    [[nodiscard]] std::expected<ScaffoldOptions, std::string> parseNewModArgs(
        std::span<const std::string> args);

    struct ScaffoldFile {
        std::filesystem::path relativePath;
        std::string content;
    };

    // Builds all scaffold file contents in memory without touching the filesystem.
    [[nodiscard]] std::expected<std::vector<ScaffoldFile>, std::string> buildScaffold(
        const ScaffoldOptions& options);

    // Writes the scaffold files to the target directory, refusing to overwrite non-empty directories.
    [[nodiscard]] std::expected<void, std::string> writeScaffold(
        const ScaffoldOptions& options, const std::vector<ScaffoldFile>& files);

    // Runs the new-mod command, returning 0 on success or 1 on failure.
    int runNewMod(std::span<const std::string> args, std::ostream& out, std::ostream& err);

} // namespace crabe::cli

#endif /* !CRABELOADER_CLI_MOD_SCAFFOLD_HPP_ */
