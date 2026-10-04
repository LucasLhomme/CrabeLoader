#include <cstring>
#include <iostream>
#include <string>
#include <string_view>

#include "application/update_checker.hpp"
#include "infrastructure/update_services.hpp"
#include "shared/version.hpp"

namespace {

using namespace crabe;

class FixedSource final : public domain::IReleaseSource {
public:
    explicit FixedSource(std::string tag) : _tag(std::move(tag)) {}

    std::expected<std::string, std::string> fetchLatestRelease() override
    {
        return "{\"tag_name\":\"" + _tag + "\",\"draft\":false,\"prerelease\":false}";
    }

private:
    std::string _tag;
};

class PrintingOpener final : public domain::IUrlOpener {
public:
    bool open(std::string_view url) override
    {
        std::cout << "[probe] would open: " << url << std::endl;
        return true;
    }
};

void usage()
{
    std::cout << "update_probe [--current X.Y.Z] [--fake-latest vX.Y.Z] [--no-open]\n"
                 "  Runs the real update check outside the game.\n"
                 "  --current      version to pretend to run (default: this build)\n"
                 "  --fake-latest  skip the network and pretend GitHub published this tag\n"
                 "  --no-open      print the URL instead of opening the browser on Yes\n";
}

} // namespace

int main(int argc, char** argv)
{
    std::string current(crabe::version::String);
    std::string fakeLatest;
    bool noOpen = false;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--current" && i + 1 < argc) {
            current = argv[++i];
        } else if (arg == "--fake-latest" && i + 1 < argc) {
            fakeLatest = argv[++i];
        } else if (arg == "--no-open") {
            noOpen = true;
        } else {
            usage();
            return arg == "--help" ? 0 : 2;
        }
    }

    infrastructure::WinHttpReleaseSource networkSource;
    FixedSource fixedSource(fakeLatest);
    domain::IReleaseSource& source = fakeLatest.empty() ? static_cast<domain::IReleaseSource&>(networkSource)
                                                        : static_cast<domain::IReleaseSource&>(fixedSource);

    infrastructure::MessageBoxUpdatePrompt prompt;
    infrastructure::ShellUrlOpener shellOpener;
    PrintingOpener printingOpener;
    domain::IUrlOpener& opener = noOpen ? static_cast<domain::IUrlOpener&>(printingOpener)
                                        : static_cast<domain::IUrlOpener&>(shellOpener);

    const application::UpdateChecker checker(source, prompt, opener, current);
    const application::UpdateOutcome outcome = checker.run();
    std::cout << "[probe] outcome: " << application::describe(outcome) << std::endl;
    return 0;
}
