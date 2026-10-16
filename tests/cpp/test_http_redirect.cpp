/*
** CrabeLoader
** File description:
** Unit tests for the HTTP redirect rules: target parsing, host validation and case-insensitive lookup.
** Runs offline as an independent C++23 test binary: no game process, no WinHTTP, no hook.
**
** Authors: @LucasLhomme
*/

#include <cstdlib>
#include <iostream>
#include <string>

#include "infrastructure/http_redirect_rules.hpp"

namespace {

using crabe::infrastructure::HttpRedirectRules;
using crabe::infrastructure::HttpRedirectTarget;
using crabe::infrastructure::parseHttpRedirectTarget;

int g_checks = 0;

void require(bool condition, const std::string& message)
{
    ++g_checks;
    if (!condition) {
        std::cerr << "[FAILED] " << message << std::endl;
        std::exit(1);
    }
}

void testPlainHttpTarget()
{
    auto target = parseHttpRedirectTarget("http://127.0.0.1:8080");
    require(target.has_value(), "http://127.0.0.1:8080 parses");
    require(*target == HttpRedirectTarget{L"127.0.0.1", 8080, false}, "host, port and plain HTTP are kept");
}

void testDefaultPortsAndTrailingSlash()
{
    auto http = parseHttpRedirectTarget("http://Server.Example/");
    require(http && http->port == 80 && !http->secure && http->host == L"server.example",
            "http defaults to port 80, a trailing slash is allowed, the host is lower-cased");
    auto https = parseHttpRedirectTarget("https://server.example");
    require(https && https->port == 443 && https->secure, "https defaults to port 443 and keeps TLS");
}

void testRejectedTargets()
{
    require(!parseHttpRedirectTarget("ftp://host"), "other schemes are rejected");
    require(!parseHttpRedirectTarget("127.0.0.1:8080"), "a target needs a scheme");
    require(!parseHttpRedirectTarget("http://host/path"), "a path is rejected: only host and port change");
    require(!parseHttpRedirectTarget("http://host?x=1"), "a query is rejected");
    require(!parseHttpRedirectTarget("http://host:0"), "port 0 is rejected");
    require(!parseHttpRedirectTarget("http://host:65536"), "a port above 65535 is rejected");
    require(!parseHttpRedirectTarget("http://host:80a"), "a port with trailing junk is rejected");
    require(!parseHttpRedirectTarget("http://host:"), "an empty port is rejected");
    require(!parseHttpRedirectTarget("http://"), "an empty host is rejected");
    require(!parseHttpRedirectTarget("http://bad host"), "a host with a space is rejected");
}

void testRulesLookup()
{
    HttpRedirectRules rules;
    require(rules.empty(), "a new table is empty");
    require(rules.add("api.example.com", "http://127.0.0.1:8080").has_value(), "a valid rule is added");
    require(!rules.add("bad host", "http://127.0.0.1").has_value(), "an invalid server name is refused");
    require(!rules.add("other.example.com", "nope").has_value(), "an invalid target is refused");
    require(rules.size() == 1, "refused rules add nothing");

    auto hit = rules.find(L"API.Example.COM");
    require(hit && hit->host == L"127.0.0.1" && hit->port == 8080, "lookup ignores case");
    require(!rules.find(L"sub.api.example.com"), "lookup is exact, not a suffix match");
    require(!rules.find(L""), "an empty server name matches nothing");

    require(rules.add("api.example.com", "https://10.0.0.2").has_value(), "a rule can be replaced");
    require(rules.find(L"api.example.com")->secure, "the last rule for a server name wins");
}

}

int main()
{
    testPlainHttpTarget();
    testDefaultPortsAndTrailingSlash();
    testRejectedTargets();
    testRulesLookup();

    std::cout << "[OK] test_http_redirect: " << g_checks << " checks passed." << std::endl;
    return 0;
}
