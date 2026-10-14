/*
** CrabeLoader
** File description:
** Decides between a fresh install, an update, and a refusal from the facts about a game folder.
** Anything it does not recognise as CrabeLoader is never replaced; that ends in a refusal.
** Reads no file and renames nothing; it only returns the decision.
**
** Authors: @LucasLhomme
*/

#include "installer/install_plan.hpp"

namespace crabe::installer {

    std::string_view describe(Plan plan) noexcept
    {
        switch (plan) {
        case Plan::FreshInstall:
            return "fresh install: CrabeLoader is added as dinput8.dll";
        case Plan::Update:
            return "update: CrabeLoader's dinput8.dll is replaced";
        case Plan::NotAGameFolder:
            return "this folder does not contain the game executable";
        case Plan::LoaderSlotTaken:
            return "dinput8.dll is already there and is not CrabeLoader";
        case Plan::LegacyOriginalMissing:
            return "bink2w32.dll is an old CrabeLoader and the game's original bink2w32_orig.dll is gone";
        }
        return "unknown";
    }

    Plan planInstall(const FolderFacts& facts) noexcept
    {
        if (!facts.hasGameExecutable)
            return Plan::NotAGameFolder;
        if (facts.loader == FileKind::Other)
            return Plan::LoaderSlotTaken;
        if (facts.legacyProxy == FileKind::Loader && !facts.hasLegacyBackup)
            return Plan::LegacyOriginalMissing;
        return facts.loader == FileKind::Loader ? Plan::Update : Plan::FreshInstall;
    }

    bool restoresLegacyOriginal(const FolderFacts& facts) noexcept
    {
        return facts.hasLegacyBackup && facts.legacyProxy != FileKind::Other;
    }

} // namespace crabe::installer
