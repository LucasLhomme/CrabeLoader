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

bool ModEntryPlan::runsNothing() const noexcept
{
    return !entryScriptExists && looseScripts.empty();
}

std::string ModEntryPlan::describeTriedEntryNames() const
{
    std::string out;
    for (const std::string& name : triedEntryNames) {
        if (!out.empty())
            out += ", ";
        out += '\'';
        out += name;
        out += '\'';
    }
    return out;
}

ModEntryPlan planModEntry(const std::filesystem::path& modPath, const std::string& modName,
                          const ModManifest& manifest)
{
    ModEntryPlan plan;
    std::error_code ec;

    if (manifest.isValid() && !manifest.getEntry().empty()) {
        // A declared entry is the author's own statement of what runs. It
        // short-circuits the conventional names rather than falling back past
        // them, so a typo in mod.json surfaces as that typo.
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

    plan.entryScriptExists = std::filesystem::exists(plan.entryScript, ec);
    if (plan.entryScriptExists)
        return plan;

    // Top level only. directory_iterator with an error_code yields nothing for
    // a path that is not a readable directory, which is the answer wanted here
    // -- a mod folder that cannot be read runs nothing, and says so through
    // runsNothing() like any other empty one.
    for (const auto& entry : std::filesystem::directory_iterator(modPath, ec)) {
        if (entry.is_regular_file(ec) && entry.path().extension() == ".lua")
            plan.looseScripts.push_back(entry.path());
    }

    return plan;
}

} // namespace crabe::domain
