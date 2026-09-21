#include "domain/semver.hpp"

#include <algorithm>
#include <limits>

namespace crabe::domain {

    namespace {

        constexpr std::uint64_t kMaxComponent = std::numeric_limits<std::uint32_t>::max();

        [[nodiscard]] bool isDigit(char c) noexcept
        {
            return c >= '0' && c <= '9';
        }

        [[nodiscard]] bool isIdentifierChar(char c) noexcept
        {
            return isDigit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-';
        }

        [[nodiscard]] bool isAllDigits(std::string_view text) noexcept
        {
            return !text.empty() && std::ranges::all_of(text, isDigit);
        }

        // One numeric component of the version core. Strict on purpose: a
        // leading zero is a specification violation, not a formatting quirk,
        // and silently accepting "01" would make "01.0.0" and "1.0.0" two
        // spellings of one version that compare equal but do not round-trip.
        [[nodiscard]] std::expected<std::uint32_t, SemVerError> parseComponent(std::string_view text)
        {
            if (text.empty())
                return std::unexpected(SemVerError::NotANumber);
            if (!isAllDigits(text))
                return std::unexpected(SemVerError::NotANumber);
            if (text.size() > 1 && text.front() == '0')
                return std::unexpected(SemVerError::LeadingZero);

            std::uint64_t value = 0;
            for (char c : text) {
                value = value * 10 + static_cast<std::uint64_t>(c - '0');
                if (value > kMaxComponent)
                    return std::unexpected(SemVerError::NumericOverflow);
            }
            return static_cast<std::uint32_t>(value);
        }

        // A dot-separated run of [0-9A-Za-z-] identifiers, none of them empty.
        // Numeric prerelease identifiers additionally may not carry a leading
        // zero; build metadata identifiers may, so `numericRulesApply` says
        // which set is being checked.
        [[nodiscard]] std::expected<void, SemVerError> validateIdentifiers(
            std::string_view text, bool numericRulesApply)
        {
            if (text.empty())
                return std::unexpected(SemVerError::EmptyIdentifier);

            std::size_t start = 0;
            while (true) {
                const std::size_t dot = text.find('.', start);
                const std::string_view part =
                    text.substr(start, dot == std::string_view::npos ? std::string_view::npos
                                                                     : dot - start);
                if (part.empty())
                    return std::unexpected(SemVerError::EmptyIdentifier);
                for (char c : part) {
                    if (!isIdentifierChar(c))
                        return std::unexpected(SemVerError::InvalidCharacter);
                }
                if (numericRulesApply && isAllDigits(part) && part.size() > 1 && part.front() == '0')
                    return std::unexpected(SemVerError::LeadingZero);

                if (dot == std::string_view::npos)
                    break;
                start = dot + 1;
            }
            return {};
        }

        // Precedence between two prerelease identifiers (semver.org, rule 11):
        // all-numeric ones compare numerically and sort below alphanumeric
        // ones; anything else compares as ASCII.
        [[nodiscard]] std::strong_ordering compareIdentifier(std::string_view lhs,
                                                             std::string_view rhs) noexcept
        {
            const bool lhsNumeric = isAllDigits(lhs);
            const bool rhsNumeric = isAllDigits(rhs);

            if (lhsNumeric && rhsNumeric) {
                // Compared by length first so an identifier wider than any
                // integer type still orders correctly.
                std::string_view l = lhs.substr(std::min(lhs.find_first_not_of('0'), lhs.size() - 1));
                std::string_view r = rhs.substr(std::min(rhs.find_first_not_of('0'), rhs.size() - 1));
                if (l.size() != r.size())
                    return l.size() <=> r.size();
                return l.compare(r) <=> 0;
            }
            if (lhsNumeric != rhsNumeric)
                return lhsNumeric ? std::strong_ordering::less : std::strong_ordering::greater;
            return lhs.compare(rhs) <=> 0;
        }

        [[nodiscard]] std::strong_ordering comparePrerelease(std::string_view lhs,
                                                             std::string_view rhs) noexcept
        {
            if (lhs.empty() && rhs.empty())
                return std::strong_ordering::equal;
            // A release outranks any of its prereleases.
            if (lhs.empty())
                return std::strong_ordering::greater;
            if (rhs.empty())
                return std::strong_ordering::less;

            constexpr std::size_t kEnd = std::string_view::npos;
            std::size_t lhsPos = 0;
            std::size_t rhsPos = 0;
            while (lhsPos != kEnd || rhsPos != kEnd) {
                // Identical up to here, so the longer set of identifiers wins
                // (semver.org, rule 11: 1.0.0-alpha < 1.0.0-alpha.1).
                if (lhsPos == kEnd)
                    return std::strong_ordering::less;
                if (rhsPos == kEnd)
                    return std::strong_ordering::greater;

                const std::size_t lhsDot = lhs.find('.', lhsPos);
                const std::size_t rhsDot = rhs.find('.', rhsPos);
                const std::strong_ordering order = compareIdentifier(
                    lhs.substr(lhsPos, lhsDot == kEnd ? kEnd : lhsDot - lhsPos),
                    rhs.substr(rhsPos, rhsDot == kEnd ? kEnd : rhsDot - rhsPos));
                if (order != std::strong_ordering::equal)
                    return order;

                lhsPos = (lhsDot == kEnd) ? kEnd : lhsDot + 1;
                rhsPos = (rhsDot == kEnd) ? kEnd : rhsDot + 1;
            }
            return std::strong_ordering::equal;
        }

        [[nodiscard]] std::string_view stripLeadingV(std::string_view text) noexcept
        {
            if (!text.empty() && (text.front() == 'v' || text.front() == 'V'))
                text.remove_prefix(1);
            return text;
        }

        // ------------------------------------------------------------------
        // Range parsing helpers
        // ------------------------------------------------------------------

        // A version token as written inside a range: possibly partial
        // ("1.2"), possibly wildcarded ("1.x"). `specified` counts the
        // concrete numeric components that preceded any wildcard.
        struct PartialVersion {
            std::uint32_t parts[3]{0, 0, 0};
            int specified{0};
            bool wildcard{false};
            std::string prerelease;
            std::string build;
        };

        [[nodiscard]] bool isWildcardToken(std::string_view text) noexcept
        {
            return text == "x" || text == "X" || text == "*";
        }

        [[nodiscard]] std::expected<PartialVersion, VersionRangeError> parsePartial(
            std::string_view text)
        {
            text = stripLeadingV(text);
            if (text.empty())
                return std::unexpected(VersionRangeError::MissingVersion);

            PartialVersion out;

            std::string_view core = text;
            if (const std::size_t plus = core.find('+'); plus != std::string_view::npos) {
                out.build = std::string(core.substr(plus + 1));
                core = core.substr(0, plus);
            }
            if (const std::size_t dash = core.find('-'); dash != std::string_view::npos) {
                out.prerelease = std::string(core.substr(dash + 1));
                core = core.substr(0, dash);
            }
            if (core.empty())
                return std::unexpected(VersionRangeError::MissingVersion);

            int index = 0;
            std::size_t start = 0;
            while (start <= core.size()) {
                const std::size_t dot = core.find('.', start);
                const std::string_view part = core.substr(
                    start, dot == std::string_view::npos ? std::string_view::npos : dot - start);

                if (index >= 3)
                    return std::unexpected(VersionRangeError::InvalidVersion);

                if (isWildcardToken(part)) {
                    out.wildcard = true;
                } else {
                    // Nothing concrete may follow a wildcard: "1.x.3" asks for
                    // two incompatible things at once.
                    if (out.wildcard)
                        return std::unexpected(VersionRangeError::MisplacedWildcard);
                    const auto component = parseComponent(part);
                    if (!component)
                        return std::unexpected(VersionRangeError::InvalidVersion);
                    out.parts[index] = *component;
                    out.specified = index + 1;
                }

                ++index;
                if (dot == std::string_view::npos)
                    break;
                start = dot + 1;
            }

            if (!out.prerelease.empty()) {
                if (out.wildcard || out.specified != 3)
                    return std::unexpected(VersionRangeError::InvalidVersion);
                if (!validateIdentifiers(out.prerelease, true))
                    return std::unexpected(VersionRangeError::InvalidVersion);
            }
            if (!out.build.empty() && !validateIdentifiers(out.build, false))
                return std::unexpected(VersionRangeError::InvalidVersion);

            return out;
        }

        // Bumping a component can overflow only for a version nobody will ever
        // write; when it would, the caller drops the upper bound rather than
        // wrapping around to an upper bound of zero that matches nothing.
        [[nodiscard]] bool bump(std::uint32_t value, std::uint32_t& out) noexcept
        {
            if (value == std::numeric_limits<std::uint32_t>::max())
                return false;
            out = value + 1;
            return true;
        }

    } // namespace

    std::string_view describe(SemVerError error) noexcept
    {
        switch (error) {
        case SemVerError::Empty:
            return "it is empty";
        case SemVerError::MissingMinor:
            return "it has no minor component (semantic versions are MAJOR.MINOR.PATCH)";
        case SemVerError::MissingPatch:
            return "it has no patch component (semantic versions are MAJOR.MINOR.PATCH)";
        case SemVerError::NotANumber:
            return "MAJOR, MINOR and PATCH must each be a run of digits";
        case SemVerError::LeadingZero:
            return "numeric components may not carry a leading zero";
        case SemVerError::NumericOverflow:
            return "a numeric component does not fit in 32 bits";
        case SemVerError::EmptyIdentifier:
            return "a prerelease or build identifier is empty";
        case SemVerError::InvalidCharacter:
            return "prerelease and build identifiers accept only [0-9A-Za-z-]";
        case SemVerError::TrailingCharacters:
            return "it has trailing characters";
        }
        return "it is not a semantic version";
    }

    std::string_view describe(VersionRangeError error) noexcept
    {
        switch (error) {
        case VersionRangeError::Empty:
            return "it is empty (write \"*\" to accept any version)";
        case VersionRangeError::UnknownOperator:
            return "it starts with an operator that is not one of >= > <= < = ~ ^";
        case VersionRangeError::MissingVersion:
            return "an operator is not followed by a version";
        case VersionRangeError::InvalidVersion:
            return "one of its bounds is not a version";
        case VersionRangeError::MisplacedWildcard:
            return "nothing may follow a wildcard (write \"1.x\", not \"1.x.3\")";
        case VersionRangeError::WildcardWithOperator:
            return "a comparison operator needs a concrete bound, not a wildcard";
        case VersionRangeError::UnsupportedOrClause:
            return "alternatives with \"||\" are not supported";
        }
        return "it is not a version range";
    }

    std::expected<SemVer, SemVerError> SemVer::parse(std::string_view text)
    {
        text = stripLeadingV(text);
        if (text.empty())
            return std::unexpected(SemVerError::Empty);
        if (text.find_first_of(" \t\r\n") != std::string_view::npos)
            return std::unexpected(SemVerError::TrailingCharacters);

        SemVer out;

        std::string_view core = text;
        if (const std::size_t plus = core.find('+'); plus != std::string_view::npos) {
            out._build = std::string(core.substr(plus + 1));
            core = core.substr(0, plus);
            if (auto ok = validateIdentifiers(out._build, false); !ok)
                return std::unexpected(ok.error());
        }
        if (const std::size_t dash = core.find('-'); dash != std::string_view::npos) {
            out._prerelease = std::string(core.substr(dash + 1));
            core = core.substr(0, dash);
            if (auto ok = validateIdentifiers(out._prerelease, true); !ok)
                return std::unexpected(ok.error());
        }
        if (core.empty())
            return std::unexpected(SemVerError::Empty);

        const std::size_t firstDot = core.find('.');
        if (firstDot == std::string_view::npos)
            return std::unexpected(SemVerError::MissingMinor);
        const std::size_t secondDot = core.find('.', firstDot + 1);
        if (secondDot == std::string_view::npos)
            return std::unexpected(SemVerError::MissingPatch);
        if (core.find('.', secondDot + 1) != std::string_view::npos)
            return std::unexpected(SemVerError::TrailingCharacters);

        const auto majorPart = parseComponent(core.substr(0, firstDot));
        if (!majorPart)
            return std::unexpected(majorPart.error());
        const auto minorPart = parseComponent(core.substr(firstDot + 1, secondDot - firstDot - 1));
        if (!minorPart)
            return std::unexpected(minorPart.error());
        const auto patchPart = parseComponent(core.substr(secondDot + 1));
        if (!patchPart)
            return std::unexpected(patchPart.error());

        out._major = *majorPart;
        out._minor = *minorPart;
        out._patch = *patchPart;
        return out;
    }

    std::strong_ordering SemVer::compare(const SemVer& other) const noexcept
    {
        if (const auto order = _major <=> other._major; order != std::strong_ordering::equal)
            return order;
        if (const auto order = _minor <=> other._minor; order != std::strong_ordering::equal)
            return order;
        if (const auto order = _patch <=> other._patch; order != std::strong_ordering::equal)
            return order;
        return comparePrerelease(_prerelease, other._prerelease);
    }

    std::string SemVer::toString() const
    {
        std::string out = std::to_string(_major);
        out += '.';
        out += std::to_string(_minor);
        out += '.';
        out += std::to_string(_patch);
        if (!_prerelease.empty()) {
            out += '-';
            out += _prerelease;
        }
        if (!_build.empty()) {
            out += '+';
            out += _build;
        }
        return out;
    }

    std::expected<VersionRange, VersionRangeError> VersionRange::parse(std::string_view text)
    {
        if (text.find("||") != std::string_view::npos)
            return std::unexpected(VersionRangeError::UnsupportedOrClause);

        VersionRange range;
        range._text = std::string(text);

        constexpr std::string_view kSeparators = " \t\r\n,";
        std::size_t pos = 0;
        bool sawToken = false;

        while (pos < text.size()) {
            pos = text.find_first_not_of(kSeparators, pos);
            if (pos == std::string_view::npos)
                break;

            // The operator, if any. Two-character forms are tested first so
            // ">=" is never read as ">" followed by a bogus version "=1.2.0".
            const char lead = text[pos];
            std::string_view op;
            if (text.compare(pos, 2, ">=") == 0 || text.compare(pos, 2, "<=") == 0) {
                op = text.substr(pos, 2);
                pos += 2;
            } else if (lead == '>' || lead == '<' || lead == '=' || lead == '~' || lead == '^') {
                op = text.substr(pos, 1);
                pos += 1;
                // "=>" and "=<" are a common slip and mean nothing here.
                if (pos < text.size() && (text[pos] == '>' || text[pos] == '<' || text[pos] == '='))
                    return std::unexpected(VersionRangeError::UnknownOperator);
            } else if (!isDigit(lead) && lead != 'v' && lead != 'V'
                       && !isWildcardToken(text.substr(pos, 1))) {
                return std::unexpected(VersionRangeError::UnknownOperator);
            }

            // An operator may be detached from its version (">= 1.2.0").
            pos = text.find_first_not_of(" \t\r\n", pos);
            if (pos == std::string_view::npos)
                return std::unexpected(VersionRangeError::MissingVersion);

            const std::size_t end = text.find_first_of(kSeparators, pos);
            const std::string_view token = text.substr(
                pos, end == std::string_view::npos ? std::string_view::npos : end - pos);
            pos = (end == std::string_view::npos) ? text.size() : end;

            if (token.empty())
                return std::unexpected(VersionRangeError::MissingVersion);
            sawToken = true;

            const auto partial = parsePartial(token);
            if (!partial)
                return std::unexpected(partial.error());

            const bool isComparison = (op == ">=" || op == "<=" || op == ">" || op == "<" || op == "=");
            if (partial->wildcard && isComparison && op != "=")
                return std::unexpected(VersionRangeError::WildcardWithOperator);

            SemVer lower(partial->parts[0], partial->parts[1], partial->parts[2]);
            if (!partial->prerelease.empty() || !partial->build.empty()) {
                // parsePartial already accepted the shape; re-parsing is how
                // the prerelease and build strings get onto the bound itself,
                // because only a complete version can carry them.
                const auto full = SemVer::parse(token);
                if (!full)
                    return std::unexpected(VersionRangeError::InvalidVersion);
                lower = *full;
            }

            std::uint32_t bumped = 0;

            if (op == ">=") {
                range._comparators.push_back({Op::GreaterEqual, lower});
            } else if (op == ">") {
                range._comparators.push_back({Op::Greater, lower});
            } else if (op == "<=") {
                range._comparators.push_back({Op::LessEqual, lower});
            } else if (op == "<") {
                range._comparators.push_back({Op::Less, lower});
            } else if (op == "~") {
                if (partial->specified == 0)
                    return std::unexpected(VersionRangeError::MissingVersion);
                range._comparators.push_back({Op::GreaterEqual, lower});
                if (partial->specified == 1) {
                    if (bump(partial->parts[0], bumped))
                        range._comparators.push_back({Op::Less, SemVer(bumped, 0, 0)});
                } else if (bump(partial->parts[1], bumped)) {
                    range._comparators.push_back({Op::Less, SemVer(partial->parts[0], bumped, 0)});
                }
            } else if (op == "^") {
                if (partial->specified == 0)
                    return std::unexpected(VersionRangeError::MissingVersion);
                range._comparators.push_back({Op::GreaterEqual, lower});
                // The caret pins the left-most non-zero component, but only
                // among the components that were actually written: "^0.x" is
                // "<1.0.0", while "^0.0.x" is "<0.1.0".
                const bool pinMajor = partial->specified == 1 || partial->parts[0] != 0;
                const bool pinMinor = !pinMajor && (partial->specified == 2 || partial->parts[1] != 0);
                if (pinMajor) {
                    if (bump(partial->parts[0], bumped))
                        range._comparators.push_back({Op::Less, SemVer(bumped, 0, 0)});
                } else if (pinMinor) {
                    if (bump(partial->parts[1], bumped))
                        range._comparators.push_back({Op::Less, SemVer(0, bumped, 0)});
                } else if (bump(partial->parts[2], bumped)) {
                    range._comparators.push_back({Op::Less, SemVer(0, 0, bumped)});
                }
            } else {
                // No operator, or an explicit "=". A complete version pins
                // exactly; anything partial or wildcarded widens to the range
                // the missing components imply.
                if (!partial->wildcard && partial->specified == 3) {
                    range._comparators.push_back({Op::Equal, lower});
                } else if (partial->specified == 0) {
                    // "*" / "x": no constraint at all.
                } else if (partial->specified == 1) {
                    range._comparators.push_back({Op::GreaterEqual, SemVer(partial->parts[0], 0, 0)});
                    if (bump(partial->parts[0], bumped))
                        range._comparators.push_back({Op::Less, SemVer(bumped, 0, 0)});
                } else {
                    range._comparators.push_back(
                        {Op::GreaterEqual, SemVer(partial->parts[0], partial->parts[1], 0)});
                    if (bump(partial->parts[1], bumped))
                        range._comparators.push_back({Op::Less, SemVer(partial->parts[0], bumped, 0)});
                }
            }
        }

        if (!sawToken)
            return std::unexpected(VersionRangeError::Empty);

        return range;
    }

    bool VersionRange::matches(const SemVer& version) const noexcept
    {
        for (const Comparator& comparator : _comparators) {
            const std::strong_ordering order = version.compare(comparator.bound);
            bool ok = false;
            switch (comparator.op) {
            case Op::Equal:
                ok = (order == std::strong_ordering::equal);
                break;
            case Op::Less:
                ok = (order == std::strong_ordering::less);
                break;
            case Op::LessEqual:
                ok = (order != std::strong_ordering::greater);
                break;
            case Op::Greater:
                ok = (order == std::strong_ordering::greater);
                break;
            case Op::GreaterEqual:
                ok = (order != std::strong_ordering::less);
                break;
            }
            if (!ok)
                return false;
        }
        return true;
    }

    bool VersionRange::exceededBy(const SemVer& version) const noexcept
    {
        for (const Comparator& comparator : _comparators) {
            const std::strong_ordering order = version.compare(comparator.bound);
            switch (comparator.op) {
            case Op::Equal:
            case Op::LessEqual:
                if (order == std::strong_ordering::greater)
                    return true;
                break;
            case Op::Less:
                if (order != std::strong_ordering::less)
                    return true;
                break;
            // A lower bound can never be overshot.
            case Op::Greater:
            case Op::GreaterEqual:
                break;
            }
        }
        return false;
    }

} // namespace crabe::domain
