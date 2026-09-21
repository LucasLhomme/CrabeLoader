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

// `crabe-cli new-mod` -- scaffolds a mod folder the loader actually accepts:
// a manifestVersion 1 mod.json, a main.lua that registers a lifecycle the way
// src/api/03_lifecycle.lua and docs/guides/mods.md actually expect
// (Crabe.Mod.register with id/name/onInit/onUpdate/onDraw/onShutdown), an
// i18n/en.toml placeholder, a README, and a .gitignore.

#include <expected>
#include <filesystem>
#include <ostream>
#include <span>
#include <string>
#include <vector>

namespace crabe::cli {

    struct ScaffoldOptions {
        std::string id;
        std::string name;         // defaults to a title-cased reading of id's last segment
        std::filesystem::path dir; // defaults to "./<id's last segment>"
    };

    [[nodiscard]] std::expected<ScaffoldOptions, std::string> parseNewModArgs(
        std::span<const std::string> args);

    // One file the scaffold produces, with a path relative to the mod's own
    // root directory (e.g. "i18n/en.toml").
    struct ScaffoldFile {
        std::filesystem::path relativePath;
        std::string content;
    };

    // Builds every file's content in memory. Pure and side-effect free, so
    // the scaffold's own shape is testable (and comparable against what
    // `validate` expects) without touching a filesystem. The only failure
    // mode is `options.id` failing validateModId(), since nothing downstream
    // of that could ever produce a mod the loader accepts.
    [[nodiscard]] std::expected<std::vector<ScaffoldFile>, std::string> buildScaffold(
        const ScaffoldOptions& options);

    // Writes `files` under `options.dir`, creating it if absent. Refuses to
    // touch a directory that already exists and is not empty, so this can
    // never silently overwrite a mod that is already there.
    [[nodiscard]] std::expected<void, std::string> writeScaffold(
        const ScaffoldOptions& options, const std::vector<ScaffoldFile>& files);

    // Exit codes: 0 scaffolded; 1 on any failure (bad id, usage, an
    // already-populated target directory, or an I/O error while writing).
    int runNewMod(std::span<const std::string> args, std::ostream& out, std::ostream& err);

} // namespace crabe::cli

#endif /* !CRABELOADER_CLI_MOD_SCAFFOLD_HPP_ */
