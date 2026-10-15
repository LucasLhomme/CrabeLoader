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
        std::string version;
    };

    struct RejectedEntry {
        std::string id;
        std::string folder;
        std::string version;
        crabe::domain::Rejection reason{crabe::domain::Rejection::InvalidManifest};
        std::string detail;
    };

    struct ResolveReport {
        std::filesystem::path modsDir;
        std::string loaderVersion;

        std::vector<ResolvedEntry> loadOrder;
        std::vector<RejectedEntry> rejected;
        std::vector<std::string> notes;

        // Returns 0 when no mod was rejected, or 1 otherwise.
        [[nodiscard]] int exitCode() const noexcept;
    };

    // Resolves dependencies across all mods in the specified directory.
    [[nodiscard]] std::expected<ResolveReport, std::string> resolveModsDir(
        const std::filesystem::path& modsDir);

    // Formats and prints the resolve report to the given stream.
    void printHuman(const ResolveReport& report, std::ostream& out);

    // Parses command line arguments and runs the mod dependency resolution command.
    int runResolve(std::span<const std::string> args, std::ostream& out, std::ostream& err);

} // namespace crabe::cli

#endif /* !CRABELOADER_CLI_RESOLVE_COMMAND_HPP_ */
