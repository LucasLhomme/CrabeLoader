/*
** CrabeLoader
** File description:
** Version definitions for CrabeLoader (Generated automatically by CMake)
*/

#ifndef CRABELOADER_VERSION_HPP_
#define CRABELOADER_VERSION_HPP_

#include <cstdint>
#include <string_view>

namespace Crabe::Version {

    constexpr uint32_t Major = 0;
    constexpr uint32_t Minor = 2;
    constexpr uint32_t Patch = 0;

    constexpr std::string_view String = "0.2.0";
    constexpr std::string_view BuildDate = __DATE__;
    constexpr std::string_view BuildTime = __TIME__;

    bool isCompatible(std::string_view requiredVersion);

} // namespace Crabe::Version

#endif /* !CRABELOADER_VERSION_HPP_ */
