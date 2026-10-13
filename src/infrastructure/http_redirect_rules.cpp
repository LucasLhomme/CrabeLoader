/*
** CrabeLoader
** File description:
** Parses HTTP redirect targets and matches server names against configured redirect rules.
** Validates host syntax and ports, normalising hostnames to lowercase for lookup.
** Pure string operations with no sockets or WinHTTP calls; unit-tested offline.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/http_redirect_rules.hpp"

#include <algorithm>
#include <charconv>
#include <format>

namespace crabe::infrastructure {

namespace {

constexpr std::string_view kHttp = "http://";
constexpr std::string_view kHttps = "https://";
constexpr std::size_t kMaxHostLength = 253;

// True for a non-empty ASCII host name made of letters, digits, '.' and '-'.
bool isValidHost(std::string_view host)
{
    if (host.empty() || host.size() > kMaxHostLength)
        return false;
    return std::ranges::all_of(host, [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-';
    });
}

// Lower-cases an ASCII string while widening it; callers have validated it first.
std::wstring widenLower(std::string_view text)
{
    std::wstring out;
    out.reserve(text.size());
    for (char c : text)
        out.push_back(static_cast<wchar_t>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c));
    return out;
}

// Lower-cases a wide server name for lookup.
std::wstring lowerWide(std::wstring_view text)
{
    std::wstring out(text);
    std::ranges::transform(out, out.begin(), [](wchar_t c) {
        return c >= L'A' && c <= L'Z' ? static_cast<wchar_t>(c - L'A' + L'a') : c;
    });
    return out;
}

} // namespace

std::expected<HttpRedirectTarget, std::string> parseHttpRedirectTarget(std::string_view url)
{
    HttpRedirectTarget target;
    std::string_view rest;
    if (url.starts_with(kHttp)) {
        rest = url.substr(kHttp.size());
        target.port = 80;
        target.secure = false;
    } else if (url.starts_with(kHttps)) {
        rest = url.substr(kHttps.size());
        target.port = 443;
        target.secure = true;
    } else {
        return std::unexpected(std::format("'{}' must start with http:// or https://", url));
    }

    if (rest.ends_with('/'))
        rest.remove_suffix(1);
    if (rest.find_first_of("/?#") != std::string_view::npos)
        return std::unexpected(std::format("'{}' must not have a path: only the host and port are replaced", url));

    std::string_view host = rest;
    const std::size_t colon = rest.rfind(':');
    if (colon != std::string_view::npos) {
        host = rest.substr(0, colon);
        const std::string_view portText = rest.substr(colon + 1);
        unsigned int port = 0;
        const auto [end, error] = std::from_chars(portText.data(), portText.data() + portText.size(), port);
        if (portText.empty() || error != std::errc{} || end != portText.data() + portText.size() || port == 0 ||
            port > 65535)
            return std::unexpected(std::format("'{}' has an invalid port", url));
        target.port = static_cast<std::uint16_t>(port);
    }

    if (!isValidHost(host))
        return std::unexpected(std::format("'{}' has an invalid host", url));
    target.host = widenLower(host);
    return target;
}

std::string narrowForLog(std::wstring_view text)
{
    std::string out;
    out.reserve(text.size());
    for (wchar_t c : text)
        out.push_back(c >= 0x20 && c < 0x7F ? static_cast<char>(c) : '?');
    return out;
}

std::expected<void, std::string> HttpRedirectRules::add(std::string_view host, std::string_view url)
{
    if (!isValidHost(host))
        return std::unexpected(std::format("'{}' is not a valid server name", host));
    auto target = parseHttpRedirectTarget(url);
    if (!target)
        return std::unexpected(target.error());
    _rules.insert_or_assign(widenLower(host), std::move(*target));
    return {};
}

std::optional<HttpRedirectTarget> HttpRedirectRules::find(std::wstring_view host) const
{
    if (host.empty())
        return std::nullopt;
    const auto it = _rules.find(lowerWide(host));
    if (it == _rules.end())
        return std::nullopt;
    return it->second;
}

} // namespace crabe::infrastructure
