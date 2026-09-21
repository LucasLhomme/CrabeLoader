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

// A lexical (not syntactic) scanner over Lua source text, used by `check-api`
// to find call-site expressions without embedding a Lua parser in the CLI.
//
// This is deliberately shallow. It knows enough about Lua's lexical grammar
// to skip comments and string literals (so a call spelled out in a comment or
// in a log message is never mistaken for real code), and enough about dotted
// identifier chains to recognise `A.B.C(` -- but it has no notion of scope,
// control flow or Lua's full grammar. Anything that needs either is out of
// reach on purpose: see check_api.hpp for what that leaves uncheckable.

#include <cstddef>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace crabe::cli {

    // One call-site expression: the dotted (or colon-joined, normalised to
    // dots) path being called, and where its first character sits in the
    // original source -- both 1-based, as an editor would show them.
    struct CallSite {
        std::string path;
        std::size_t line{0};
        std::size_t column{0};
    };

    // Replaces every Lua comment (`--...`, `--[[...]]`, `--[=[...]=]`, at any
    // `=`-level) and every string literal (`'...'`, `"..."`, `[[...]]`,
    // `[=[...]=]`) with spaces, preserving line breaks so positions computed
    // against the result still line up with the original text. Exposed
    // mainly for testing; findCallSites() and hasShadowedRoot() both run
    // against this cleaned text so a call written inside a string or a
    // comment is never reported as real code.
    [[nodiscard]] std::string stripCommentsAndStrings(std::string_view source);

    // Finds every `Ident(.Ident)*(` or `Ident(:Ident)*(` call expression in
    // `source` (comments and strings already excluded) whose leftmost
    // identifier is one of `roots`, in the order they appear. A colon
    // separator is normalised to a dot in the returned path, since Lua's
    // `a:b(...)` is sugar for a dotted call with an implicit first argument
    // -- check-api cares about which name was called, not the sugar.
    //
    // This is a lexical scan: `Crabe["write"](...)`, a call reached through a
    // local alias (`local w = Crabe.write; w(...)`), and any call built up at
    // runtime are invisible to it. That is intentional -- see check_api.hpp.
    [[nodiscard]] std::vector<CallSite> findCallSites(std::string_view source,
                                                       const std::set<std::string>& roots);

    // True when `source` declares a local variable or function parameter
    // named `root` anywhere in the file (comments and strings excluded).
    // Deliberately coarse: it has no notion of scope, so it can only ever
    // produce a false "yes" -- skipping a root that was in fact never
    // shadowed at the point of a given call -- never a false "no" that would
    // let a genuinely shadowed call through unnoticed. Callers that see this
    // return true stop checking that root for the whole file.
    [[nodiscard]] bool hasShadowedRoot(std::string_view source, std::string_view root);

    // Plain Levenshtein edit distance between two strings (insertions,
    // deletions and substitutions each cost 1).
    [[nodiscard]] std::size_t editDistance(std::string_view a, std::string_view b);

} // namespace crabe::cli

#endif /* !CRABELOADER_CLI_LUA_CALL_SCAN_HPP_ */
