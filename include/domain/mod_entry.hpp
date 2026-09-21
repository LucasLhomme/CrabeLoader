/*
** CrabeLoader
** File description:
** Decides what a mod folder runs: its entry script, or every top-level .lua when there is none.
** The single copy of that rule -- the loader executes the answer, crabe-cli validate reports it.
** Reads the filesystem and nothing above the domain layer; no Lua state is needed to ask it.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_DOMAIN_MOD_ENTRY_HPP_
#define CRABELOADER_DOMAIN_MOD_ENTRY_HPP_

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace crabe::domain {

    class ModManifest;

    // Subdirectories of a mod folder the loader consumes outside Lua execution.
    // Both are read at boot by src/application/loader_content.cpp, before any
    // Lua state exists -- which is why a mod can carry one and no entry script
    // and still be doing its job. The spelling variants are historical: both
    // readers have always accepted them, so they are listed here rather than
    // re-typed at each site.
    inline constexpr std::array<std::string_view, 3> kCharacterDirectories = {
        "characters", "Character", "character"
    };
    inline constexpr std::array<std::string_view, 3> kSkillTreeDirectories = {
        "skilltrees", "Skillstree", "skilltree"
    };

    // What a mod directory will run, and what was looked for to decide it.
    //
    // This used to exist twice: ModManager::resolveEntryScript plus the
    // fallback branch of loadModDirectory, and a hand-kept copy in
    // src/cli/validate_command.cpp whose own comment asked the next reader to
    // keep the two in step by eye. They had already drifted on the one case
    // that matters -- the CLI refused a folder holding no Lua, the loader
    // reported it loaded -- which is what mods/crabe_heroes ran into.
    struct ModEntryPlan {
        // The entry script that was resolved, whether or not it is on disk:
        // a mod author reading "looked for X" needs to see X even when X is
        // exactly what is missing.
        std::filesystem::path entryScript;
        bool entryScriptExists{false};

        // Every top-level *.lua, in directory order, populated only when no
        // entry script exists. Not recursive, deliberately: a subdirectory is
        // a module tree reached through package.path, not something to run on
        // sight -- which is why a folder whose only Lua lives one level down
        // runs nothing at all.
        std::vector<std::filesystem::path> looseScripts;

        // The names that were tried, in order, phrased for a mod author to
        // read. Recorded whether or not one matched, because it is this list
        // that turns "nothing ran" into something actionable.
        std::vector<std::string> triedEntryNames;

        // Content subdirectories found, named as they are on disk. A mod that
        // ships only characters/ is the reference example of what this loader
        // does -- mods/crabe_heroes exposes Mace Windu with no Lua of its own --
        // so it must not be mistaken for a folder with nothing in it.
        std::vector<std::string> contentDirectories;

        // No entry script on disk and no loose script either: the folder holds
        // no Lua for the mod loader to run. On its own this is not a defect;
        // ask isContentOnly() before calling it one.
        [[nodiscard]] bool runsNothing() const noexcept;

        // Runs no Lua, but carries content the loader reads elsewhere.
        [[nodiscard]] bool isContentOnly() const noexcept;

        // Nothing to run and nothing to contribute: the folder is not a mod.
        [[nodiscard]] bool isEmpty() const noexcept;

        // contentDirectories as one quoted, comma-separated clause.
        [[nodiscard]] std::string describeContentDirectories() const;

        // triedEntryNames as one quoted, comma-separated clause, for the line
        // the author reads.
        [[nodiscard]] std::string describeTriedEntryNames() const;
    };

    // Resolution order, unchanged from what the loader has always done:
    //
    //   1. manifest "entry", when the manifest is valid and declares one --
    //      and then nothing else is tried, because a declared entry that is
    //      missing is a mistake to report, not one to paper over;
    //   2. main.lua;
    //   3. <folder name>.lua.
    //
    // `modPath` not being a readable directory is not an error here: the plan
    // simply comes back with no loose scripts, and runsNothing() answers true.
    [[nodiscard]] ModEntryPlan planModEntry(const std::filesystem::path& modPath,
                                            const std::string& modName,
                                            const ModManifest& manifest);

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_MOD_ENTRY_HPP_ */
