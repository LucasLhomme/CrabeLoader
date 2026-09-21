/*
** CrabeLoader
** File description:
** Implements check-api: edit distance against the documented surface, plus shadowing checks.
** A near miss is reported as a warning, because the documented surface is not exhaustive.
** Finds no call sites itself -- the lexical scan is src/cli/lua_call_scan.cpp.
**
** Authors: @LucasLhomme
*/

#include "cli/check_api.hpp"

#include "cli/embedded_api_def.hpp"
#include "cli/lua_call_scan.hpp"

#include <format>
#include <fstream>
#include <iterator>
#include <regex>
#include <sstream>

namespace crabe::cli {

    namespace {

        [[nodiscard]] std::string rootOf(const std::string& dottedPath)
        {
            const std::size_t dot = dottedPath.find('.');
            return (dot == std::string::npos) ? dottedPath : dottedPath.substr(0, dot);
        }

        [[nodiscard]] std::string_view lastSegment(std::string_view dottedPath)
        {
            const std::size_t dot = dottedPath.rfind('.');
            return (dot == std::string_view::npos) ? dottedPath : dottedPath.substr(dot + 1);
        }

        [[nodiscard]] std::expected<std::string, std::string> readFile(const std::filesystem::path& path)
        {
            std::ifstream in(path, std::ios::binary);
            if (!in)
                return std::unexpected(std::format("{}: could not be opened", path.string()));
            std::ostringstream buffer;
            buffer << in.rdbuf();
            return buffer.str();
        }

    } // namespace

    std::expected<ApiSurface, std::string> parseApiSurface(std::string_view text,
                                                            const std::filesystem::path& origin)
    {
        ApiSurface surface;

        static const std::regex functionDecl(
            R"(^function\s+([A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)*)\s*\()");

        std::size_t lineStart = 0;
        while (lineStart <= text.size()) {
            const std::size_t lineEnd = text.find('\n', lineStart);
            const std::size_t stop = (lineEnd == std::string_view::npos) ? text.size() : lineEnd;
            std::string_view line = text.substr(lineStart, stop - lineStart);
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t'))
                line.remove_prefix(1);

            if (line.size() >= 9 && line.substr(0, 9) == "function ") {
                const std::string lineStr(line);
                std::smatch match;
                if (std::regex_search(lineStr, match, functionDecl)) {
                    std::string path = match[1].str();
                    const std::string root = rootOf(path);
                    if (root != path) // only multi-segment paths name a root
                        surface.roots.insert(root);
                    surface.callables.insert(std::move(path));
                }
            }

            if (lineEnd == std::string_view::npos)
                break;
            lineStart = lineEnd + 1;
        }

        if (surface.callables.empty()) {
            return std::unexpected(std::format(
                "{}: found no `function A.B(...)` declarations; is this really an EmmyLua API "
                "definitions file in crabe_api.def.lua's shape?",
                origin.string()));
        }
        return surface;
    }

    CheckApiReport checkApi(const ApiSurface& surface, const std::vector<std::filesystem::path>& luaFiles)
    {
        CheckApiReport report;

        for (const std::filesystem::path& file : luaFiles) {
            report.scannedFiles.push_back(file);

            const auto source = readFile(file);
            if (!source.has_value()) {
                report.skipped.push_back(source.error());
                continue;
            }

            std::set<std::string> rootsToCheck;
            for (const std::string& root : surface.roots) {
                if (hasShadowedRoot(*source, root)) {
                    report.skipped.push_back(std::format(
                        "{}: '{}' is shadowed by a local variable or parameter somewhere in this "
                        "file, so its calls were not checked",
                        file.string(), root));
                } else {
                    rootsToCheck.insert(root);
                }
            }
            if (rootsToCheck.empty())
                continue;

            for (const CallSite& site : findCallSites(*source, rootsToCheck)) {
                if (surface.callables.contains(site.path))
                    continue; // exact match: known good

                const std::string siteRoot = rootOf(site.path);
                const std::size_t maxAllowed = lastSegment(site.path).size() <= 3 ? 1 : 2;

                std::string best;
                std::size_t bestDistance = std::string::npos;
                for (const std::string& candidate : surface.callables) {
                    if (rootOf(candidate) != siteRoot)
                        continue; // never suggest across roots

                    const std::size_t distance = editDistance(site.path, candidate);
                    if (distance < bestDistance) {
                        bestDistance = distance;
                        best = candidate;
                    }
                }

                // A near-miss, not an exhaustive membership check -- see
                // check_api.hpp for why crabe_api.def.lua cannot be treated
                // as a complete listing of everything under Crabe/Game.
                if (bestDistance != std::string::npos && bestDistance > 0 && bestDistance <= maxAllowed)
                    report.flagged.push_back(FlaggedCall{ file, site.line, site.column, site.path, best });
            }
        }

        return report;
    }

    int CheckApiReport::exitCode() const noexcept
    {
        return flagged.empty() ? 0 : 2;
    }

    void printHuman(const CheckApiReport& report, std::ostream& out)
    {
        out << "check-api: " << report.scannedFiles.size() << " file(s) scanned against "
            << report.apiDefPath.string() << "\n";

        for (const std::string& note : report.skipped)
            out << "  (skipped) " << note << "\n";

        if (report.flagged.empty()) {
            out << "No calls resembling a documented Crabe/Game function closely enough to flag.\n";
            return;
        }

        out << report.flagged.size() << " possible typo(s):\n";
        for (const FlaggedCall& call : report.flagged) {
            out << "  " << call.file.string() << ":" << call.line << ":" << call.column << ": `"
                << call.called << "` is not declared in crabe_api.def.lua; did you mean `"
                << call.suggestion << "`?\n";
        }
    }

    namespace {

        [[nodiscard]] std::vector<std::filesystem::path> collectLuaFiles(const std::filesystem::path& target)
        {
            std::vector<std::filesystem::path> files;
            std::error_code ec;
            if (std::filesystem::is_regular_file(target, ec)) {
                files.push_back(target);
                return files;
            }
            for (const auto& entry : std::filesystem::recursive_directory_iterator(
                     target, std::filesystem::directory_options::skip_permission_denied, ec)) {
                if (entry.is_regular_file() && entry.path().extension() == ".lua")
                    files.push_back(entry.path());
            }
            return files;
        }

    } // namespace

    int runCheckApi(std::span<const std::string> args, std::ostream& out, std::ostream& err)
    {
        std::filesystem::path target;
        std::filesystem::path apiDefOverride;

        for (std::size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "--api-def" && i + 1 < args.size()) {
                apiDefOverride = args[++i];
            } else if (target.empty()) {
                target = args[i];
            } else {
                err << "check-api: unexpected argument '" << args[i] << "'\n";
                return 1;
            }
        }

        if (target.empty()) {
            err << "check-api: usage: crabe-cli check-api <path> [--api-def <file>]\n";
            return 1;
        }

        std::error_code ec;
        if (!std::filesystem::exists(target, ec)) {
            err << "check-api: '" << target.string() << "' does not exist\n";
            return 1;
        }

        std::string apiDefText;
        std::filesystem::path apiDefPath;
        if (!apiDefOverride.empty()) {
            const auto text = readFile(apiDefOverride);
            if (!text.has_value()) {
                err << "check-api: " << text.error() << "\n";
                return 1;
            }
            apiDefText = *text;
            apiDefPath = apiDefOverride;
        } else {
            apiDefText = std::string(kEmbeddedApiDef);
            apiDefPath = "<embedded copy of docs/crabe_api.def.lua>";
        }

        const auto surface = parseApiSurface(apiDefText, apiDefPath);
        if (!surface.has_value()) {
            err << "check-api: " << surface.error() << "\n";
            return 1;
        }

        const std::vector<std::filesystem::path> luaFiles = collectLuaFiles(target);
        if (luaFiles.empty()) {
            err << "check-api: no .lua files found under '" << target.string() << "'\n";
            return 1;
        }

        CheckApiReport report = checkApi(*surface, luaFiles);
        report.apiDefPath = apiDefPath;
        printHuman(report, out);
        return report.exitCode();
    }

} // namespace crabe::cli
