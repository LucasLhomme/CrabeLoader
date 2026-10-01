/*
** CrabeLoader
** File description:
** Reads a GitHub release body and decides whether it is newer than the running build.
** A draft, a prerelease, or a tag that is not a version is never offered to the player.
** Opens no socket and no window; it only reads text it is handed.
**
** Authors: @LucasLhomme
*/

#include "domain/update_check.hpp"

#include <utility>

#include "third_party/json.hpp"

namespace crabe::domain {

    std::string_view describe(ReleaseParseError error) noexcept
    {
        switch (error) {
        case ReleaseParseError::NotJson:
            return "the response is not valid JSON";
        case ReleaseParseError::NotAnObject:
            return "the response is not a JSON object";
        case ReleaseParseError::MissingTag:
            return "the response has no tag_name";
        case ReleaseParseError::InvalidTag:
            return "the release tag is not a semantic version";
        }
        return "the release could not be read";
    }

    std::expected<ReleaseInfo, ReleaseParseError> parseLatestRelease(std::string_view body)
    {
        const nlohmann::json root = nlohmann::json::parse(body.begin(), body.end(), nullptr, false);
        if (root.is_discarded())
            return std::unexpected(ReleaseParseError::NotJson);
        if (!root.is_object())
            return std::unexpected(ReleaseParseError::NotAnObject);

        const auto tagIt = root.find("tag_name");
        if (tagIt == root.end() || !tagIt->is_string())
            return std::unexpected(ReleaseParseError::MissingTag);

        ReleaseInfo info;
        info.tag = tagIt->get<std::string>();

        auto version = SemVer::parse(info.tag);
        if (!version)
            return std::unexpected(ReleaseParseError::InvalidTag);
        info.version = std::move(*version);

        const auto draftIt = root.find("draft");
        info.draft = draftIt != root.end() && draftIt->is_boolean() && draftIt->get<bool>();

        const auto prereleaseIt = root.find("prerelease");
        info.prerelease = prereleaseIt != root.end() && prereleaseIt->is_boolean() && prereleaseIt->get<bool>();

        return info;
    }

    bool isUpdateAvailable(const SemVer& current, const ReleaseInfo& latest) noexcept
    {
        if (latest.draft || latest.prerelease || latest.version.isPrerelease())
            return false;
        return latest.version > current;
    }

} // namespace crabe::domain
