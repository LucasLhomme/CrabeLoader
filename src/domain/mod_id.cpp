/*
** CrabeLoader
** File description:
** Implements id synthesis and assignment: declared ids are kept, the rest become local.<folder>.
** A declared id is never renamed, so a synthesised one is what moves when two ids collide.
** Pure string work only; a guard bank keeps Windows, Lua and the hook engine out of this file.
**
** Authors: @LucasLhomme
*/

#include "domain/mod_id.hpp"

#include <algorithm>
#include <format>
#include <set>

namespace crabe::domain {

    std::string synthesiseModId(std::string_view folderName)
    {
        std::string suffix;
        suffix.reserve(folderName.size() + 1);

        for (const char character : folderName) {
            if ((character >= 'a' && character <= 'z') || (character >= '0' && character <= '9')) {
                suffix += character;
            } else if (character >= 'A' && character <= 'Z') {
                suffix += static_cast<char>(character - 'A' + 'a');
            } else if (!suffix.empty() && suffix.back() != '-') {
                suffix += '-';
            }
        }
        while (!suffix.empty() && suffix.back() == '-')
            suffix.pop_back();

        if (suffix.empty())
            suffix = "unnamed";

        return "local." + suffix;
    }

    std::vector<std::string> assignModIds(const std::vector<ModIdCandidate>& candidates,
                                          std::vector<ModIdRenumbering>* renumbered)
    {
        std::vector<std::string> ids(candidates.size());

        // ---- 1. Declared ids go first and are never renamed. ----
        std::set<std::string> takenIds;
        for (std::size_t index = 0; index < candidates.size(); ++index) {
            if (!candidates[index].declaredId.empty()) {
                ids[index] = candidates[index].declaredId;
                takenIds.insert(ids[index]);
            }
        }

        // ---- 2. Everything else is synthesised in sortKey order, so two
        //         folders that sanitise to the same id are numbered apart the
        //         same way regardless of what order the caller discovered
        //         them in. ----
        std::vector<std::size_t> unnamed;
        for (std::size_t index = 0; index < candidates.size(); ++index) {
            if (candidates[index].declaredId.empty())
                unnamed.push_back(index);
        }
        std::sort(unnamed.begin(), unnamed.end(), [&candidates](std::size_t lhs, std::size_t rhs) {
            return candidates[lhs].sortKey < candidates[rhs].sortKey;
        });

        for (std::size_t index : unnamed) {
            const std::string wanted = synthesiseModId(candidates[index].folderName);
            std::string chosen = wanted;
            for (unsigned int attempt = 2; takenIds.count(chosen) != 0; ++attempt)
                chosen = std::format("{}-{}", wanted, attempt);

            if (chosen != wanted && renumbered != nullptr)
                renumbered->push_back(ModIdRenumbering{ index, wanted, chosen });

            takenIds.insert(chosen);
            ids[index] = std::move(chosen);
        }

        return ids;
    }

} // namespace crabe::domain
