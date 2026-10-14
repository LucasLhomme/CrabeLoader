/*
** CrabeLoader
** File description:
** Declares what tells one build from another: its label, commit and time, stamped at every build.
** A development build reads 1.1.0-dev.20261015.0012+46329b2; a release build reads 1.1.0.
** Defined in the generated build_stamp.cpp; version.hpp keeps the plain x.y.z that mods compare.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_SHARED_BUILD_INFO_HPP_
#define CRABELOADER_SHARED_BUILD_INFO_HPP_

#include <string_view>

namespace crabe::build {

    /// The name shown to people: the plain version for a release, with date, time and commit otherwise.
    [[nodiscard]] std::string_view label() noexcept;

    /// The short commit the build came from, with -dirty when uncommitted changes were built in.
    [[nodiscard]] std::string_view commit() noexcept;

    /// When the build ran, as YYYY-MM-DD HH:MM in the builder's local time.
    [[nodiscard]] std::string_view time() noexcept;

    /// Whether this is a release build (the release workflow sets CRABELOADER_RELEASE_BUILD).
    [[nodiscard]] bool isRelease() noexcept;

} // namespace crabe::build

#endif /* !CRABELOADER_SHARED_BUILD_INFO_HPP_ */
