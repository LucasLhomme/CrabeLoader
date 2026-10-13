/*
** CrabeLoader
** File description:
** Declares the HTTP host redirect rules: which server names go elsewhere, and where.
** Pure: parses "http://host[:port]" targets and matches server names; no socket, no hook.
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

// Where a redirected connection goes. secure = false is plain HTTP: the request
// loses its TLS flag, so a local server needs no certificate.
struct HttpRedirectTarget {
    std::wstring host;
    std::uint16_t port{80};
    bool secure{false};

    [[nodiscard]] bool operator==(const HttpRedirectTarget&) const = default;
};

// "http://host[:port]" or "https://host[:port]", an optional trailing "/" and nothing after it.
// Host names are ASCII letters, digits, '.' and '-'.
[[nodiscard]] std::expected<HttpRedirectTarget, std::string> parseHttpRedirectTarget(std::string_view url);

// ASCII-only narrowing for log lines; anything else becomes '?'.
[[nodiscard]] std::string narrowForLog(std::wstring_view text);

// Exact, case-insensitive server name -> target table.
class HttpRedirectRules {
    public:
        // Adds nothing and returns why when the host or the target is invalid.
        std::expected<void, std::string> add(std::string_view host, std::string_view url);

        [[nodiscard]] std::optional<HttpRedirectTarget> find(std::wstring_view host) const;
        [[nodiscard]] bool empty() const noexcept { return _rules.empty(); }
        [[nodiscard]] std::size_t size() const noexcept { return _rules.size(); }

    private:
        std::unordered_map<std::wstring, HttpRedirectTarget> _rules;
};

} // namespace crabe::infrastructure

#endif /* !HTTP_REDIRECT_RULES_HPP_ */
