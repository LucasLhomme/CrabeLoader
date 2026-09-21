/*
** CrabeLoader
** File description:
** Implements the lexical Lua scan: long brackets, comments, strings, then dotted call sites.
** Long-bracket levels must be counted, or a [==[ string closes on the wrong ]] and code is eaten.
** Understands no Lua semantics; whether a call is suspect is decided in src/cli/check_api.cpp.
**
** Authors: @LucasLhomme
*/

#include "cli/lua_call_scan.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <utility>

namespace crabe::cli {

    namespace {

        // Returns the `=`-level of a long-bracket opener "[" "="* "[" starting
        // exactly at `pos`, or std::string::npos if `pos` is not one.
        std::size_t longBracketLevel(std::string_view text, std::size_t pos)
        {
            if (pos >= text.size() || text[pos] != '[')
                return std::string::npos;
            std::size_t k = pos + 1;
            std::size_t level = 0;
            while (k < text.size() && text[k] == '=') {
                ++level;
                ++k;
            }
            return (k < text.size() && text[k] == '[') ? level : std::string::npos;
        }

        // Index, just past a matching "]" "="*level "]" searched for from
        // `from`, or text.size() if the file ends before one is found.
        std::size_t findLongBracketEnd(std::string_view text, std::size_t from, std::size_t level)
        {
            std::size_t k = from;
            while (k < text.size()) {
                if (text[k] == ']') {
                    std::size_t j = k + 1;
                    std::size_t eq = 0;
                    while (j < text.size() && text[j] == '=') {
                        ++eq;
                        ++j;
                    }
                    if (eq == level && j < text.size() && text[j] == ']')
                        return j + 1;
                }
                ++k;
            }
            return text.size();
        }

        // 1-based (line, column) lookups for byte offsets into one piece of
        // text, computed once so a file with many call sites is not
        // rescanned from the top for each one.
        class LineIndex {
        public:
            explicit LineIndex(std::string_view text)
            {
                _lineStarts.push_back(0);
                for (std::size_t i = 0; i < text.size(); ++i) {
                    if (text[i] == '\n')
                        _lineStarts.push_back(i + 1);
                }
            }

            [[nodiscard]] std::pair<std::size_t, std::size_t> at(std::size_t offset) const
            {
                const auto it = std::upper_bound(_lineStarts.begin(), _lineStarts.end(), offset);
                const std::size_t lineIndex =
                    static_cast<std::size_t>(it - _lineStarts.begin()) - 1;
                return { lineIndex + 1, offset - _lineStarts[lineIndex] + 1 };
            }

        private:
            std::vector<std::size_t> _lineStarts;
        };

        std::string_view trim(std::string_view text)
        {
            while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
                text.remove_prefix(1);
            while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
                text.remove_suffix(1);
            return text;
        }

        // True when `commaSeparated` (as captured from a `local` declaration
        // or a function parameter list) names `needle` as one of its plain
        // identifiers.
        bool identListContains(const std::string& commaSeparated, std::string_view needle)
        {
            std::size_t start = 0;
            while (start <= commaSeparated.size()) {
                const std::size_t comma = commaSeparated.find(',', start);
                const std::size_t stop =
                    (comma == std::string::npos) ? commaSeparated.size() : comma;
                const std::string_view name =
                    trim(std::string_view(commaSeparated).substr(start, stop - start));
                if (name == needle)
                    return true;
                if (comma == std::string::npos)
                    break;
                start = comma + 1;
            }
            return false;
        }

    } // namespace

    std::string stripCommentsAndStrings(std::string_view source)
    {
        std::string out(source);
        const std::size_t n = out.size();

        auto blank = [&out](std::size_t from, std::size_t to) {
            for (std::size_t k = from; k < to; ++k) {
                if (out[k] != '\n')
                    out[k] = ' ';
            }
        };

        std::size_t i = 0;
        while (i < n) {
            if (out[i] == '-' && i + 1 < n && out[i + 1] == '-') {
                const std::size_t start = i;
                const std::size_t after = i + 2;
                const std::size_t level = longBracketLevel(out, after);
                std::size_t end;
                if (level != std::string::npos) {
                    const std::size_t contentStart = after + 2 + level;
                    end = findLongBracketEnd(out, contentStart, level);
                } else {
                    end = after;
                    while (end < n && out[end] != '\n')
                        ++end;
                }
                blank(start, end);
                i = end;
                continue;
            }

            if (out[i] == '[') {
                const std::size_t level = longBracketLevel(out, i);
                if (level != std::string::npos) {
                    const std::size_t contentStart = i + 2 + level;
                    const std::size_t end = findLongBracketEnd(out, contentStart, level);
                    blank(i, end);
                    i = end;
                    continue;
                }
            }

            if (out[i] == '\'' || out[i] == '"') {
                const char quote = out[i];
                std::size_t k = i + 1;
                while (k < n && out[k] != quote && out[k] != '\n') {
                    if (out[k] == '\\' && k + 1 < n)
                        ++k;
                    ++k;
                }
                const std::size_t end = (k < n && out[k] == quote) ? k + 1 : k;
                blank(i, end);
                i = end;
                continue;
            }

            ++i;
        }

        return out;
    }

    std::vector<CallSite> findCallSites(std::string_view source, const std::set<std::string>& roots)
    {
        const std::string cleaned = stripCommentsAndStrings(source);
        const LineIndex lines(cleaned);

        // A dotted or colon-joined identifier chain immediately followed by
        // '('. `\b` keeps this from matching in the middle of a longer
        // identifier; it does not stop a chain from being found starting
        // anywhere a call could legally begin.
        static const std::regex callPattern(
            R"(\b([A-Za-z_][A-Za-z0-9_]*(?:[.:][A-Za-z_][A-Za-z0-9_]*)*)[ \t]*\()");

        std::vector<CallSite> sites;
        for (std::sregex_iterator it(cleaned.begin(), cleaned.end(), callPattern), end; it != end;
             ++it) {
            const std::smatch& match = *it;
            std::string path = match[1].str();
            std::replace(path.begin(), path.end(), ':', '.');

            const std::size_t firstDot = path.find('.');
            const std::string root = (firstDot == std::string::npos) ? path : path.substr(0, firstDot);
            if (roots.find(root) == roots.end())
                continue;

            const std::size_t offset = static_cast<std::size_t>(match.position(1));
            const auto [line, column] = lines.at(offset);
            sites.push_back(CallSite{ std::move(path), line, column });
        }
        return sites;
    }

    bool hasShadowedRoot(std::string_view source, std::string_view root)
    {
        const std::string cleaned = stripCommentsAndStrings(source);

        static const std::regex localDecl(
            R"(\blocal\s+([A-Za-z_][A-Za-z0-9_]*(?:\s*,\s*[A-Za-z_][A-Za-z0-9_]*)*))");
        static const std::regex functionParams(R"(\bfunction\b[^()\n]*\(([^)]*)\))");

        for (std::sregex_iterator it(cleaned.begin(), cleaned.end(), localDecl), end; it != end;
             ++it) {
            if (identListContains((*it)[1].str(), root))
                return true;
        }
        for (std::sregex_iterator it(cleaned.begin(), cleaned.end(), functionParams), end;
             it != end; ++it) {
            if (identListContains((*it)[1].str(), root))
                return true;
        }
        return false;
    }

    std::size_t editDistance(std::string_view a, std::string_view b)
    {
        const std::size_t n = a.size();
        const std::size_t m = b.size();

        std::vector<std::size_t> prev(m + 1);
        std::vector<std::size_t> curr(m + 1);
        for (std::size_t j = 0; j <= m; ++j)
            prev[j] = j;

        for (std::size_t i = 1; i <= n; ++i) {
            curr[0] = i;
            for (std::size_t j = 1; j <= m; ++j) {
                const std::size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
                curr[j] = std::min({ prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost });
            }
            std::swap(prev, curr);
        }
        return prev[m];
    }

} // namespace crabe::cli
