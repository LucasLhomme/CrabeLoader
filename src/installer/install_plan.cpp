/*
** CrabeLoader
** File description:
** Decides between a fresh install, an update, and a refusal from the facts about a game folder.
** Anything it does not recognise ends in a refusal, never in a guess.
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
            return "fresh install: the original bink2w32.dll is kept as bink2w32_orig.dll";
        case Plan::Update:
            return "update: bink2w32.dll is replaced, bink2w32_orig.dll is left alone";
        case Plan::NotAGameFolder:
            return "this folder does not contain the game executable";
        case Plan::ProxyMissing:
            return "bink2w32.dll is missing and there is no backup to rely on";
        case Plan::ProxyUnrecognized:
            return "bink2w32.dll is not a known original and there is no backup";
        }
        return "unknown";
    }

    Plan planInstall(const FolderFacts& facts) noexcept
    {
        if (!facts.hasGameExecutable)
            return Plan::NotAGameFolder;
        if (facts.hasBackup)
            return Plan::Update;

        switch (facts.proxy) {
        case ProxySlot::KnownOriginal:
            return Plan::FreshInstall;
        case ProxySlot::Missing:
            return Plan::ProxyMissing;
        case ProxySlot::Other:
            return Plan::ProxyUnrecognized;
        }
        return Plan::ProxyUnrecognized;
    }

} // namespace crabe::installer
