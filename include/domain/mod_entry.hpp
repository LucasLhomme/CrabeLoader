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

    inline constexpr std::array<std::string_view, 3> kCharacterDirectories = {
        "characters", "Character", "character"
    };
    inline constexpr std::array<std::string_view, 3> kSkillTreeDirectories = {
        "skilltrees", "Skillstree", "skilltree"
    };

    struct ModEntryPlan {
        std::filesystem::path entryScript;
        bool entryScriptExists{false};
        std::vector<std::filesystem::path> looseScripts;
        std::vector<std::string> triedEntryNames;
        std::vector<std::string> contentDirectories;

        // Returns true if the folder contains neither an entry script nor loose scripts.
        [[nodiscard]] bool runsNothing() const noexcept;

        // Returns true if the mod ships content directories but runs no Lua scripts.
        [[nodiscard]] bool isContentOnly() const noexcept;

        // Returns true if the folder contains no scripts and no content directories.
        [[nodiscard]] bool isEmpty() const noexcept;

        // Formats content directories as a comma-separated string.
        [[nodiscard]] std::string describeContentDirectories() const;

        // Formats tried entry names as a comma-separated string.
        [[nodiscard]] std::string describeTriedEntryNames() const;
    };

    // Determines the entry script or loose scripts to execute for a mod directory.
    [[nodiscard]] ModEntryPlan planModEntry(const std::filesystem::path& modPath,
                                            const std::string& modName,
                                            const ModManifest& manifest);

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_MOD_ENTRY_HPP_ */
