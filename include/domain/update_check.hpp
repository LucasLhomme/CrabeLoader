/*
** CrabeLoader
** File description:
** Declares the pure half of the update check: reading a GitHub release and comparing versions.
** Also declares the three ports the check talks through, so it runs offline in a test.
** Performs no network call and shows no dialog; infrastructure/update_services.hpp does both.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_DOMAIN_UPDATE_CHECK_HPP_
#define CRABELOADER_DOMAIN_UPDATE_CHECK_HPP_

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

#include "domain/semver.hpp"

namespace crabe::domain {

    constexpr std::string_view kReleaseApiHost = "api.github.com";
    constexpr std::string_view kLatestReleasePath = "/repos/LucasLhomme/CrabeLoader/releases/latest";
    constexpr std::string_view kReleasesPageUrl = "https://github.com/LucasLhomme/CrabeLoader/releases/latest";

    enum class ReleaseParseError : std::uint8_t {
        NotJson,
        NotAnObject,
        MissingTag,
        InvalidTag,
    };

    // Returns a readable description of a release parse error.
    [[nodiscard]] std::string_view describe(ReleaseParseError error) noexcept;

    struct ReleaseInfo {
        SemVer version;
        std::string tag;
        bool draft{false};
        bool prerelease{false};
    };

    // Parses a GitHub release response payload into ReleaseInfo.
    [[nodiscard]] std::expected<ReleaseInfo, ReleaseParseError> parseLatestRelease(std::string_view body);

    // Returns true if the release is published, stable, and newer than current.
    [[nodiscard]] bool isUpdateAvailable(const SemVer& current, const ReleaseInfo& latest) noexcept;

    class IReleaseSource {
    public:
        virtual ~IReleaseSource() = default;

        // Fetches the latest release payload string from GitHub.
        [[nodiscard]] virtual std::expected<std::string, std::string> fetchLatestRelease() = 0;
    };

    class IUpdatePrompt {
    public:
        virtual ~IUpdatePrompt() = default;

        // Prompts the user to update to the newer release.
        [[nodiscard]] virtual bool askToUpdate(std::string_view currentVersion, std::string_view latestVersion) = 0;
    };

    class IUrlOpener {
    public:
        virtual ~IUrlOpener() = default;

        // Opens the specified URL in the system browser.
        [[nodiscard]] virtual bool open(std::string_view url) = 0;
    };

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_UPDATE_CHECK_HPP_ */
