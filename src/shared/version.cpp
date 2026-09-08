/*
** CrabeLoader
** File description:
** Version helpers implementation
*/

#include <charconv>
#include <string_view>

#include "shared/version.hpp"

namespace Crabe::Version {

    namespace {

        struct SemVer {
            uint32_t major{0};
            uint32_t minor{0};
            uint32_t patch{0};
        };

        uint32_t parseComponent(std::string_view& str)
        {
            uint32_t value = 0;
            auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
            if (ec == std::errc()) {
                str.remove_prefix(ptr - str.data());
            }
            if (!str.empty() && str.front() == '.') {
                str.remove_prefix(1);
            }
            return value;
        }

        SemVer parseSemVer(std::string_view str)
        {
            if (!str.empty() && (str.front() == 'v' || str.front() == 'V')) {
                str.remove_prefix(1);
            }
            SemVer ver;
            ver.major = parseComponent(str);
            ver.minor = parseComponent(str);
            ver.patch = parseComponent(str);
            return ver;
        }

    } // namespace

    bool isCompatible(std::string_view requiredVersion)
    {
        if (requiredVersion.empty())
            return true;

        SemVer req = parseSemVer(requiredVersion);
        if (Major < req.major)
            return false;
        if (Major > req.major)
            return true;
        if (Minor < req.minor)
            return false;
        if (Minor > req.minor)
            return true;
        return Patch >= req.patch;
    }

} // namespace Crabe::Version
