/*
** CrabeLoader
** File description:
** Version constants CMake stamps in from include/shared/version.hpp.in; never edit the output.
** Carries the stamped build version and the compatibility test a manifest is checked against.
** Holds no range parser beyond that test; the full one is domain/semver.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_VERSION_HPP_
#define CRABELOADER_VERSION_HPP_

#include <cstdint>
#include <string_view>

namespace crabe::version {

    constexpr uint32_t Major = 0;
    constexpr uint32_t Minor = 2;
    constexpr uint32_t Patch = 0;

    constexpr std::string_view String = "0.2.0";
    constexpr std::string_view BuildDate = __DATE__;
    constexpr std::string_view BuildTime = __TIME__;

    bool isCompatible(std::string_view requiredVersion);

} // namespace crabe::version

#endif /* !CRABELOADER_VERSION_HPP_ */
