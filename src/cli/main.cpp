/*
** CrabeLoader
** File description:
** The crabe-cli entry point: argument dispatch to new-mod, validate, resolve and check-api.
** Each subcommand owns its exit codes; this file only routes and prints usage.
** Shares the loader domain code rather than reimplementing it, which is the point of the tool.
**
** Authors: @LucasLhomme
*/

// crabe-cli -- offline mod-authoring tools for CrabeLoader (T16).
//
// Links the domain layer only (manifest parsing, SemVer, the dependency
// resolver) plus the shared layer it already sits on: no Win32 API, no
// Direct3D, no Dear ImGui, no Lua runtime. See tests/resolver_purity.cmake,
// which audits every translation unit here the same way it already audits
// src/domain/dependency_resolver.cpp.

#include "cli/check_api.hpp"
#include "cli/mod_scaffold.hpp"
#include "cli/resolve_command.hpp"
#include "cli/validate_command.hpp"

#include <iostream>
#include <span>
#include <string>
#include <vector>

#ifndef CRABE_CLI_VERSION
#    define CRABE_CLI_VERSION "0.0.0"
#endif

namespace {

    void printUsage(std::ostream& out)
    {
        out << "crabe-cli " CRABE_CLI_VERSION " -- offline mod-authoring tools for CrabeLoader\n"
               "\n"
               "Usage:\n"
               "  crabe-cli new-mod <id> [--name <name>] [--dir <path>]\n"
               "  crabe-cli validate <path> [--json]\n"
               "  crabe-cli resolve <mods-dir> [--json]\n"
               "  crabe-cli check-api <path> [--api-def <file>]\n"
               "\n"
               "See docs/guides/cli.md for the full command reference and exit codes.\n";
    }

} // namespace

int main(int argc, char** argv)
{
    const std::vector<std::string> allArgs(argv + 1, argv + argc);

    if (allArgs.empty()) {
        printUsage(std::cerr);
        return 1;
    }

    const std::string& command = allArgs[0];
    if (command == "--help" || command == "-h" || command == "help") {
        printUsage(std::cout);
        return 0;
    }
    if (command == "--version") {
        std::cout << "crabe-cli " CRABE_CLI_VERSION "\n";
        return 0;
    }

    const std::span<const std::string> rest(allArgs.data() + 1, allArgs.size() - 1);

    if (command == "new-mod")
        return crabe::cli::runNewMod(rest, std::cout, std::cerr);
    if (command == "validate")
        return crabe::cli::runValidate(rest, std::cout, std::cerr);
    if (command == "resolve")
        return crabe::cli::runResolve(rest, std::cout, std::cerr);
    if (command == "check-api")
        return crabe::cli::runCheckApi(rest, std::cout, std::cerr);

    std::cerr << "crabe-cli: unknown command '" << command << "'\n\n";
    printUsage(std::cerr);
    return 1;
}
