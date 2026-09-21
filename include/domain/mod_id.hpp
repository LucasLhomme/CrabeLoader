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

// Deciding what to call a mod that declares no id of its own.
//
// Pure string manipulation: no I/O, no Windows headers, no Lua. This used to
// live entirely inside ModManager::discoverAndLoadMods (T9), which is fine for
// the shipped loader but leaves nothing else able to compute the same answer
// without dragging in the infrastructure/application/presentation headers
// ModManager itself depends on. crabe-cli's `resolve` command needs exactly
// this answer -- "what id would the loader give this folder?" -- to print a
// load order that is not just plausible but the *actual* one, so the logic is
// pulled out here once and called from both places.

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace crabe::domain {

    // Turns a folder or standalone script name into "local.<sanitised>":
    // every character outside [a-z0-9] folds to '-', runs of '-' collapse, the
    // ends are trimmed, and a name that sanitises to nothing becomes
    // "local.unnamed". The result always satisfies validateModId().
    [[nodiscard]] std::string synthesiseModId(std::string_view folderName);

    // One discovered mod, reduced to what naming needs: the id its manifest
    // already declares (empty if it declares none, or has no manifest), the
    // folder/script name to synthesise from, and a key to sort by so
    // synthesis lands on the same folder every time regardless of what order
    // the filesystem happened to hand candidates over in.
    struct ModIdCandidate {
        std::string declaredId;
        std::string folderName;
        std::string sortKey;
    };

    // One synthesised id that collided with something already taken, and what
    // it was renamed to. Mirrors the warning ModManager logs when this
    // happens; the caller decides whether and how to report it.
    struct ModIdRenumbering {
        std::size_t index{0}; // position in the candidates/result vectors
        std::string wantedId;
        std::string chosenId;
    };

    // Assigns a final id to every candidate, one rule set, used identically by
    // ModManager::discoverAndLoadMods and by crabe-cli's `resolve`:
    //
    //  1. Every candidate that already declares an id keeps it verbatim, and
    //     that id is never handed to anything else.
    //  2. Every other candidate is synthesised via synthesiseModId(), in
    //     ascending order of sortKey (not input order, which the filesystem
    //     does not promise), and numbered apart ("-2", "-3", ...) when the
    //     synthesised id collides with one already taken -- a declared id or
    //     an earlier synthesised one.
    //
    // Returns one id per candidate, parallel to `candidates`. Every collision
    // that had to be renamed is appended to `renumbered` (when non-null), in
    // the order it was resolved.
    [[nodiscard]] std::vector<std::string> assignModIds(
        const std::vector<ModIdCandidate>& candidates,
        std::vector<ModIdRenumbering>* renumbered = nullptr);

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_MOD_ID_HPP_ */
