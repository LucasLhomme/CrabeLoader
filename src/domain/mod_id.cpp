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

    // Converts a folder or script name into a sanitized "local.<name>" mod id.
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

    // Assigns deterministic mod ids to all candidates, resolving collisions by renumbering.
    std::vector<std::string> assignModIds(const std::vector<ModIdCandidate>& candidates,
                                          std::vector<ModIdRenumbering>* renumbered)
    {
        std::vector<std::string> ids(candidates.size());

        std::set<std::string> takenIds;
        for (std::size_t index = 0; index < candidates.size(); ++index) {
            if (!candidates[index].declaredId.empty()) {
                ids[index] = candidates[index].declaredId;
                takenIds.insert(ids[index]);
            }
        }

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
