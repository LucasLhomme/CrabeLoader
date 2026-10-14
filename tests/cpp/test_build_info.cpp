/*
** CrabeLoader
** File description:
** Tests the build label this test was itself built with, as stamped by tools/build_stamp.cmake.
** A development label must name the version, the date and time, and the commit; a release, the version alone.
** Reads nothing from disk.
**
** Authors: @LucasLhomme
*/

#include <cstdlib>
#include <iostream>
#include <regex>
#include <string>

#include "shared/build_info.hpp"
#include "shared/version.hpp"

namespace {

/// Asserts that a boolean condition is satisfied or exits immediately.
void require(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "[FAIL] " << message << std::endl;
        std::exit(1);
    }
}

} // namespace

/// Checks the shape of the label, commit and time against the plain version.
int main()
{
    const std::string label(crabe::build::label());
    const std::string commit(crabe::build::commit());
    const std::string version(crabe::version::String);

    require(std::regex_match(commit, std::regex(R"(([0-9a-f]{7,}(-dirty)?)|unknown)")), "commit: " + commit);
    require(std::regex_match(std::string(crabe::build::time()), std::regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2})")),
            "time: " + std::string(crabe::build::time()));
    if (crabe::build::isRelease()) {
        require(label == version, "a release is labelled with the plain version: " + label);
    } else {
        require(label.starts_with(version + "-dev."), "a development label starts with the version: " + label);
        require(std::regex_match(label, std::regex(R"(\d+\.\d+\.\d+-dev\.\d{8}\.\d{4}\+.+)")), "label shape: " + label);
        require(label.ends_with("+" + commit), "the label ends with the commit: " + label);
    }
    std::cout << "[PASS] test_build_info (" << label << ")" << std::endl;
    return 0;
}
