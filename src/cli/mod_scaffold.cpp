/*
** CrabeLoader
** File description:
** Implements new-mod: writes mod.json, main.lua, an i18n placeholder, a README and .gitignore.
** Refuses a non-empty target rather than merging into it, so no existing mod is ever damaged.
** Validates nothing after writing; the caller is expected to run validate next.
**
** Authors: @LucasLhomme
*/

#include "cli/mod_scaffold.hpp"

#include "domain/mod_manifest.hpp"

#include <format>
#include <fstream>

namespace crabe::cli {

    namespace {

        [[nodiscard]] std::string_view idLastSegment(std::string_view id)
        {
            const std::size_t lastDot = id.rfind('.');
            return (lastDot == std::string_view::npos) ? id : id.substr(lastDot + 1);
        }

        // "my-mod" / "my_mod" -> "My Mod". Cosmetic only -- --name overrides
        // it outright, and mod.json stores whatever this produces verbatim.
        [[nodiscard]] std::string defaultNameFromId(std::string_view id)
        {
            const std::string_view segment = idLastSegment(id);
            std::string name;
            name.reserve(segment.size());
            bool startOfWord = true;
            for (const char character : segment) {
                if (character == '-' || character == '_') {
                    name += ' ';
                    startOfWord = true;
                    continue;
                }
                if (startOfWord && character >= 'a' && character <= 'z')
                    name += static_cast<char>(character - 'a' + 'A');
                else
                    name += character;
                startOfWord = false;
            }
            return name.empty() ? std::string(id) : name;
        }

        // Escapes backslash, double quote, newline and tab -- enough for
        // embedding arbitrary --name text inside both a JSON string
        // (mod.json) and a double-quoted Lua string literal (main.lua,
        // i18n/en.toml), which use the same escape sequences for these.
        [[nodiscard]] std::string quoteEscape(std::string_view text)
        {
            std::string out;
            out.reserve(text.size());
            for (const char character : text) {
                switch (character) {
                case '"':
                    out += "\\\"";
                    break;
                case '\\':
                    out += "\\\\";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\t':
                    out += "\\t";
                    break;
                default:
                    out += character;
                }
            }
            return out;
        }

    } // namespace

    std::expected<ScaffoldOptions, std::string> parseNewModArgs(std::span<const std::string> args)
    {
        ScaffoldOptions options;
        bool haveId = false;

        for (std::size_t i = 0; i < args.size(); ++i) {
            const std::string& arg = args[i];
            if (arg == "--name") {
                if (i + 1 >= args.size())
                    return std::unexpected(std::string("--name needs a value"));
                options.name = args[++i];
            } else if (arg == "--dir") {
                if (i + 1 >= args.size())
                    return std::unexpected(std::string("--dir needs a value"));
                options.dir = args[++i];
            } else if (!haveId) {
                options.id = arg;
                haveId = true;
            } else {
                return std::unexpected(std::format("unexpected argument '{}'", arg));
            }
        }

        if (!haveId) {
            return std::unexpected(
                std::string("usage: crabe-cli new-mod <id> [--name <name>] [--dir <path>]"));
        }

        if (const auto ok = crabe::domain::validateModId(options.id); !ok) {
            return std::unexpected(
                std::format("'{}' is not a valid mod id: {}", options.id, ok.error()));
        }

        if (options.name.empty())
            options.name = defaultNameFromId(options.id);
        if (options.dir.empty())
            options.dir = std::filesystem::path(".") / idLastSegment(options.id);

        return options;
    }

    std::expected<std::vector<ScaffoldFile>, std::string> buildScaffold(const ScaffoldOptions& options)
    {
        if (const auto ok = crabe::domain::validateModId(options.id); !ok) {
            return std::unexpected(
                std::format("'{}' is not a valid mod id: {}", options.id, ok.error()));
        }

        const std::string name = options.name.empty() ? defaultNameFromId(options.id) : options.name;
        const std::string safeId = quoteEscape(options.id);
        const std::string safeName = quoteEscape(name);

        std::vector<ScaffoldFile> files;

        files.push_back(ScaffoldFile{
            "mod.json",
            std::format("{{\n"
                        "  \"manifestVersion\": 1,\n"
                        "  \"id\": \"{}\",\n"
                        "  \"name\": \"{}\",\n"
                        "  \"version\": \"0.1.0\",\n"
                        "  \"entry\": \"main.lua\"\n"
                        "}}\n",
                        safeId, safeName) });

        files.push_back(ScaffoldFile{
            "main.lua",
            std::format(
                "-- {}\n"
                "-- Scaffolded by `crabe-cli new-mod`. See docs/guides/mods.md for the full\n"
                "-- lifecycle and API reference, and docs/guides/cli.md for this tool.\n"
                "\n"
                "Crabe.Mod.register({{\n"
                "    id = \"{}\",\n"
                "    name = \"{}\",\n"
                "\n"
                "    -- Called once when the game's Lua VM and the Crabe API are ready.\n"
                "    onInit = function()\n"
                "        Crabe.write(\"[{}] initialized.\")\n"
                "    end,\n"
                "\n"
                "    -- Called every frame (~60 Hz) on the game's Lua thread.\n"
                "    onUpdate = function(dt)\n"
                "    end,\n"
                "\n"
                "    -- Called once per frame to describe this mod's Dear ImGui UI.\n"
                "    onDraw = function()\n"
                "    end,\n"
                "\n"
                "    -- Called when the mod is unloaded or hot-reloaded (F4).\n"
                "    onShutdown = function()\n"
                "    end,\n"
                "}})\n",
                name, safeId, safeName, safeName) });

        files.push_back(ScaffoldFile{
            "i18n/en.toml",
            std::format(
                "# Localization strings for {}.\n"
                "#\n"
                "# CrabeLoader has no built-in i18n loader yet -- this file is a placeholder\n"
                "# so your mod's user-facing strings live in one place from day one. Read it\n"
                "# yourself (a small TOML/INI parser, or split lines by hand) until the\n"
                "# platform grows first-class support.\n"
                "\n"
                "[mod]\n"
                "name = \"{}\"\n"
                "\n"
                "[messages]\n"
                "initialized = \"[{}] initialized.\"\n",
                name, safeName, safeName) });

        files.push_back(ScaffoldFile{
            "README.md",
            std::format(
                "# {}\n"
                "\n"
                "A CrabeLoader mod for Disney Infinity 3.0: Gold Edition.\n"
                "\n"
                "- **id:** `{}`\n"
                "- **version:** 0.1.0\n"
                "\n"
                "## Installing\n"
                "\n"
                "Drop this folder into your game's `mods/` directory, next to `bink2w32.dll`.\n"
                "Press **F4** in-game to hot-reload after making changes.\n"
                "\n"
                "## Developing\n"
                "\n"
                "- `mod.json` -- the manifest (id, version, entry script, dependencies).\n"
                "- `main.lua` -- registers this mod's lifecycle with `Crabe.Mod.register`.\n"
                "- `i18n/en.toml` -- user-facing strings, kept out of the Lua so they can be\n"
                "  translated later.\n"
                "\n"
                "Check this mod before launching the game:\n"
                "\n"
                "```\n"
                "crabe-cli validate .\n"
                "```\n"
                "\n"
                "See `docs/guides/mods.md` in a CrabeLoader checkout for the full lifecycle\n"
                "and API reference.\n",
                name, options.id) });

        files.push_back(ScaffoldFile{
            ".gitignore",
            "# CrabeLoader mod scaffold\n"
            "*.log\n"
            ".DS_Store\n"
            "Thumbs.db\n" });

        return files;
    }

    std::expected<void, std::string> writeScaffold(const ScaffoldOptions& options,
                                                    const std::vector<ScaffoldFile>& files)
    {
        std::error_code ec;
        if (std::filesystem::exists(options.dir, ec)) {
            if (!std::filesystem::is_directory(options.dir, ec)) {
                return std::unexpected(
                    std::format("'{}' already exists and is not a directory", options.dir.string()));
            }
            if (!std::filesystem::is_empty(options.dir, ec)) {
                return std::unexpected(std::format(
                    "'{}' already exists and is not empty; new-mod refuses to write into it",
                    options.dir.string()));
            }
        }

        for (const ScaffoldFile& file : files) {
            const std::filesystem::path fullPath = options.dir / file.relativePath;
            std::filesystem::create_directories(fullPath.parent_path(), ec);

            std::ofstream out(fullPath, std::ios::binary);
            if (!out)
                return std::unexpected(std::format("could not create '{}'", fullPath.string()));
            out << file.content;
            if (!out)
                return std::unexpected(std::format("could not write '{}'", fullPath.string()));
        }

        return {};
    }

    int runNewMod(std::span<const std::string> args, std::ostream& out, std::ostream& err)
    {
        const auto options = parseNewModArgs(args);
        if (!options.has_value()) {
            err << "new-mod: " << options.error() << "\n";
            return 1;
        }

        const auto files = buildScaffold(*options);
        if (!files.has_value()) {
            err << "new-mod: " << files.error() << "\n";
            return 1;
        }

        if (const auto written = writeScaffold(*options, *files); !written.has_value()) {
            err << "new-mod: " << written.error() << "\n";
            return 1;
        }

        out << "new-mod: scaffolded '" << options->id << "' in '" << options->dir.string() << "'\n";
        out << "  Next: crabe-cli validate " << options->dir.string() << "\n";
        return 0;
    }

} // namespace crabe::cli
