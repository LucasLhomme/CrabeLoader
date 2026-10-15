/*
** CrabeLoader
** File description:
** Declares semantic versions and the version ranges a manifest is allowed to write.
** Deliberately pure, which is what makes every rule below a unit test instead of a session.
** A second, simpler SemVer lives in src/shared/version.cpp; shared must not depend on domain.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_DOMAIN_SEMVER_HPP_
#define CRABELOADER_DOMAIN_SEMVER_HPP_

#include <compare>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace crabe::domain {

    enum class SemVerError {
        Empty,
        MissingMinor,
        MissingPatch,
        NotANumber,
        LeadingZero,
        NumericOverflow,
        EmptyIdentifier,
        InvalidCharacter,
        TrailingCharacters,
    };

    // Returns a readable description of a semantic version parsing error.
    [[nodiscard]] std::string_view describe(SemVerError error) noexcept;

    class SemVer {
    public:
        constexpr SemVer() noexcept = default;

        constexpr SemVer(std::uint32_t majorPart,
                         std::uint32_t minorPart,
                         std::uint32_t patchPart) noexcept
            : _major(majorPart), _minor(minorPart), _patch(patchPart)
        {
        }

        // Parses a semantic version string in MAJOR.MINOR.PATCH format.
        [[nodiscard]] static std::expected<SemVer, SemVerError> parse(std::string_view text);

        [[nodiscard]] constexpr std::uint32_t major() const noexcept { return _major; }
        [[nodiscard]] constexpr std::uint32_t minor() const noexcept { return _minor; }
        [[nodiscard]] constexpr std::uint32_t patch() const noexcept { return _patch; }

        [[nodiscard]] const std::string& prerelease() const noexcept { return _prerelease; }
        [[nodiscard]] const std::string& build() const noexcept { return _build; }
        [[nodiscard]] bool isPrerelease() const noexcept { return !_prerelease.empty(); }

        // Compares two semantic versions according to semver 2.0.0 precedence rules.
        [[nodiscard]] std::strong_ordering compare(const SemVer& other) const noexcept;

        // Formats the semantic version into its canonical string representation.
        [[nodiscard]] std::string toString() const;

        friend std::strong_ordering operator<=>(const SemVer& lhs, const SemVer& rhs) noexcept
        {
            return lhs.compare(rhs);
        }

        // Returns true if both versions have equal precedence.
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

    enum class VersionRangeError {
        Empty,
        UnknownOperator,
        MissingVersion,
        InvalidVersion,
        MisplacedWildcard,
        WildcardWithOperator,
        UnsupportedOrClause,
    };

    // Returns a readable description of a version range parsing error.
    [[nodiscard]] std::string_view describe(VersionRangeError error) noexcept;

    class VersionRange {
    public:
        // Constructs a wildcard range matching any version.
        VersionRange() = default;

        // Parses a version range constraint expression.
        [[nodiscard]] static std::expected<VersionRange, VersionRangeError> parse(
            std::string_view text);

        // Evaluates whether a semantic version satisfies this range constraint.
        [[nodiscard]] bool matches(const SemVer& version) const noexcept;

        // Returns true if the version strictly exceeds the range ceiling.
        [[nodiscard]] bool exceededBy(const SemVer& version) const noexcept;

        // Returns the original range specification text.
        [[nodiscard]] const std::string& text() const noexcept { return _text; }

        // Returns true if the range imposes no constraints.
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
