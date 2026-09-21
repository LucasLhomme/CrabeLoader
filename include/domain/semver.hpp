#ifndef CRABELOADER_DOMAIN_SEMVER_HPP_
#define CRABELOADER_DOMAIN_SEMVER_HPP_

// Semantic versions and version ranges.
//
// Deliberately pure: no I/O, no logging, no Windows headers, no Lua. The
// dependency resolver links this and nothing else, so anything that drags in
// the loader's runtime does not belong here.
//
// Note there is a second, simpler SemVer in src/shared/version.cpp, private to
// crabe::version::isCompatible. It is left alone on purpose: `shared` sits
// below `domain` and must not depend on it.

#include <compare>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace crabe::domain {

    // Why a version string was rejected. Each value names one concrete defect
    // so the caller can say what to fix rather than "invalid version".
    enum class SemVerError {
        Empty,              // "" or only a leading 'v'
        MissingMinor,       // "1"
        MissingPatch,       // "1.2"
        NotANumber,         // "a.2.3", "1..3"
        LeadingZero,        // "01.2.3" -- forbidden by the specification
        NumericOverflow,    // a component wider than uint32_t
        EmptyIdentifier,    // "1.2.3-", "1.2.3-alpha..1", "1.2.3+"
        InvalidCharacter,   // "1.2.3-bêta", "1.2.3_4"
        TrailingCharacters, // "1.2.3 extra"
    };

    // A short sentence, lower case and without trailing punctuation, meant to
    // be embedded in a larger message next to the offending text.
    [[nodiscard]] std::string_view describe(SemVerError error) noexcept;

    // MAJOR.MINOR.PATCH[-prerelease][+build], as per semver.org 2.0.0.
    //
    // One deviation: a leading 'v' or 'V' is accepted and discarded, because
    // git tags and the loader's own release names carry one and rejecting
    // "v0.3.0" in a manifest would be pedantry rather than a service.
    class SemVer {
    public:
        constexpr SemVer() noexcept = default;

        constexpr SemVer(std::uint32_t majorPart,
                         std::uint32_t minorPart,
                         std::uint32_t patchPart) noexcept
            : _major(majorPart), _minor(minorPart), _patch(patchPart)
        {
        }

        // Strict: all three numeric components are required. Callers that want
        // "1.2" to mean "1.2.anything" are expressing a range, and a range is
        // what VersionRange parses.
        [[nodiscard]] static std::expected<SemVer, SemVerError> parse(std::string_view text);

        [[nodiscard]] constexpr std::uint32_t major() const noexcept { return _major; }
        [[nodiscard]] constexpr std::uint32_t minor() const noexcept { return _minor; }
        [[nodiscard]] constexpr std::uint32_t patch() const noexcept { return _patch; }

        [[nodiscard]] const std::string& prerelease() const noexcept { return _prerelease; }
        [[nodiscard]] const std::string& build() const noexcept { return _build; }
        [[nodiscard]] bool isPrerelease() const noexcept { return !_prerelease.empty(); }

        // Precedence as the specification defines it: build metadata is
        // ignored, and a prerelease sorts below the release it precedes.
        [[nodiscard]] std::strong_ordering compare(const SemVer& other) const noexcept;

        [[nodiscard]] std::string toString() const;

        friend std::strong_ordering operator<=>(const SemVer& lhs, const SemVer& rhs) noexcept
        {
            return lhs.compare(rhs);
        }

        // Equality is precedence equality, so 1.2.3+a == 1.2.3+b. Use
        // toString() comparison when the build metadata matters.
        friend bool operator==(const SemVer& lhs, const SemVer& rhs) noexcept
        {
            return lhs.compare(rhs) == std::strong_ordering::equal;
        }

    private:
        std::uint32_t _major{0};
        std::uint32_t _minor{0};
        std::uint32_t _patch{0};
        std::string _prerelease;
        std::string _build;
    };

    // Why a range expression was rejected.
    enum class VersionRangeError {
        Empty,              // "" -- say "*" if anything goes
        UnknownOperator,    // "!1.2.3", "=>1.2.3"
        MissingVersion,     // ">=", "~"
        InvalidVersion,     // ">=1.2.beta"
        MisplacedWildcard,  // "1.x.3" -- nothing may follow an 'x'
        WildcardWithOperator, // ">=1.x" -- an operator needs a concrete bound
        UnsupportedOrClause,  // "1.x || 2.x" -- see the note on VersionRange
    };

    [[nodiscard]] std::string_view describe(VersionRangeError error) noexcept;

    // A conjunction of comparators: every one must hold.
    //
    // Accepted forms, separated by spaces and/or commas:
    //
    //   *  x  X            anything
    //   1.x  1.*  1        >=1.0.0 <2.0.0
    //   1.2.x  1.2         >=1.2.0 <1.3.0
    //   1.2.3              exactly 1.2.3
    //   =1.2.3             exactly 1.2.3
    //   >=1.2.0  >1.2.0  <=2.0.0  <2.0.0
    //   ~1.2.3             >=1.2.3 <1.3.0      (patch-level drift)
    //   ~1.2               >=1.2.0 <1.3.0
    //   ~1                 >=1.0.0 <2.0.0
    //   ^1.2.3             >=1.2.3 <2.0.0      (npm caret: the left-most
    //   ^0.2.3             >=0.2.3 <0.3.0       non-zero component is pinned)
    //   ^0.0.3             >=0.0.3 <0.0.4
    //   ">=1.0.0 <2.0.0"   both, ANDed
    //
    // Two deliberate simplifications, both of which T9 should revisit if its
    // resolver needs more:
    //
    //  * `||` is rejected rather than parsed. A half-understood disjunction
    //    that silently matched the wrong thing is worse than a clear refusal.
    //  * A partial version after a comparison operator is zero-filled, so
    //    ">1.2" means ">1.2.0" and does match 1.2.1. npm reads it as ">=1.3.0".
    //    Zero-filling is the rule everywhere else here, so it is the rule here
    //    too; write ">=1.3.0" when that is what you mean.
    //
    // Prereleases are matched purely on precedence: ">=1.0.0" is satisfied by
    // 2.0.0-beta, because 2.0.0-beta > 1.0.0. npm would exclude it. Ranges in
    // manifests gate compatibility, not release channels, so the simpler rule
    // is the honest one here -- and it is the rule T9 will inherit.
    class VersionRange {
    public:
        // Default-constructed means "*": it matches every version.
        VersionRange() = default;

        [[nodiscard]] static std::expected<VersionRange, VersionRangeError> parse(
            std::string_view text);

        [[nodiscard]] bool matches(const SemVer& version) const noexcept;

        // True when `version` is above everything the range admits, ignoring
        // the range's lower bounds entirely. This is what a *ceiling* means:
        // "maxLoaderVersion": "1.x" says 1.anything is the newest loader the
        // mod was tested against, so 2.0.0 is out and 0.9.0 is fine even
        // though it does not match the range. matches() cannot express that,
        // because "1.x" desugars to ">=1.0.0 <2.0.0" and would reject 0.9.0.
        [[nodiscard]] bool exceededBy(const SemVer& version) const noexcept;

        // The expression as written, for error messages and round-tripping.
        [[nodiscard]] const std::string& text() const noexcept { return _text; }

        // True when no comparator constrains anything.
        [[nodiscard]] bool isAny() const noexcept { return _comparators.empty(); }

    private:
        enum class Op : std::uint8_t {
            Equal,
            Less,
            LessEqual,
            Greater,
            GreaterEqual,
        };

        struct Comparator {
            Op op{Op::Equal};
            SemVer bound;
        };

        std::vector<Comparator> _comparators;
        std::string _text{"*"};
    };

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_SEMVER_HPP_ */
