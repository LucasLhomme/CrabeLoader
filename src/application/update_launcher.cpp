/*
** CrabeLoader
** File description:
** Starts the update check on a detached thread so the game never waits for the network or the box.
** The thread owns everything it uses and touches neither Lua nor Direct3D.
** Contains no check logic; application/update_checker.cpp does.
**
** Authors: @LucasLhomme
*/

#include "application/update_launcher.hpp"

#include <atomic>
#include <string>
#include <thread>

#include "application/update_checker.hpp"
#include "infrastructure/crash_handler.hpp"
#include "infrastructure/crash_reporter.hpp"
#include "infrastructure/update_services.hpp"
#include "shared/logger.hpp"
#include "shared/version.hpp"

namespace crabe::application {

    namespace {

        std::atomic_flag s_started;

        void runUpdateCheck()
        {
            crabe::infrastructure::CrashReporter::declareThreadRole(crabe::infrastructure::ThreadRole::Worker);

            crabe::infrastructure::CrashHandler::runGuarded([] {
                crabe::infrastructure::WinHttpReleaseSource source;
                crabe::infrastructure::MessageBoxUpdatePrompt prompt;
                crabe::infrastructure::ShellUrlOpener opener;
                const UpdateChecker checker(source, prompt, opener, std::string(crabe::version::String));

                const UpdateOutcome outcome = checker.run();
                crabe::shared::Logger::getInstance().debug("UpdateChecker: finished: {}.", describe(outcome));
            }, "UpdateChecker");
        }

    } // namespace

    void startUpdateCheckInBackground(bool enabled)
    {
        crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

        if (!enabled) {
            logger.info("UpdateChecker: disabled by crabe.toml ([updates].check = false).");
            return;
        }
        if (s_started.test_and_set()) {
            logger.debug("UpdateChecker: already started in this process.");
            return;
        }

        std::thread(runUpdateCheck).detach();
    }

} // namespace crabe::application
