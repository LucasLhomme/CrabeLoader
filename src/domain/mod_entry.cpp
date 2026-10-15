/*
** CrabeLoader
** File description:
** Implements planModEntry: resolves a mod folder's entry script, or the loose .lua fallback.
** Every path decision a mod folder gets is made here, so the loader and crabe-cli cannot disagree.
** Runs nothing and compiles nothing; it only answers what would run.
**
** Authors: @LucasLhomme
*/

#include "domain/mod_entry.hpp"

#include "domain/mod_manifest.hpp"

#include <system_error>
#include <utility>

namespace crabe::domain {

namespace {

    // Joins string tokens with commas and single quotes for readable diagnostics.
    [[nodiscard]] std::string quoteAndJoin(const std::vector<std::string>& names)
    {
        std::string out;
        for (const std::string& name : names) {
            if (!out.empty())
                out += ", ";
            out += '\'';
            out += name;
            out += '\'';
        }
        return out;
    }

} // namespace

// Returns true if the plan specifies neither an entry script nor loose scripts.
bool ModEntryPlan::runsNothing() const noexcept
{
    return !entryScriptExists && looseScripts.empty();
}

// Returns true if the mod has content directories but no executable Lua scripts.
bool ModEntryPlan::isContentOnly() const noexcept
{
    return runsNothing() && !contentDirectories.empty();
}

// Returns true if the mod contains neither Lua scripts nor content directories.
bool ModEntryPlan::isEmpty() const noexcept
{
    return runsNothing() && contentDirectories.empty();
}

// Formats the candidate entry script names tried during resolution.
std::string ModEntryPlan::describeTriedEntryNames() const
{
    return quoteAndJoin(triedEntryNames);
}

// Formats the detected content directories as a comma-separated list.
std::string ModEntryPlan::describeContentDirectories() const
{
    return quoteAndJoin(contentDirectories);
}

// Resolves entry scripts and content directories for a discovered mod folder.
ModEntryPlan planModEntry(const std::filesystem::path& modPath, const std::string& modName,
                          const ModManifest& manifest)
{
    ModEntryPlan plan;
    std::error_code ec;

    if (manifest.isValid() && !manifest.getEntry().empty()) {
        plan.entryScript = modPath / manifest.getEntry();
        plan.triedEntryNames.push_back(manifest.getEntry() + " (declared as \"entry\" in mod.json)");
    } else {
        const std::filesystem::path mainScript = modPath / "main.lua";
        plan.triedEntryNames.emplace_back("main.lua");
        if (std::filesystem::exists(mainScript, ec)) {
            plan.entryScript = mainScript;
        } else {
            plan.entryScript = modPath / (modName + ".lua");
            plan.triedEntryNames.push_back(modName + ".lua");
        }
    }

    for (const std::array<std::string_view, 3>* names :
         { &kCharacterDirectories, &kSkillTreeDirectories }) {
        for (const std::string_view name : *names) {
            if (std::filesystem::is_directory(modPath / name, ec)) {
                plan.contentDirectories.emplace_back(name);
                break;
            }
        }
    }

    plan.entryScriptExists = std::filesystem::exists(plan.entryScript, ec);
    if (plan.entryScriptExists)
        return plan;

    for (const auto& entry : std::filesystem::directory_iterator(modPath, ec)) {
        if (entry.is_regular_file(ec) && entry.path().extension() == ".lua")
            plan.looseScripts.push_back(entry.path());
    }

    return plan;
}

} // namespace crabe::domain
