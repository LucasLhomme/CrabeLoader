/*
** CrabeLoader
** File description:
** Declares resolve: prints the load order and every rejection for a mods directory.
** Must mirror ModManager::discoverAndLoadMods, or authors learn to distrust the answer.
** Holds no resolution logic; it calls the same domain/dependency_resolver.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_CLI_RESOLVE_COMMAND_HPP_
#define CRABELOADER_CLI_RESOLVE_COMMAND_HPP_

// `crabe-cli resolve` -- runs the real dependency resolver over a mods/
// directory and prints the load order and every rejection, answering "why
// isn't my mod loading?" without starting the game.
//
// This has to mirror ModManager::discoverAndLoadMods exactly, or it is worse
// than useless: a `resolve` that disagrees with the loader is a tool authors
// will learn to distrust, or worse, trust when it is wrong. Three things
// keep it in step:
//
//  * the discovery rules (which entries are candidates, which are skipped,
//    the malformed-manifest-becomes-absent fallback) are copied verbatim
//    from discoverAndLoadMods's steps 1-2, in resolve_command.cpp;
//  * id assignment (step 3) calls crabe::domain::assignModIds -- the exact
//    same function ModManager itself now calls (domain/mod_id.hpp), not a
//    reimplementation of it;
//  * resolution itself is one call to crabe::domain::resolve with the
//    loader's own compiled-in version (crabe::version::{Major,Minor,Patch}),
//    the same pure function and the same version ModManager uses.
//
// What it cannot mirror is step 5: crabe-cli never touches Lua, so it
// reports what *would* load, not whether each one's entry script actually
// runs without erroring.

#include <cstddef>
#include <expected>
#include <filesystem>
#include <ostream>
#include <span>
#include <string>
#include <vector>

#include "domain/dependency_resolver.hpp"

namespace crabe::cli {

    struct ResolvedEntry {
        std::string id;
        std::string folder;
        std::string version; // empty when the manifest declares none
    };

    struct RejectedEntry {
        std::string id;
        std::string folder;  // empty if no candidate on disk could be matched to it
        std::string version;
        crabe::domain::Rejection reason{crabe::domain::Rejection::InvalidManifest};
        std::string detail;
    };

    struct ResolveReport {
        std::filesystem::path modsDir;
        std::string loaderVersion;

        std::vector<ResolvedEntry> loadOrder;
        std::vector<RejectedEntry> rejected;

        // Malformed-manifest fallbacks and id-collision renumbering: the
        // same things ModManager logs at warning/error level, in plain
        // sentences rather than a Logger call.
        std::vector<std::string> notes;

        // 0 when nothing was rejected, 1 otherwise. There is no separate
        // "warnings only" tier here: a resolver rejection is a fact about
        // whether the mod loads, not a heuristic -- unlike check-api's.
        [[nodiscard]] int exitCode() const noexcept;
    };

    [[nodiscard]] std::expected<ResolveReport, std::string> resolveModsDir(
        const std::filesystem::path& modsDir);

    void printHuman(const ResolveReport& report, std::ostream& out);

    // Parses argv (`<mods-dir> [--json]`) and runs the command.
    //
    // Exit codes: 0 nothing rejected; 1 otherwise -- either the resolver
    // rejected at least one mod (see the report for which), or the tool
    // itself could not run (`<mods-dir>` does not exist or is not a
    // directory), disambiguated by stderr rather than by a third code.
    int runResolve(std::span<const std::string> args, std::ostream& out, std::ostream& err);

} // namespace crabe::cli

#endif /* !CRABELOADER_CLI_RESOLVE_COMMAND_HPP_ */
