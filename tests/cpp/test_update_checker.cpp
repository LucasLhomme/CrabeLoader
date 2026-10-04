#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

#include "application/update_checker.hpp"
#include "domain/config.hpp"
#include "domain/update_check.hpp"

namespace {

using namespace crabe;

/// Asserts that a boolean condition is satisfied or exits immediately.
void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "[FAIL] " << message << std::endl;
        std::exit(1);
    }
}

class FakeSource final : public domain::IReleaseSource {
public:
    explicit FakeSource(std::expected<std::string, std::string> response) : _response(std::move(response)) {}

    std::expected<std::string, std::string> fetchLatestRelease() override
    {
        ++calls;
        return _response;
    }

    int calls{0};

private:
    std::expected<std::string, std::string> _response;
};

class FakePrompt final : public domain::IUpdatePrompt {
public:
    explicit FakePrompt(bool answer) : _answer(answer) {}

    bool askToUpdate(std::string_view current, std::string_view latest) override
    {
        ++calls;
        lastCurrent = std::string(current);
        lastLatest = std::string(latest);
        return _answer;
    }

    int calls{0};
    std::string lastCurrent;
    std::string lastLatest;

private:
    bool _answer;
};

class FakeOpener final : public domain::IUrlOpener {
public:
    explicit FakeOpener(bool succeeds) : _succeeds(succeeds) {}

    bool open(std::string_view url) override
    {
        ++calls;
        lastUrl = std::string(url);
        return _succeeds;
    }

    int calls{0};
    std::string lastUrl;

private:
    bool _succeeds;
};

std::string release(std::string_view tag, bool draft = false, bool prerelease = false)
{
    return std::string("{\"tag_name\":\"") + std::string(tag) + "\",\"draft\":" + (draft ? "true" : "false")
         + ",\"prerelease\":" + (prerelease ? "true" : "false") + ",\"html_url\":\"https://evil.example/\"}";
}

application::UpdateOutcome runCheck(FakeSource& source, FakePrompt& prompt, FakeOpener& opener,
                                    std::string current = "0.2.0")
{
    const application::UpdateChecker checker(source, prompt, opener, std::move(current));
    return checker.run();
}

/// Verifies how a GitHub release body is read and what counts as an update.
void testParseAndCompare()
{
    auto parsed = domain::parseLatestRelease(release("v0.3.0"));
    require(parsed.has_value(), "a normal release body must parse");
    require(parsed->tag == "v0.3.0", "tag must be kept as written");
    require(parsed->version == domain::SemVer(0, 3, 0), "a leading v must be ignored");

    require(domain::parseLatestRelease("not json").error() == domain::ReleaseParseError::NotJson,
            "garbage must be NotJson");
    require(domain::parseLatestRelease("[1,2]").error() == domain::ReleaseParseError::NotAnObject,
            "an array must be NotAnObject");
    require(domain::parseLatestRelease("{}").error() == domain::ReleaseParseError::MissingTag,
            "no tag_name must be MissingTag");
    require(domain::parseLatestRelease("{\"tag_name\":42}").error() == domain::ReleaseParseError::MissingTag,
            "a non-string tag_name must be MissingTag");
    require(domain::parseLatestRelease("{\"tag_name\":\"nightly\"}").error() == domain::ReleaseParseError::InvalidTag,
            "a tag that is not a version must be InvalidTag");

    const domain::SemVer current(0, 2, 0);
    require(domain::isUpdateAvailable(current, *domain::parseLatestRelease(release("v0.2.1"))), "patch bump is an update");
    require(domain::isUpdateAvailable(current, *domain::parseLatestRelease(release("v1.0.0"))), "major bump is an update");
    require(!domain::isUpdateAvailable(current, *domain::parseLatestRelease(release("v0.2.0"))), "same version is not an update");
    require(!domain::isUpdateAvailable(current, *domain::parseLatestRelease(release("v0.1.9"))), "older release is not an update");
    require(!domain::isUpdateAvailable(current, *domain::parseLatestRelease(release("v0.3.0", true))), "a draft is never offered");
    require(!domain::isUpdateAvailable(current, *domain::parseLatestRelease(release("v0.3.0", false, true))), "a prerelease flag is never offered");
    require(!domain::isUpdateAvailable(current, *domain::parseLatestRelease(release("v0.3.0-rc.1"))), "a prerelease tag is never offered");
    require(domain::isUpdateAvailable(*domain::SemVer::parse("0.3.0-dev"), *domain::parseLatestRelease(release("v0.3.0"))),
            "a dev build of the next version is offered its own release");
}

/// Verifies the whole flow with fakes: who is asked, what is opened, and when nothing happens.
void testCheckerFlow()
{
    {
        FakeSource source(release("v0.3.0"));
        FakePrompt prompt(true);
        FakeOpener opener(true);
        require(runCheck(source, prompt, opener) == application::UpdateOutcome::PageOpened, "yes must open the page");
        require(prompt.calls == 1, "the player is asked exactly once");
        require(prompt.lastCurrent == "0.2.0" && prompt.lastLatest == "0.3.0", "the prompt shows both versions");
        require(opener.calls == 1, "the page is opened once");
        require(opener.lastUrl == std::string(domain::kReleasesPageUrl), "only the fixed releases page is opened");
    }
    {
        FakeSource source(release("v0.3.0"));
        FakePrompt prompt(false);
        FakeOpener opener(true);
        require(runCheck(source, prompt, opener) == application::UpdateOutcome::Declined, "no must be Declined");
        require(opener.calls == 0, "no must never open the browser");
    }
    {
        FakeSource source(release("v0.2.0"));
        FakePrompt prompt(true);
        FakeOpener opener(true);
        require(runCheck(source, prompt, opener) == application::UpdateOutcome::UpToDate, "same version is UpToDate");
        require(prompt.calls == 0 && opener.calls == 0, "an up to date build is never asked anything");
    }
    {
        FakeSource source(std::unexpected(std::string("offline")));
        FakePrompt prompt(true);
        FakeOpener opener(true);
        require(runCheck(source, prompt, opener) == application::UpdateOutcome::FetchFailed, "offline must be FetchFailed");
        require(prompt.calls == 0 && opener.calls == 0, "offline is silent");
    }
    {
        FakeSource source(std::string("<html>rate limited</html>"));
        FakePrompt prompt(true);
        FakeOpener opener(true);
        require(runCheck(source, prompt, opener) == application::UpdateOutcome::Unreadable, "a non JSON body must be Unreadable");
        require(prompt.calls == 0, "an unreadable body is silent");
    }
    {
        FakeSource source(release("v0.3.0"));
        FakePrompt prompt(true);
        FakeOpener opener(false);
        require(runCheck(source, prompt, opener) == application::UpdateOutcome::PageFailed, "a failing browser must be PageFailed");
    }
    {
        FakeSource source(release("v0.3.0"));
        FakePrompt prompt(true);
        FakeOpener opener(true);
        require(runCheck(source, prompt, opener, "garbage") == application::UpdateOutcome::InvalidCurrentVersion,
                "a broken running version must be InvalidCurrentVersion");
        require(source.calls == 0, "no request is made when the running version is unusable");
    }
    {
        FakeSource source(release("v0.3.0"));
        FakePrompt prompt(false);
        FakeOpener opener(true);
        const application::UpdateChecker checker(source, prompt, opener, "0.2.0");
        (void)checker.run();
        (void)checker.run();
        require(prompt.calls == 2, "a fresh run asks again: declining only lasts until the next launch");
    }
}

/// Verifies the [updates] section of crabe.toml: on by default, switchable off, no stray key.
void testConfigKey()
{
    const auto defaults = domain::Config::parse("", "crabe.toml");
    require(defaults.has_value() && defaults->updateCheckEnabled(), "the check is on by default");

    const auto withoutSection = domain::Config::parse("[general]\nlanguage = \"en\"\n", "crabe.toml");
    require(withoutSection.has_value() && withoutSection->updateCheckEnabled(),
            "a config written before [updates] existed keeps the check on");

    const auto off = domain::Config::parse("[updates]\ncheck = false\n", "crabe.toml");
    require(off.has_value() && !off->updateCheckEnabled(), "check = false must disable it");
    require(off->getIgnoredKeys().empty(), "[updates].check is a known key");

    const auto typo = domain::Config::parse("[updates]\nchek = false\n", "crabe.toml");
    require(typo.has_value() && typo->updateCheckEnabled(), "a typo must not disable the check");
    require(typo->getIgnoredKeys().size() == 1, "a typo must be reported as an ignored key");

    const auto generated = domain::Config::parse(domain::buildDefaultConfigText(), "crabe.toml");
    require(generated.has_value() && generated->updateCheckEnabled(), "the generated default file parses with the check on");
    require(generated->getIgnoredKeys().empty(), "the generated default file contains no unknown key");
}

} // namespace

int main()
{
    testParseAndCompare();
    testCheckerFlow();
    testConfigKey();
    std::cout << "[PASS] test_update_checker" << std::endl;
    return 0;
}
