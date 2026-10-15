/*
** CrabeLoader
** File description:
** Declares what to call a mod that carries no id of its own, and how ids are kept unique.
** Pure string work, so crabe-cli computes the same ids the loader does without its headers.
** Decides no load order, only names. Ordering is domain/dependency_resolver.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_DOMAIN_MOD_ID_HPP_
#define CRABELOADER_DOMAIN_MOD_ID_HPP_

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace crabe::domain {

    // Converts a folder or script name into a sanitized "local.<name>" mod id.
    [[nodiscard]] std::string synthesiseModId(std::string_view folderName);

    struct ModIdCandidate {
        std::string declaredId;
        std::string folderName;
        std::string sortKey;
    };

    struct ModIdRenumbering {
        std::size_t index{0};
        std::string wantedId;
        std::string chosenId;
    };

    // Assigns deterministic mod ids to all candidates, resolving collisions by renumbering.
    [[nodiscard]] std::vector<std::string> assignModIds(
        const std::vector<ModIdCandidate>& candidates,
        std::vector<ModIdRenumbering>* renumbered = nullptr);

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_MOD_ID_HPP_ */
