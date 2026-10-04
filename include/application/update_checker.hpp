/*
** CrabeLoader
** File description:
** Declares the update check: fetch the latest release, compare, ask once, open the page.
** Talks only to the ports in domain/update_check.hpp, so a test drives it with fakes.
** Starts no thread; update_launcher.hpp decides when and where it runs.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_APPLICATION_UPDATE_CHECKER_HPP_
#define CRABELOADER_APPLICATION_UPDATE_CHECKER_HPP_

#include <cstdint>
#include <string>
#include <string_view>

#include "domain/update_check.hpp"

namespace crabe::application {

    enum class UpdateOutcome : std::uint8_t {
        InvalidCurrentVersion,
        FetchFailed,
        Unreadable,
        UpToDate,
        Declined,
        PageOpened,
        PageFailed,
    };

    [[nodiscard]] std::string_view describe(UpdateOutcome outcome) noexcept;

    class UpdateChecker final {
    public:
        UpdateChecker(domain::IReleaseSource& source,
                      domain::IUpdatePrompt& prompt,
                      domain::IUrlOpener& opener,
                      std::string currentVersion);

        // Runs the whole check once and never throws. The player is asked at most once per call.
        [[nodiscard]] UpdateOutcome run() const;

    private:
        domain::IReleaseSource& _source;
        domain::IUpdatePrompt& _prompt;
        domain::IUrlOpener& _opener;
        std::string _currentVersion;
    };

} // namespace crabe::application

#endif /* !CRABELOADER_APPLICATION_UPDATE_CHECKER_HPP_ */
