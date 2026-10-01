/*
** CrabeLoader
** File description:
** Declares the Windows side of the update check: a WinHTTP fetch, a message box, the shell open.
** Each class implements one port from domain/update_check.hpp and nothing else.
** Decides nothing about versions or when to run; application/update_checker.hpp does.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_UPDATE_SERVICES_HPP_
#define CRABELOADER_INFRASTRUCTURE_UPDATE_SERVICES_HPP_

#include <expected>
#include <string>
#include <string_view>

#include "domain/update_check.hpp"

namespace crabe::infrastructure {

    // GET https://api.github.com/.../releases/latest with short timeouts and a capped body.
    class WinHttpReleaseSource final : public domain::IReleaseSource {
    public:
        [[nodiscard]] std::expected<std::string, std::string> fetchLatestRelease() override;
    };

    // A native Yes/No box shown above the game, in French or English by the Windows UI language.
    class MessageBoxUpdatePrompt final : public domain::IUpdatePrompt {
    public:
        [[nodiscard]] bool askToUpdate(std::string_view currentVersion, std::string_view latestVersion) override;
    };

    // Opens an https URL in the default browser; anything else is refused.
    class ShellUrlOpener final : public domain::IUrlOpener {
    public:
        [[nodiscard]] bool open(std::string_view url) override;
    };

} // namespace crabe::infrastructure

#endif /* !CRABELOADER_INFRASTRUCTURE_UPDATE_SERVICES_HPP_ */
