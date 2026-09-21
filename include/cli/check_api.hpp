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

// `crabe-cli check-api` -- flags Lua calls that resemble a documented
// Crabe/Game API function closely enough to be a probable typo, without
// pretending to know everything that is or is not a real member of those
// tables.
//
// crabe_api.def.lua documents CrabeLoader's "public V2 platform surface"
// (docs/modding.md, Layer 3). It is not, and cannot safely be treated as,
// an exhaustive list of every real member of Crabe or Game: Crabe.Registry
// and Crabe.Scheduler, for instance, are real (src/api/03_lifecycle.lua
// calls both) and are not declared anywhere in that file. Flagging "any
// direct child of Crabe this file does not mention" would therefore flag
// real code on this project's own shipped API module -- exactly the
// confident false positive that gets a linter switched off.
//
// So this tool does something narrower and safer: it looks for calls that
// are a *near miss* of a name the file does document (small edit distance on
// the full dotted path), and only ever suggests a fix when one exists. A
// legitimate, merely-undocumented member is never close enough to anything
// in the known set to trip that threshold, so it is silently left alone.
// See check_api.cpp for the exact rule and lua_call_scan.hpp for what the
// underlying scan can and cannot see (dynamic calls, aliases, shadowing).

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

    // The set of API functions crabe_api.def.lua declares (every
    // `function A.B.C(...) end`), plus the root table names (`Crabe`, `Game`)
    // that own at least one of them -- calls rooted anywhere else are never
    // checked, because this tool has no catalogue of the ~900 engine
    // natives documented separately in docs/nativedb.md and cannot tell a
    // real one from a typo.
    struct ApiSurface {
        std::set<std::string> callables;
        std::set<std::string> roots;
    };

    // Parses an EmmyLua API definitions file in crabe_api.def.lua's own
    // shape. `origin` only labels the error when the file declares no
    // functions at all.
    [[nodiscard]] std::expected<ApiSurface, std::string> parseApiSurface(
        std::string_view text, const std::filesystem::path& origin);

    struct FlaggedCall {
        std::filesystem::path file;
        std::size_t line{0};
        std::size_t column{0};
        std::string called;
        std::string suggestion; // the near-miss it was flagged against
    };

    struct CheckApiReport {
        std::filesystem::path apiDefPath;
        std::vector<std::filesystem::path> scannedFiles;

        // Human-readable notes on what was *not* checked and why: a root
        // shadowed by a local of the same name, or a file that could not be
        // read. Never a reason to flag anything -- only to stay silent.
        std::vector<std::string> skipped;

        std::vector<FlaggedCall> flagged;

        // 0 clean, 2 if anything was flagged. check-api's findings are a
        // heuristic near-miss, not a certainty, so they sit at the same
        // "advisory" tier `validate`'s warnings-only case does -- never 1,
        // which run() reserves for the tool itself failing to do its job.
        [[nodiscard]] int exitCode() const noexcept;
    };

    // Scans every file in `luaFiles` for calls rooted at a name in
    // `surface.roots`, flagging the ones that are not an exact match for
    // anything in `surface.callables` but are a small edit distance away
    // from one.
    [[nodiscard]] CheckApiReport checkApi(const ApiSurface& surface,
                                          const std::vector<std::filesystem::path>& luaFiles);

    void printHuman(const CheckApiReport& report, std::ostream& out);

    // Parses argv (a target .lua file or directory, plus an optional
    // `--api-def <path>` override of the embedded copy of
    // crabe_api.def.lua) and runs the command.
    //
    // Exit codes: 0 clean; 1 the tool itself could not run (bad target path,
    // unreadable or empty-looking API definitions file); 2 at least one call
    // was flagged.
    int runCheckApi(std::span<const std::string> args, std::ostream& out, std::ostream& err);

} // namespace crabe::cli

#endif /* !CRABELOADER_CLI_CHECK_API_HPP_ */
