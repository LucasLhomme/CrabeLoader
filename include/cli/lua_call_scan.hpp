/*
** CrabeLoader
** File description:
** Declares a lexical scanner that finds dotted call sites in Lua text without a Lua parser.
** Deliberately shallow: comments and string literals are skipped, never read as code.
** Judges nothing about a call being wrong; that decision is cli/check_api.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_CLI_LUA_CALL_SCAN_HPP_
#define CRABELOADER_CLI_LUA_CALL_SCAN_HPP_

#include <cstddef>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace crabe::cli {

    struct CallSite {
        std::string path;
        std::size_t line{0};
        std::size_t column{0};
    };

    // Replaces comments and string literals with spaces while preserving line breaks.
    [[nodiscard]] std::string stripCommentsAndStrings(std::string_view source);

    // Finds call expressions whose leftmost identifier is one of roots, normalising colons to dots.
    [[nodiscard]] std::vector<CallSite> findCallSites(std::string_view source,
                                                       const std::set<std::string>& roots);

    // Returns true when source declares a local variable or parameter named root anywhere in the file.
    [[nodiscard]] bool hasShadowedRoot(std::string_view source, std::string_view root);

    // Plain Levenshtein edit distance between two strings (insertions,
    // deletions and substitutions each cost 1).
    [[nodiscard]] std::size_t editDistance(std::string_view a, std::string_view b);

} // namespace crabe::cli

#endif /* !CRABELOADER_CLI_LUA_CALL_SCAN_HPP_ */
