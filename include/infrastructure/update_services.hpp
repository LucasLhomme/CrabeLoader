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

    class WinHttpReleaseSource final : public domain::IReleaseSource {
    public:
        // Fetches latest release JSON metadata from GitHub via WinHTTP.
        [[nodiscard]] std::expected<std::string, std::string> fetchLatestRelease() override;
    };

    class MessageBoxUpdatePrompt final : public domain::IUpdatePrompt {
    public:
        // Displays a native update confirmation message box.
        [[nodiscard]] bool askToUpdate(std::string_view currentVersion, std::string_view latestVersion) override;
    };

    class ShellUrlOpener final : public domain::IUrlOpener {
    public:
        // Opens an HTTPS URL in the default web browser.
        [[nodiscard]] bool open(std::string_view url) override;
    };

} // namespace crabe::infrastructure

#endif /* !CRABELOADER_INFRASTRUCTURE_UPDATE_SERVICES_HPP_ */
