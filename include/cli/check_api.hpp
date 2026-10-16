/*
** CrabeLoader
** File description:
** Declares check-api: flags Lua calls close enough to a documented API name to be a typo.
** The documented surface is a subset, not an inventory, so every finding is a warning.
** Parses no Lua -- call sites come from the lexical scanner in cli/lua_call_scan.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_CLI_CHECK_API_HPP_
#define CRABELOADER_CLI_CHECK_API_HPP_

#include <cstddef>
#include <expected>
#include <filesystem>
#include <ostream>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace crabe::cli {

    struct ApiSurface {
        std::set<std::string> callables;
        std::set<std::string> roots;
    };

    // Parses an EmmyLua API definitions file in crabe_api.def.lua's shape.
    [[nodiscard]] std::expected<ApiSurface, std::string> parseApiSurface(
        std::string_view text, const std::filesystem::path& origin);

    struct FlaggedCall {
        std::filesystem::path file;
        std::size_t line{0};
        std::size_t column{0};
        std::string called;
        std::string suggestion;
    };

    struct CheckApiReport {
        std::filesystem::path apiDefPath;
        std::vector<std::filesystem::path> scannedFiles;
        std::vector<std::string> skipped;
        std::vector<FlaggedCall> flagged;

        // Returns 0 if clean, or 2 if any call was flagged.
        [[nodiscard]] int exitCode() const noexcept;
    };

    // Scans files for API calls and flags near-miss typos against the known surface.
    [[nodiscard]] CheckApiReport checkApi(const ApiSurface& surface,
                                          const std::vector<std::filesystem::path>& luaFiles);

    // Formats and prints the check report to the given stream.
    void printHuman(const CheckApiReport& report, std::ostream& out);

    // Parses command line arguments and runs the API check workflow.
    int runCheckApi(std::span<const std::string> args, std::ostream& out, std::ostream& err);

} // namespace crabe::cli

#endif /* !CRABELOADER_CLI_CHECK_API_HPP_ */
