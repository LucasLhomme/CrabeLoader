/*
** CrabeLoader
** File description:
** Runs the update check against whatever source, prompt and opener it is given.
** Every failure ends the check quietly in the log; only a real update ever reaches the player.
** Owns no thread and no state between runs, so declining lasts exactly until the next launch.
**
** Authors: @LucasLhomme
*/

#include "application/update_checker.hpp"

#include <utility>

#include "shared/logger.hpp"

namespace crabe::application {

    std::string_view describe(UpdateOutcome outcome) noexcept
    {
        switch (outcome) {
        case UpdateOutcome::InvalidCurrentVersion:
            return "the running version is not a semantic version";
        case UpdateOutcome::FetchFailed:
            return "the latest release could not be fetched";
        case UpdateOutcome::Unreadable:
            return "the latest release could not be read";
        case UpdateOutcome::UpToDate:
            return "already up to date";
        case UpdateOutcome::Declined:
            return "update declined by the player";
        case UpdateOutcome::PageOpened:
            return "release page opened";
        case UpdateOutcome::PageFailed:
            return "the release page could not be opened";
        }
        return "unknown";
    }

    UpdateChecker::UpdateChecker(domain::IReleaseSource& source,
                                 domain::IUpdatePrompt& prompt,
                                 domain::IUrlOpener& opener,
                                 std::string currentVersion)
        : _source(source), _prompt(prompt), _opener(opener), _currentVersion(std::move(currentVersion))
    {
    }

    UpdateOutcome UpdateChecker::run() const
    {
        crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

        const auto current = domain::SemVer::parse(_currentVersion);
        if (!current) {
            logger.warning("UpdateChecker: running version '{}' is not a semantic version; check skipped.",
                           _currentVersion);
            return UpdateOutcome::InvalidCurrentVersion;
        }

        const auto body = _source.fetchLatestRelease();
        if (!body) {
            logger.info("UpdateChecker: could not reach the release server ({}); check skipped.", body.error());
            return UpdateOutcome::FetchFailed;
        }

        const auto latest = domain::parseLatestRelease(*body);
        if (!latest) {
            logger.warning("UpdateChecker: {}; check skipped.", domain::describe(latest.error()));
            return UpdateOutcome::Unreadable;
        }

        if (!domain::isUpdateAvailable(*current, *latest)) {
            logger.info("UpdateChecker: v{} is up to date (latest published: {}).", _currentVersion, latest->tag);
            return UpdateOutcome::UpToDate;
        }

        logger.info("UpdateChecker: update available: v{} -> {}.", _currentVersion, latest->tag);
        if (!_prompt.askToUpdate(_currentVersion, latest->version.toString())) {
            logger.info("UpdateChecker: update declined; the player will be asked again at the next launch.");
            return UpdateOutcome::Declined;
        }

        if (!_opener.open(domain::kReleasesPageUrl)) {
            logger.warning("UpdateChecker: could not open {}.", domain::kReleasesPageUrl);
            return UpdateOutcome::PageFailed;
        }
        return UpdateOutcome::PageOpened;
    }

} // namespace crabe::application
