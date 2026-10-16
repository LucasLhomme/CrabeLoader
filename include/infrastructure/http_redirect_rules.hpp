/*
** CrabeLoader
** File description:
** Declares the HTTP host redirect rules: which server names go elsewhere, and where.
** Parses "http(s)://host[:port]" targets and matches wide server names case-insensitively.
** Pure data structures with no sockets or hooks; installation is deferred to HttpRedirect.
**
** Authors: @LucasLhomme
*/

#ifndef HTTP_REDIRECT_RULES_HPP_
#define HTTP_REDIRECT_RULES_HPP_

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace crabe::infrastructure {

struct HttpRedirectTarget {
    std::wstring host;
    std::uint16_t port{80};
    bool secure{false};

    [[nodiscard]] bool operator==(const HttpRedirectTarget&) const = default;
};

// Parses a target URL string into an HttpRedirectTarget.
// Returns an error string if the target URL is malformed.
[[nodiscard]] std::expected<HttpRedirectTarget, std::string> parseHttpRedirectTarget(std::string_view url);

// Narrows a wide string to ASCII for logging, replacing non-ASCII characters with '?'.
[[nodiscard]] std::string narrowForLog(std::wstring_view text);

class HttpRedirectRules {
    public:
        // Adds a redirect rule mapping a host to a target URL.
        // Returns an error string if host or target URL is invalid.
        std::expected<void, std::string> add(std::string_view host, std::string_view url);

        // Finds a redirect target matching the given host case-insensitively.
        [[nodiscard]] std::optional<HttpRedirectTarget> find(std::wstring_view host) const;

        // Checks whether the rule table is empty.
        [[nodiscard]] bool empty() const noexcept { return _rules.empty(); }

        // Returns the number of redirection rules.
        [[nodiscard]] std::size_t size() const noexcept { return _rules.size(); }

    private:
        std::unordered_map<std::wstring, HttpRedirectTarget> _rules;
};

} // namespace crabe::infrastructure

#endif /* !HTTP_REDIRECT_RULES_HPP_ */
